#include "mio.h"

#define TILESIZE 128

struct tile {
	int w, h, list;
	float *z;
	char *t;
};

static int tileset[5];

void init_tileset(void)
{
	tileset[0] = load_texture(0, "data/terrain/sand.png");
	tileset[1] = load_texture(0, "data/terrain/grass1.png");
	tileset[2] = load_texture(0, "data/terrain/grass2.png");
	tileset[3] = load_texture(0, "data/terrain/earth.png");
	tileset[4] = 0;
}

struct tile *load_tile(char *filename)
{
	struct tile *tile;
	int w, h, x, y, v, t;

	unsigned char *png = stbi_load(filename, &w, &h, 0, 1);
	if (!png) {
		fprintf(stderr, "cannot load terrain tile: '%s'\n", filename);
		return NULL;
	}

	tile = malloc(sizeof *tile);
	tile->w = w;
	tile->h = h;
	tile->list = 0;
	tile->z = malloc(w * h * sizeof(float));
	tile->t = malloc(w * h);

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

	free(png);
	return tile;
}

static void draw_square(struct tile *tile, int x, int y)
{
	float z00, z01, z11, z10;
	float v[4][3];
	float a[3], b[3], c[3], ab[3], ac[3], bc[3];
	float n[3];

	z00 = tile->z[x+0 + (y+0) * tile->w];
	z10 = tile->z[x+1 + (y+0) * tile->w];
	z11 = tile->z[x+1 + (y+1) * tile->w];
	z01 = tile->z[x+0 + (y+1) * tile->w];

	#define SCALE 2

	v[0][0] = (x+0)*SCALE; v[0][1] = (y+0)*SCALE; v[0][2] = z00;
	v[1][0] = (x+1)*SCALE; v[1][1] = (y+0)*SCALE; v[1][2] = z10;
	v[2][0] = (x+1)*SCALE; v[2][1] = (y+1)*SCALE; v[2][2] = z11;
	v[3][0] = (x+0)*SCALE; v[3][1] = (y+1)*SCALE; v[3][2] = z01;

	vec_sub(a, v[1], v[0]);
	vec_sub(b, v[2], v[0]);
	vec_sub(c, v[3], v[0]);
	vec_cross(ab, a, b);
	vec_cross(ac, a, c);
	vec_cross(bc, b, c);

	vec_add(n, ab, ac);
	vec_add(n, n, bc);
	vec_normalize(n);
	glNormal3fv(n);

	glTexCoord2f(0,0); glVertex3fv(v[0]);
	glTexCoord2f(1,0); glVertex3fv(v[1]);
	glTexCoord2f(1,1); glVertex3fv(v[2]);

	glTexCoord2f(0,0); glVertex3fv(v[0]);
	glTexCoord2f(1,1); glVertex3fv(v[2]);
	glTexCoord2f(0,1); glVertex3fv(v[3]);
}

void draw_tile(struct tile *tile)
{
	int k, x, y;

	if (!tile->list) {
		tile->list = glGenLists(1);
		glNewList(tile->list, GL_COMPILE);
		for (k = 0; k < nelem(tileset); k++) {
			glBindTexture(GL_TEXTURE_2D, tileset[k]);
			glBegin(GL_TRIANGLES);
			for (y = 0; y < tile->h - 1; y++)
				for (x = 0; x < tile->w - 1; x++)
					if (tile->t[y * tile->w + x] == k)
						draw_square(tile, x, y);
			glEnd();
		}
		glEndList();
	}

	glCallList(tile->list);
}