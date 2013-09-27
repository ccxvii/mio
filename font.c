/*
 * A very simple font cache and rasterizer that uses stb_truetype
 * to draw fonts from a single OpenGL texture. The code uses
 * a linear-probe hashtable, and writes new glyphs into
 * the texture using glTexSubImage2D. When the texture fills
 * up, or the hash table gets too crowded, the cache is emptied.
 *
 * This is designed to be used for horizontal text only,
 * and draws unhinted text with subpixel accurate metrics
 * and kerning. As such, you should always call the drawing
 * function with an orthogonal transform that maps units
 * to pixels accurately.
 */

#include "mio.h"

static void text_flush(void);
static void clear_glyph_cache(void);

static struct cache *font_cache = NULL;

/*
 * TrueType font
 */

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u) malloc(x)
#define STBTT_free(x,u) free(x)
#include "stb_truetype.h"

struct font
{
	int tag;
	unsigned char *data;
	stbtt_fontinfo info;
};

struct font *load_font_from_memory(const char *filename, unsigned char *data, int len)
{
	struct font *font;
	int ok;

	font = malloc(sizeof(struct font));
	font->tag = TAG_FONT;
	font->data = malloc(len);
	memcpy(font->data, data, len);

	ok = stbtt_InitFont(&font->info, font->data, 0);
	if (!ok) {
		warn("error: cannot init font file: '%s'", filename);
		return NULL;
	}

	return font;
}

struct font *load_font(const char *filename)
{
	struct font *font;
	int ok;

	font = lookup(font_cache, filename);
	if (font)
		return font;

	font = malloc(sizeof(struct font));
	font->tag = TAG_FONT;
	font->data = load_file(filename, NULL);
	if (!font->data) {
		warn("error: cannot load font file: '%s'", filename);
		return NULL;
	}

	ok = stbtt_InitFont(&font->info, font->data, 0);
	if (!ok) {
		warn("error: cannot init font file: '%s'", filename);
		return NULL;
	}

	if (font)
		font_cache = insert(font_cache, filename, font);

	return font;
}

void free_font(struct font *font)
{
	clear_glyph_cache();
	free(font->data);
	free(font);
}

/*
 * Glyph cache
 */

#define PADDING 1	/* set to 0 to save some space but disallow arbitrary transforms */

#define MAXGLYPHS 4093	/* prime number for hash table goodness */
#define CACHESIZE 512
#define XPRECISION 4
#define YPRECISION 1

struct key
{
	struct font *font;
	int size;
	int gid;
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

static unsigned char cache_zero[CACHESIZE * CACHESIZE];
static unsigned int cache_tex = 0;
static int cache_row_y = PADDING;
static int cache_row_x = PADDING;
static int cache_row_h = 0;

static void clear_glyph_cache(void)
{
	text_flush();

	memset(table, 0, sizeof table);
	table_load = 0;

	glBindTexture(GL_TEXTURE_2D, cache_tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CACHESIZE, CACHESIZE,
		GL_RED, GL_UNSIGNED_BYTE, cache_zero);

	cache_row_y = PADDING;
	cache_row_x = PADDING;
	cache_row_h = 0;
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

	memset(&key, 0, sizeof key);
	key.font = font;
	key.size = scale * 10000;
	key.gid = gid;
	key.subx = subx;
	key.suby = suby;

	pos = lookup_table(&key);
	if (table[pos].key.font)
		return &table[pos].glyph;

	/* Render the bitmap */

	shift_x = (float)subx / XPRECISION;
	shift_y = (float)suby / YPRECISION;

	stbtt_GetGlyphHMetrics(&font->info, gid, &advance, &lsb);
	data = stbtt_GetGlyphBitmapSubpixel(&font->info, scale, scale, shift_x, shift_y, gid, &w, &h, &x, &y);

	/* Find an empty slot in the texture */

	if (table_load == (MAXGLYPHS * 3) / 4) {
		puts("glyph cache table full, clearing cache");
		clear_glyph_cache();
		pos = lookup_table(&key);
	}

	if (h + PADDING > CACHESIZE || w + PADDING > CACHESIZE) {
		warn("error: rendered glyph exceeds cache dimensions");
		exit(1);
	}

	if (cache_row_x + w + PADDING > CACHESIZE) {
		cache_row_y += cache_row_h + PADDING;
		cache_row_x = PADDING;
		cache_row_h = 0;
	}
	if (cache_row_y + h + PADDING > CACHESIZE) {
		puts("glyph cache texture full, clearing cache");
		clear_glyph_cache();
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
				GL_RED, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	free(data);

	cache_row_x += w + PADDING;
	if (cache_row_h < h + PADDING)
		cache_row_h = h + PADDING;

	return &table[pos].glyph;
}

/*
 * Text buffer and drawing.
 */

static const char *text_vert_src =
	"uniform mat4 clip_from_view;\n"
	"uniform mat4 view_from_world;\n"
	"in vec4 att_position;\n"
	"in vec2 att_texcoord;\n"
	"in vec4 att_color;\n"
	"out vec2 var_texcoord;\n"
	"out vec4 var_color;\n"
	"void main() {\n"
	"	gl_Position = clip_from_view * view_from_world * att_position;\n"
	"	var_texcoord = att_texcoord;\n"
	"	var_color = att_color;\n"
	"}\n"
;

static const char *text_frag_src =
	"uniform sampler2D map_color;\n"
	"in vec2 var_texcoord;\n"
	"in vec4 var_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	float coverage = texture(map_color, var_texcoord).r;\n"
	"	frag_color = var_color * coverage;\n"
	"}\n"
;

#define MAXBUFFER (256*6)	/* size of our triangle buffer */

static unsigned int text_vao = 0;
static unsigned int text_vbo = 0;
static int text_buf_len = 0;
static struct {
	float position[2];
	float texcoord[2];
	float color[4];
} text_buf[MAXBUFFER];

static float text_color[4] = { 1, 1, 1, 1 };
static struct font *text_font = NULL;
static float text_scale = 1;

void text_set_color(float r, float g, float b, float a)
{
	text_color[0] = r;
	text_color[1] = g;
	text_color[2] = b;
	text_color[3] = a;
}

void text_set_font(struct font *font, float size)
{
	text_font = font;
	text_scale = stbtt_ScaleForPixelHeight(&font->info, size);
}

void text_begin(mat4 clip_from_view, mat4 view_from_world)
{
	static int text_prog = 0;
	static int text_uni_clip_from_view = -1;
	static int text_uni_view_from_world = -1;

	if (!cache_tex) {
		memset(table, 0, sizeof table);
		memset(cache_zero, 0, sizeof cache_zero);
		glGenTextures(1, &cache_tex);
		glBindTexture(GL_TEXTURE_2D, cache_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, CACHESIZE, CACHESIZE, 0,
				GL_RED, GL_UNSIGNED_BYTE, cache_zero);
	}

	if (!text_prog) {
		text_prog = compile_shader(text_vert_src, text_frag_src);
		text_uni_clip_from_view = glGetUniformLocation(text_prog, "clip_from_view");
		text_uni_view_from_world = glGetUniformLocation(text_prog, "view_from_world");
	}

	if (!text_vao) {
		glGenVertexArrays(1, &text_vao);
		glBindVertexArray(text_vao);

		glGenBuffers(1, &text_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof text_buf, NULL, GL_STREAM_DRAW);

		glEnableVertexAttribArray(ATT_POSITION);
		glEnableVertexAttribArray(ATT_TEXCOORD);
		glEnableVertexAttribArray(ATT_COLOR);
		glVertexAttribPointer(ATT_POSITION, 2, GL_FLOAT, 0, sizeof text_buf[0], (void*)0);
		glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, sizeof text_buf[0], (void*)8);
		glVertexAttribPointer(ATT_COLOR, 4, GL_FLOAT, 0, sizeof text_buf[0], (void*)16);
	}

	glEnable(GL_BLEND);

	glUseProgram(text_prog);
	glUniformMatrix4fv(text_uni_clip_from_view, 1, 0, clip_from_view);
	glUniformMatrix4fv(text_uni_view_from_world, 1, 0, view_from_world);

	glBindVertexArray(text_vao);

	glActiveTexture(MAP_COLOR);
	glBindTexture(GL_TEXTURE_2D, cache_tex);
}

static void text_flush(void)
{
	if (text_buf_len > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof text_buf[0] * text_buf_len, text_buf);
		glDrawArrays(GL_TRIANGLES, 0, text_buf_len);
		text_buf_len = 0;
	}
}

void text_end(void)
{
	text_flush();

	glDisable(GL_BLEND);
}

static void add_vertex(float x, float y, float s, float t, float *color)
{
	text_buf[text_buf_len].position[0] = x;
	text_buf[text_buf_len].position[1] = y;
	text_buf[text_buf_len].texcoord[0] = s;
	text_buf[text_buf_len].texcoord[1] = t;
	text_buf[text_buf_len].color[0] = color[0];
	text_buf[text_buf_len].color[1] = color[1];
	text_buf[text_buf_len].color[2] = color[2];
	text_buf[text_buf_len].color[3] = color[3];
	text_buf_len++;
}

static void add_rect(float x0, float y0, float x1, float y1,
	float s0, float t0, float s1, float t1)
{
	if (text_buf_len + 6 >= MAXBUFFER)
		text_flush();

	add_vertex(x0, y0, s0, t0, text_color);
	add_vertex(x0, y1, s0, t1, text_color);
	add_vertex(x1, y1, s1, t1, text_color);

	add_vertex(x0, y0, s0, t0, text_color);
	add_vertex(x1, y1, s1, t1, text_color);
	add_vertex(x1, y0, s1, t0, text_color);
}

static float add_glyph(int gid, float x, float y)
{
	struct glyph *glyph;
	int subx = (x - floor(x)) * XPRECISION;
	int suby = (y - floor(y)) * YPRECISION;

	glyph = lookup_glyph(text_font, text_scale, gid, subx, suby);
	if (!glyph)
		return 0;

	float s0 = (float) glyph->s / CACHESIZE;
	float t0 = (float) glyph->t / CACHESIZE;
	float s1 = (float) (glyph->s + glyph->w) / CACHESIZE;
	float t1 = (float) (glyph->t + glyph->h) / CACHESIZE;
	float xc = floor(x) + glyph->x;
	float yc = floor(y) + glyph->y;

	add_rect(xc, yc, xc + glyph->w, yc + glyph->h, s0, t0, s1, t1);

	return glyph->advance;
}

float text_show(float x, float y, char *str)
{
	int ucs, gid;
	int kern;
	int left = 0;

	while (*str) {
		str += chartorune(&ucs, str);
		gid = stbtt_FindGlyphIndex(&text_font->info, ucs);
		kern = stbtt_GetGlyphKernAdvance(&text_font->info, left, gid);
		x += add_glyph(gid, x, y);
		x += kern * text_scale;
		left = gid;
	}

	return x;
}

static float font_width_imp(struct font *font, float scale, char *str)
{
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

float font_width(struct font *font, float size, char *str)
{
	return font_width_imp(font, stbtt_ScaleForPixelHeight(&font->info, size), str);
}

float text_width(char *str)
{
	return font_width_imp(text_font, text_scale, str);
}
