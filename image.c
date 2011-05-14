#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opengl.h"

#include "stb_image.c"

int make_texture(unsigned char *data, int w, int h, int n)
{
	unsigned int texture;
	int format;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	if (n == 1) format = GL_LUMINANCE;
	if (n == 2) format = GL_LUMINANCE_ALPHA;
	if (n == 3) format = GL_RGB;
	if (n == 4) format = GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format,
		w, h, 0, format, GL_UNSIGNED_BYTE, data);

	return texture;
}

int load_texture(char *filename, int *wp, int *hp, int *np, int req_n)
{
	unsigned char *data;
	int w, h, n, t;

	data = stbi_load(filename, &w, &h, &n, req_n);
	if (!data) {
		fprintf(stderr, "error: cannot load image '%s'\n", filename);
		return 0;
	}

	if (wp) *wp = w;
	if (hp) *hp = h;
	if (np) *np = n;

	t = make_texture(data, w, h, n);

	stbi_image_free(data);

	return t;
}
