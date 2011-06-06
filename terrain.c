#include "mio.h"

#define TILESIZE 128

struct tile {
	int w, h;
	float *z;
	unsigned char *t;
	float *vba;
	int *iba;
	unsigned int vbo, ibo, tex, program;
};

static unsigned int tileset;

void init_tileset_slice(int i, char *filename)
{
	int w, h;
	unsigned char *data;
	data = stbi_load(filename, &w, &h, 0, 3);
	if (data) {
		printf("loading slice %d with %s (%dx%d)\n", i, filename, w, h);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
		free(data);
	}
}

void init_tileset(void)
{
	glGenTextures(1, &tileset);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tileset);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 128, 128, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	init_tileset_slice(0, "data/terrain/sand.png");
	init_tileset_slice(1, "data/terrain/grass1.png");
	init_tileset_slice(2, "data/terrain/grass2.png");
	init_tileset_slice(3, "data/terrain/earth.png");
}

struct tile *load_tile(char *filename)
{
	struct tile *tile;
	int w, h, x, y, v, t, *q;
	float *p, z;

	unsigned char *png = stbi_load(filename, &w, &h, 0, 1);
	if (!png) {
		fprintf(stderr, "cannot load terrain tile: '%s'\n", filename);
		return NULL;
	}

	tile = malloc(sizeof *tile);
	tile->w = w;
	tile->h = h;
	tile->z = malloc(w * h * sizeof(float));
	tile->t = malloc(w * h);
	tile->vba = 0;
	tile->iba = 0;
	tile->vbo = tile->ibo = tile->tex = 0;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			v = png[y * w + x];
			tile->z[y * w + x] = v / 5.0f;
			if (v < 64) t = 0;
			else if (v < 128) t = 1;
			else if (v < 160) t = 2;
			else t = 3;
			tile->t[y * w + x] = t;
		}
	}

	tile->vba = p = malloc((w + 1) * (h + 1) * 8 * sizeof(float));
	for (y = 0; y < h + 1; y++) {
		for (x = 0; x < w + 1; x++) {
			if (x < w && y < h) z = tile->z[y * w + x];
			else z = 0;
			*p++ = x * 2;
			*p++ = y * 2;
			*p++ = z;
			*p++ = (float) x / w;
			*p++ = (float) y / h;
			*p++ = 0;
			*p++ = 0;
			*p++ = 1;
		}
	}

	tile->iba = q = malloc(w * h * 6 * sizeof(int));
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			int i00 = (x+0) + (y+0) * (w+1);
			int i10 = (x+1) + (y+0) * (w+1);
			int i11 = (x+1) + (y+1) * (w+1);
			int i01 = (x+0) + (y+1) * (w+1);
			*q++ = i00; *q++ = i10; *q++ = i11;
			*q++ = i00; *q++ = i11; *q++ = i01;
		}
	}

	for (t = 0; t < w * h * 6; t += 3) {
		float n[3];
		q = tile->iba + t;
		vec_face_normal(n, &tile->vba[q[0]*8], &tile->vba[q[1]*8], &tile->vba[q[2]*8]);
		vec_add(&tile->vba[q[0]*8+5], &tile->vba[q[0]*8+5], n);
		vec_add(&tile->vba[q[1]*8+5], &tile->vba[q[1]*8+5], n);
		vec_add(&tile->vba[q[2]*8+5], &tile->vba[q[2]*8+5], n);
	}

	for (t = 0; t < (w+1) * (h+1); t++)
		vec_normalize(&tile->vba[t*8+5]);

	tile->program = compile_shader("terrain.vs", "terrain.fs");
	glUseProgram(tile->program);
	glUniform1i(glGetUniformLocation(tile->program, "control_tex"), 0);
	glUniform1i(glGetUniformLocation(tile->program, "tile_tex"), 1);

	glGenBuffers(1, &tile->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tile->vbo);
	glBufferData(GL_ARRAY_BUFFER, (w+1)*(h+1)*8*sizeof(float), tile->vba, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &tile->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, w*h*6*sizeof(int), tile->iba, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenTextures(1, &tile->tex);
	glBindTexture(GL_TEXTURE_2D, tile->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tile->t);

	free(png);
	return tile;
}

void draw_tile(struct tile *tile)
{
	glUseProgram(tile->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tile->tex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tileset);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, tile->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile->ibo);
	glVertexPointer(3, GL_FLOAT, 8*4, (float*)0+0);
	glTexCoordPointer(2, GL_FLOAT, 8*4, (float*)0+3);
	glNormalPointer(GL_FLOAT, 8*4, (float*)0+5);
	glDrawElements(GL_TRIANGLES, tile->w * tile->h * 6, GL_UNSIGNED_INT, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glUseProgram(0);
}
