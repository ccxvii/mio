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
static unsigned int maskset;

void init_tileset_slice(int i, char *filename)
{
	int w, h;
	unsigned char *data;
	data = stbi_load(filename, &w, &h, 0, 3);
	if (data) {
//		printf("loading slice %d with %s (%dx%d)\n", i, filename, w, h);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
		free(data);
	}
}

void init_maskset_slice(int i, char *filename)
{
	int w, h;
	unsigned char *data;
	data = stbi_load(filename, &w, &h, 0, 1);
	if (data) {
//		printf("loading slice %d with %s (%dx%d)\n", i, filename, w, h);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
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
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 128, 128, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	init_tileset_slice(0, "data/terrain/y-plages-128-a-01.png");
	init_tileset_slice(1, "data/terrain/y-plages-128-a-02.png");
	init_tileset_slice(2, "data/terrain/y-plages-128-a-03.png");
	init_tileset_slice(3, "data/terrain/y-plages-128-a-04.png");
	init_tileset_slice(4, "data/terrain/j-herbesechejungle-128-a-01_sp.png");
	init_tileset_slice(5, "data/terrain/j-herbesechejungle-128-a-02_sp.png");
	init_tileset_slice(6, "data/terrain/j-herbesechejungle-128-a-03_sp.png");
	init_tileset_slice(7, "data/terrain/j-herbesechejungle-128-a-04_sp.png");
	init_tileset_slice(8, "data/terrain/j-vieillehjungle-128-a-01_sp.png");
	init_tileset_slice(9, "data/terrain/j-vieillehjungle-128-a-02_sp.png");
	init_tileset_slice(10, "data/terrain/j-vieillehjungle-128-a-03_sp.png");
	init_tileset_slice(11, "data/terrain/j-vieillehjungle-128-a-04_sp.png");
	init_tileset_slice(12, "data/terrain/j-vieillehjungle-128-a-01_wi.png");
	init_tileset_slice(13, "data/terrain/j-vieillehjungle-128-a-02_wi.png");
	init_tileset_slice(14, "data/terrain/j-vieillehjungle-128-a-03_wi.png");
	init_tileset_slice(15, "data/terrain/j-vieillehjungle-128-a-04_wi.png");

	glGenTextures(1, &maskset);
	glBindTexture(GL_TEXTURE_2D_ARRAY, maskset);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_LUMINANCE, 64, 64, 12, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

	init_maskset_slice(0, "data/terrain/alpha_noise_00_sp.png");
	init_maskset_slice(1, "data/terrain/alpha_noise_01_sp.png");
	init_maskset_slice(2, "data/terrain/alpha_noise_02_sp.png");
	init_maskset_slice(3, "data/terrain/alpha_noise_06_sp.png");
	init_maskset_slice(4, "data/terrain/alpha_noise_07_sp.png");
	init_maskset_slice(5, "data/terrain/alpha_noise_08_sp.png");
	init_maskset_slice(6, "data/terrain/alpha_noise_12_sp.png");
	init_maskset_slice(7, "data/terrain/alpha_noise_13_sp.png");
	init_maskset_slice(8, "data/terrain/alpha_noise_14_sp.png");
	init_maskset_slice(9, "data/terrain/alpha_noise_18_sp.png");
	init_maskset_slice(10, "data/terrain/alpha_noise_19_sp.png");
	init_maskset_slice(11, "data/terrain/alpha_noise_20_sp.png");
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
	tile->t = malloc(w * h * 4);
	tile->vba = 0;
	tile->iba = 0;
	tile->vbo = tile->ibo = tile->tex = 0;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			v = png[y * w + x];
			tile->z[y * w + x] = v / 3.0f;
			if (v < 50) t = 0 + rand()%4;
			else if (v < 75) t = 4 + rand()%4;
			else if (v < 100) t = 8 + rand()%4;
			else t = 12 + rand()%4;
			tile->t[y * w * 4 + x * 4 + 0] = t;
			tile->t[y * w * 4 + x * 4 + 1] = rand()%16;
			tile->t[y * w * 4 + x * 4 + 2] = rand()%12;
			tile->t[y * w * 4 + x * 4 + 3] = 0;
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
			*p++ = x;
			*p++ = y;
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
	glUniform1i(glGetUniformLocation(tile->program, "mask_tex"), 2);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tile->t);

	free(png);
	return tile;
}

float height_at_tile_location(struct tile *tile, int x, int y)
{
	x /= 2; y /= 2;
	if (x >= 0 && y >= 0 && x < tile->w && y < tile->h)
		return tile->z[y * tile->w + x];
	return 0;
}

void draw_tile(struct tile *tile)
{
	glUseProgram(tile->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tile->tex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tileset);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, maskset);

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
