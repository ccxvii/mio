/*
 * A very simple font cache and rasterizer that uses stb_truetype
 * to draw fonts from a single OpenGL texture. The code uses
 * a linear-probe hashtable, and writes new glyphs into
 * the texture using glTexSubImage2D. When the texture fills
 * up, or the hash table gets too crowded, everything is wiped.
 *
 * This is designed to be used for horizontal text only,
 * and draws unhinted text with subpixel accurate metrics
 * and kerning. As such, you should always call the drawing
 * function with an identity transform that maps units
 * to pixels accurately.
 *
 * If you wish to use it to draw arbitrarily transformed
 * text, change the min and mag filters to GL_LINEAR and
 * add a pixel of padding between glyphs and rows, and
 * make sure to clear the texture when wiping the cache.
 */

#include "mio.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u) malloc(x)
#define STBTT_free(x,u) free(x)
#include "stb_truetype.h"

struct font {
	int refs;
	unsigned char *data;
	stbtt_fontinfo info;
};

#define PADDING 1	/* set to 0 to save some space but disallow arbitrary transforms */

#define MAXGLYPHS 4093	/* prime number for hash table goodness */
#define CACHESIZE 512
#define XPRECISION 4
#define YPRECISION 1

static inline void die(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

struct key
{
	struct font *font;
	short size;
	short gid;
	short subx;
	short suby;
};

struct glyph
{
	short x, y, w, h;
	short s, t;
	float advance;
};

struct table
{
	struct key key;
	struct glyph glyph;
};

static struct table table[MAXGLYPHS];
static int table_load = 0;
static unsigned char cache_data[CACHESIZE * CACHESIZE];
static unsigned int cache_tex = 0;
static int cache_row_y = 0;
static int cache_row_x = 0;
static int cache_row_h = 0;

static void clear_font_cache(void)
{
	memset(cache_data, 0, CACHESIZE * CACHESIZE);
	glBindTexture(GL_TEXTURE_2D, cache_tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CACHESIZE, CACHESIZE,
		GL_ALPHA, GL_UNSIGNED_BYTE, cache_data);

	memset(table, 0, sizeof(table));
	table_load = 0;

	cache_row_y = PADDING;
	cache_row_x = PADDING;
	cache_row_h = 0;
}

static void init_font_cache(void)
{
	glGenTextures(1, &cache_tex);
	glBindTexture(GL_TEXTURE_2D, cache_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, CACHESIZE, CACHESIZE, 0,
			GL_ALPHA, GL_UNSIGNED_BYTE, cache_data);

	clear_font_cache();
}

struct font *load_font(char *filename)
{
	struct font *font;
	int len, ok;
	FILE *fp;

	static int once = 1;
	if (once) {
		init_font_cache();
		once = 0;
	}

	fp = fopen(filename, "rb");
	if (!fp)
		die("cannot open truetype font");

	fseek(fp, 0, 2);
	len = ftell(fp);
	fseek(fp, 0, 0);

	font = malloc(sizeof(struct font));
	font->refs = 1;
	font->data = malloc(len);

	fread(font->data, 1, len, fp);
	fclose(fp);

	ok = stbtt_InitFont(&font->info, font->data, 0);
	if (!ok)
		die("cannot init truetype font");

	return font;
}

struct font *keep_font(struct font *font)
{
	font->refs++;
	return font;
}

void drop_font(struct font *font)
{
	if (--font->refs == 0) {
		clear_font_cache();
		free(font->data);
		free(font);
	}
}

static unsigned int hashfunc(struct key *key)
{
	unsigned char *buf = (unsigned char *)key;
	unsigned int len = sizeof(struct key);
	unsigned int h = 0;
	while (len--)
		h = *buf++ + (h << 6) + (h << 16) - h;
	return h;
}

static unsigned int lookup_table(struct key *key)
{
	unsigned int pos = hashfunc(key) % MAXGLYPHS;
	while (1) {
		if (!table[pos].key.font) /* empty slot */
			return pos;
		if (!memcmp(key, &table[pos].key, sizeof(struct key))) /* matching slot */
			return pos;
		pos = (pos + 1) % MAXGLYPHS;
	}
}

static struct glyph *lookup_glyph(struct font *font, float scale, int gid, int subx, int suby)
{
	struct key key;
	unsigned int pos;
	unsigned char *data;
	float shift_x, shift_y;
	int w, h, x, y;
	int advance, lsb;

	/* Look it up in the table */

	key.font = font;
	key.size = scale * 10000;
	key.gid = gid;
	key.subx = subx;
	key.suby = suby;

	pos = lookup_table(&key);
	if (table[pos].key.font)
		return &table[pos].glyph;

	/* Render the bitmap */

	glEnd();

	shift_x = (float)subx / XPRECISION;
	shift_y = (float)suby / YPRECISION;

	stbtt_GetGlyphHMetrics(&font->info, gid, &advance, &lsb);
	data = stbtt_GetGlyphBitmap(&font->info, scale, scale, shift_x, shift_y, gid, &w, &h, &x, &y);

	/* Find an empty slot in the texture */

	if (table_load == (MAXGLYPHS * 3) / 4) {
		puts("font cache table full, clearing cache");
		clear_font_cache();
		pos = lookup_table(&key);
	}

	if (h + PADDING > CACHESIZE || w + PADDING > CACHESIZE)
		die("rendered glyph exceeds cache dimensions");

	if (cache_row_x + w + PADDING > CACHESIZE) {
		cache_row_y += cache_row_h + PADDING;
		cache_row_x = PADDING;
	}
	if (cache_row_y + h + PADDING > CACHESIZE) {
		puts("font cache texture full, clearing cache");
		clear_font_cache();
		pos = lookup_table(&key);
	}

	/* Copy the bitmap into our texture */

	memcpy(&table[pos].key, &key, sizeof(struct key));
	table[pos].glyph.w = w;
	table[pos].glyph.h = h;
	table[pos].glyph.x = x;
	table[pos].glyph.y = y;
	table[pos].glyph.s = cache_row_x;
	table[pos].glyph.t = cache_row_y;
	table[pos].glyph.advance = advance * scale;
	table_load ++;

	if (data && w && h) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
		glTexSubImage2D(GL_TEXTURE_2D, 0,
				cache_row_x, cache_row_y, w, h,
				GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	free(data);

	glBegin(GL_QUADS);

	cache_row_x += w + PADDING;
	if (cache_row_h < h + PADDING)
		cache_row_h = h + PADDING;

	return &table[pos].glyph;
}

static float draw_glyph(struct font *font, float scale, int gid, float x, float y)
{
	struct glyph *glyph;
	int subx = (x - floor(x)) * XPRECISION;
	int suby = (y - floor(y)) * YPRECISION;

	glyph = lookup_glyph(font, scale, gid, subx, suby);
	if (!glyph)
		return 0.0;

	float s0 = (float) glyph->s / CACHESIZE;
	float t0 = (float) glyph->t / CACHESIZE;
	float s1 = (float) (glyph->s + glyph->w) / CACHESIZE;
	float t1 = (float) (glyph->t + glyph->h) / CACHESIZE;
	float xc = floor(x) + glyph->x;
	float yc = floor(y) + glyph->y;

#if 1
	glTexCoord2f(s0, t0); glVertex2f(xc, yc);
	glTexCoord2f(s1, t0); glVertex2f(xc + glyph->w, yc);
	glTexCoord2f(s1, t1); glVertex2f(xc + glyph->w, yc + glyph->h);
	glTexCoord2f(s0, t1); glVertex2f(xc, yc + glyph->h);
#else
	glTexCoord2f(s0, t0); glVertex2f(xc, yc - glyph->h);
	glTexCoord2f(s1, t0); glVertex2f(xc + glyph->w, yc - glyph->h);
	glTexCoord2f(s1, t1); glVertex2f(xc + glyph->w, yc);
	glTexCoord2f(s0, t1); glVertex2f(xc, yc);
#endif

	return glyph->advance;
}

float measure_string(struct font *font, float size, char *str)
{
	float scale = stbtt_ScaleForPixelHeight(&font->info, size);
	int ucs, gid;
	int advance, kern;
	float w = 0.0;
	int left = 0;

	while (*str) {
		str += chartorune(&ucs, str);
		gid = stbtt_FindGlyphIndex(&font->info, ucs);
		stbtt_GetGlyphHMetrics(&font->info, gid, &advance, NULL);
		kern = stbtt_GetGlyphKernAdvance(&font->info, left, gid);
		w += advance * scale;
		w += kern * scale;
		left = gid;
	}

	return w;
}

float draw_string(struct font *font, float size, float x, float y, char *str)
{
	float scale = stbtt_ScaleForPixelHeight(&font->info, size);
	int ucs, gid;
	int kern;
	int left = 0;

	glBindTexture(GL_TEXTURE_2D, cache_tex);
	glBegin(GL_QUADS);

	while (*str) {
		str += chartorune(&ucs, str);
		gid = stbtt_FindGlyphIndex(&font->info, ucs);
		kern = stbtt_GetGlyphKernAdvance(&font->info, left, gid);
		x += draw_glyph(font, scale, gid, x, y);
		x += kern * scale;
		left = gid;
	}

	glEnd();

	return x;
}
