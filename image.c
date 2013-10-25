#include "mio.h"

#define STBI_NO_HDR
#include "stb_image.c"

static struct cache *texture_cache = NULL;
static struct cache *texture_array_cache = NULL;

int make_texture(unsigned char *data, int w, int h, int n, int srgb)
{
	unsigned int texid;
	int intfmt, fmt;

	if ((w & (w-1)) || (h & (h-1)))
		warn("warning: non-power-of-two texture size (%dx%d)!", w, h);

	glGenTextures(1, &texid);

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	if (n == 1) { intfmt = fmt = GL_RED; }
	if (n == 2) { intfmt = fmt = GL_RG; }
	if (n == 3) { intfmt = GL_SRGB; fmt = GL_RGB; }
	if (n == 4) { intfmt = GL_SRGB_ALPHA; fmt = GL_RGBA; }
	if (!srgb) intfmt = fmt;

	glTexImage2D(GL_TEXTURE_2D, 0, intfmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);

	if (w > 1 || h > 1)
		glGenerateMipmap(GL_TEXTURE_2D);

	return texid;
}

static int make_white_texture(void)
{
	static unsigned char white_pixel[4] = { 255, 255, 255, 255 };
	static int white_texture = 0;
	if (!white_texture)
		white_texture = make_texture(white_pixel, 1, 1, 4, 1);
	return white_texture;
}

static inline int getint(unsigned char *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static int load_dds_from_memory(char *filename, unsigned char *data, int srgb)
{
	unsigned int texid;
	int h, w, mips, flags, size, bs, fmt, i;
	unsigned char *four;

	if (memcmp(data, "DDS ", 4) || getint(data + 4) != 124) {
		warn("error: not a DDS texture: '%s'", filename);
		return 0;
	}

	h = getint(data + 3*4);
	w = getint(data + 4*4);
	mips = getint(data + 7*4);

	if ((w & (w-1)) || (h & (h-1)))
		warn("warning: non-power-of-two DDS texture size (%dx%d): '%s'", w, h, filename);

	flags = getint(data + 20*4);
	four = data + 21*4;

	if (!(flags & 4)) {
		warn("error: not a compressed DDS texture: '%s'", filename);
		return 0;
	}

	if (!memcmp(four, "DXT1", 4)) {
		bs = 8;
		if (flags & 1) {
			if (srgb)
				fmt = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
			else
				fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		} else {
			if (srgb)
				fmt = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
			else
				fmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
	} else if (!memcmp(four, "DXT3", 4)) {
		bs = 16;
		if (srgb)
			fmt = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		else
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	} else if (!memcmp(four, "DXT5", 4)) {
		bs = 16;
		if (srgb)
			fmt = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		else
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	} else {
		warn("error: unknown format in DDS texture (%4s): '%s'", four, filename);
		return 0;
	}

	glGenTextures(1, &texid);

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	size = MAX(4, w) / 4 * MAX(4, h) / 4 * bs;
	data = data + 128;
	for (i = 0; i < mips; i++) {
		glCompressedTexImage2D(GL_TEXTURE_2D, i, fmt, w, h, 0, size, data);
		data += size;
		w = (w + 1) >> 1;
		h = (h + 1) >> 1;
		size = MAX(4, w) / 4 * MAX(4, h) / 4 * bs;
	}

	return texid;
}

static int load_texture_from_memory(char *filename, unsigned char *data, int len, int srgb)
{
	unsigned int texid;
	unsigned char *image;
	int w, h, n;

	if (!memcmp(data, "DDS ", 4))
		return load_dds_from_memory(filename, data, srgb);

	image = stbi_load_from_memory(data, len, &w, &h, &n, 0);
	if (!image) {
		warn("error: cannot decode image '%s': %s", filename, stbi_failure_reason());
		return 0;
	}
	texid = make_texture(image, w, h, n, srgb);
	free(image);
	return texid;
}

int load_texture(char *filename, int srgb)
{
	intptr_t texid;
	unsigned char *data;
	int len;

	texid = (intptr_t) lookup(texture_cache, filename);
	if (texid)
		return texid;

	data = load_file(filename, &len);
	if (data) {
		texid = load_texture_from_memory(filename, data, len, srgb);
		free(data);
	} else {
		warn("error: cannot load image file: '%s'", filename);
	}

	if (texid)
		texture_cache = insert(texture_cache, filename, (void*)texid);

	return texid ? texid : srgb ? make_white_texture() : 0;
}

/*
 * Texture arrays. Load and slice a tall image into a GL_TEXTURE_2D_ARRAY.
 * Each slice is square, the number of slices determined by image h/w.
 */

int make_texture_array(unsigned char *data, int w, int h, int d, int n, int srgb)
{
	unsigned int texid;
	int intfmt, fmt;

	if ((w & (w-1)) || (h & (h-1)))
		warn("warning: non-power-of-two texture size (%dx%d)!", w, h);

	glGenTextures(1, &texid);

	glBindTexture(GL_TEXTURE_2D_ARRAY, texid);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (n == 1) { intfmt = fmt = GL_RED; }
	if (n == 2) { intfmt = fmt = GL_RG; }
	if (n == 3) { intfmt = GL_SRGB; fmt = GL_RGB; }
	if (n == 4) { intfmt = GL_SRGB_ALPHA; fmt = GL_RGBA; }
	if (!srgb) intfmt = fmt;

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, intfmt, w, h, d, 0, fmt, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	return texid;
}

static int load_texture_array_from_memory(char *filename, unsigned char *data, int len, int srgb, int *d)
{
	unsigned int texid;
	unsigned char *image;
	int w, h, n;

	image = stbi_load_from_memory(data, len, &w, &h, &n, 0);
	if (!image) {
		warn("error: cannot decode image '%s': %s", filename, stbi_failure_reason());
		return 0;
	}
	if (d)
		*d = h / w;
	texid = make_texture_array(image, w, w, h / w, n, srgb);
	free(image);
	return texid;
}

int load_texture_array(char *filename, int srgb, int *d)
{
	intptr_t texid;
	unsigned char *data;
	int len;

	texid = (intptr_t) lookup(texture_array_cache, filename);
	if (texid) {
		glBindTexture(GL_TEXTURE_2D_ARRAY, texid);
		if (d)
			glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_DEPTH, d);
		return texid;
	}

	printf("loading texture array '%s'", filename);

	data = load_file(filename, &len);
	if (!data) return 0;
	texid = load_texture_array_from_memory(filename, data, len, srgb, d);
	free(data);

	if (texid)
		texture_array_cache = insert(texture_array_cache, filename, (void*)texid);

	return texid;
}

/*
 * Icon drawing.
 */

static const char *icon_vert_src =
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

static const char *icon_frag_src =
	"uniform sampler2D map_color;\n"
	"in vec2 var_texcoord;\n"
	"in vec4 var_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	vec4 color = texture(map_color, var_texcoord);\n"
	"	color.rgb *= color.a; /* pre-multiply input texture */\n"
	"	frag_color = color * var_color;\n"
	"}\n"
;

static unsigned int icon_vao = 0;
static unsigned int icon_vbo = 0;
static int icon_buf_len = 0;
static struct {
	float position[2];
	float texcoord[2];
	float color[4];
} icon_buf[6];

static float icon_color[4] = { 1, 1, 1, 1 };

void icon_set_color(float r, float g, float b, float a)
{
	icon_color[0] = r;
	icon_color[1] = g;
	icon_color[2] = b;
	icon_color[3] = a;
}

void icon_begin(mat4 clip_from_view, mat4 view_from_world)
{
	static int icon_prog = 0;
	static int icon_uni_clip_from_view = -1;
	static int icon_uni_view_from_world = -1;

	if (!icon_prog) {
		icon_prog = compile_shader(icon_vert_src, icon_frag_src);
		icon_uni_clip_from_view = glGetUniformLocation(icon_prog, "clip_from_view");
		icon_uni_view_from_world = glGetUniformLocation(icon_prog, "view_from_world");
	}

	if (!icon_vao) {
		glGenVertexArrays(1, &icon_vao);
		glBindVertexArray(icon_vao);

		glGenBuffers(1, &icon_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, icon_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof icon_buf, NULL, GL_STREAM_DRAW);

		glEnableVertexAttribArray(ATT_POSITION);
		glEnableVertexAttribArray(ATT_TEXCOORD);
		glEnableVertexAttribArray(ATT_COLOR);
		glVertexAttribPointer(ATT_POSITION, 2, GL_FLOAT, 0, sizeof icon_buf[0], (void*)0);
		glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, sizeof icon_buf[0], (void*)8);
		glVertexAttribPointer(ATT_COLOR, 4, GL_FLOAT, 0, sizeof icon_buf[0], (void*)16);
	}

	glEnable(GL_BLEND);

	glUseProgram(icon_prog);
	glUniformMatrix4fv(icon_uni_clip_from_view, 1, 0, clip_from_view);
	glUniformMatrix4fv(icon_uni_view_from_world, 1, 0, view_from_world);

	glBindVertexArray(icon_vao);
}

void icon_end(void)
{
	glDisable(GL_BLEND);
}

static void add_vertex(float x, float y, float s, float t, float *color)
{
	icon_buf[icon_buf_len].position[0] = x;
	icon_buf[icon_buf_len].position[1] = y;
	icon_buf[icon_buf_len].texcoord[0] = s;
	icon_buf[icon_buf_len].texcoord[1] = t;
	icon_buf[icon_buf_len].color[0] = color[0];
	icon_buf[icon_buf_len].color[1] = color[1];
	icon_buf[icon_buf_len].color[2] = color[2];
	icon_buf[icon_buf_len].color[3] = color[3];
	icon_buf_len++;
}

void icon_show(int texture,
		float x0, float y0, float x1, float y1,
		float s0, float t0, float s1, float t1)
{
	icon_buf_len = 0;

	add_vertex(x0, y0, s0, t0, icon_color);
	add_vertex(x0, y1, s0, t1, icon_color);
	add_vertex(x1, y1, s1, t1, icon_color);

	add_vertex(x0, y0, s0, t0, icon_color);
	add_vertex(x1, y1, s1, t1, icon_color);
	add_vertex(x1, y0, s1, t0, icon_color);

	glActiveTexture(MAP_COLOR);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindBuffer(GL_ARRAY_BUFFER, icon_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof icon_buf[0] * icon_buf_len, icon_buf);
	glDrawArrays(GL_TRIANGLES, 0, icon_buf_len);
}
