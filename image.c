#include "mio.h"

#define STBI_NO_HDR
#include "stb_image.c"

#define MAX(a,b) (a>b?a:b)

unsigned char *load_file(char *filename, int *lenp)
{
	unsigned char *data;
	int len;
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "error: cannot open '%s'\n", filename);
		return NULL;
	}
	fseek(file, 0, 2);
	len = ftell(file);
	fseek(file, 0, 0);
	data = malloc(len);
	fread(data, 1, len, file);
	fclose(file);
	if (lenp) *lenp = len;
	return data;
}

int make_texture(unsigned int texid, unsigned char *data, int w, int h, int n)
{
	int format;

	if ((w & (w-1)) || (h & (h-1)))
		fprintf(stderr, "warning: non-power-of-two texture size (%dx%d)!\n", w, h);

	if (texid == 0)
		glGenTextures(1, &texid);

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	if (n == 1) format = GL_LUMINANCE;
	if (n == 2) format = GL_LUMINANCE_ALPHA;
	if (n == 3) format = GL_RGB;
	if (n == 4) format = GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

	return texid;
}

static inline int getint(unsigned char *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static int load_dds_from_memory(unsigned int texid, unsigned char *data)
{
	int h, w, mips, flags, size, bs, fmt, i;
	unsigned char *four;

	if (memcmp(data, "DDS ", 4) || getint(data + 4) != 124) {
		fprintf(stderr, "error: not a DDS texture\n");
		return 0;
	}

	h = getint(data + 3*4);
	w = getint(data + 4*4);
	mips = getint(data + 7*4);

	if ((w & (w-1)) || (h & (h-1)))
		fprintf(stderr, "warning: non-power-of-two DDS texture size (%dx%d)!\n", w, h);

	flags = getint(data + 2*4);
	four = data + 21*4;

	if (!(flags & 4)) {
		fprintf(stderr, "error: not a compressed DDS texture\n");
		return 0;
	}

	if (!memcmp(four, "DXT1", 4)) {
		fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; bs = 8;
	} else if (!memcmp(four, "DXT3", 4)) {
		fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; bs = 16;
	} else if (!memcmp(four, "DXT5", 4)) {
		fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; bs = 16;
	} else {
		fprintf(stderr, "error: unknown format in DDS texture: '%4s'\n", four);
		return 0;
	}

	if (texid == 0)
		glGenTextures(1, &texid);

	glBindTexture(GL_TEXTURE_2D, texid);
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

static int load_dds_from_file(unsigned int texid, char *filename)
{
	unsigned char *data = load_file(filename, 0);
	if (!data)
		return 0;
	texid = load_dds_from_memory(texid, data);
	free(data);
	return texid;
}

int load_texture_from_memory(unsigned int texid, unsigned char *data, int len)
{
	unsigned char *image;
	int w, h, n;

	if (!memcmp(data, "DDS ", 4))
		return load_dds_from_memory(texid, data);

	image = stbi_load_from_memory(data, len, &w, &h, &n, 0);
	if (!image) {
		fprintf(stderr, "error: cannot decode image\n");
		return 0;
	}
	texid = make_texture(texid, image, w, h, n);
	free(image);
	return texid;
}

int load_texture(unsigned int texid, char *filename)
{
	unsigned char *image;
	int w, h, n;

	if (strstr(filename, ".dds"))
		return load_dds_from_file(texid, filename);

	image = stbi_load(filename, &w, &h, &n, 0);
	if (!image) {
		fprintf(stderr, "error: cannot load image '%s'\n", filename);
		return 0;
	}
	texid = make_texture(0, image, w, h, n);
	free(image);
	return texid;
}
