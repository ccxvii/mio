#include "mio.h"

#include <ctype.h>

#define IQE_MAGIC "# Inter-Quake Export"

struct floatarray {
	int len, cap;
	float *data;
};

struct intarray {
	int len, cap;
	unsigned short *data;
};

struct bytearray {
	int len, cap;
	unsigned char *data;
};

struct partarray {
	int len, cap;
	struct part *data;
};

// temp buffers are global so we can reuse them between meshs
static struct floatarray position = { 0, 0, NULL };
static struct floatarray normal = { 0, 0, NULL };
static struct floatarray texcoord = { 0, 0, NULL };
static struct bytearray color = { 0, 0, NULL };
static struct bytearray blendindex = { 0, 0, NULL };
static struct bytearray blendweight = { 0, 0, NULL };
static struct intarray element = { 0, 0, NULL };
static struct partarray part = { 0, 0, NULL };

static struct bytearray customb[10] = { { 0 } };
static struct floatarray customf[10] = { { 0 } };
static int custom_format[10] = { 0 };
static int custom_count[10] = { 0 };
static char custom_name[10][80] = { { 0 } };

static inline void push_float(struct floatarray *a, float v)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static inline void push_int(struct intarray *a, int v)
{
	assert(v >= 0 && v < 65535);
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static inline void push_byte(struct bytearray *a, int v)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static inline void push_part(struct partarray *a, int first, int last, int material)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len].first = first;
	a->data[a->len].count = last - first;
	a->data[a->len].material = material;
	a->len++;
}

static void add_position(float x, float y, float z)
{
	push_float(&position, x);
	push_float(&position, y);
	push_float(&position, z);
}

static void add_texcoord(float u, float v)
{
	push_float(&texcoord, u);
	push_float(&texcoord, v);
}

static void add_normal(float x, float y, float z)
{
	push_float(&normal, x);
	push_float(&normal, y);
	push_float(&normal, z);
}

static void add_color(float r, float g, float b, float a)
{
	push_byte(&color, r * 255);
	push_byte(&color, g * 255);
	push_byte(&color, b * 255);
	push_byte(&color, a * 255);
}

static void add_blend(int a, int b, int c, int d, float x, float y, float z, float w)
{
	float total = x + y + z + w;
	push_byte(&blendindex, a);
	push_byte(&blendindex, b);
	push_byte(&blendindex, c);
	push_byte(&blendindex, d);
	push_byte(&blendweight, x * 255 / total);
	push_byte(&blendweight, y * 255 / total);
	push_byte(&blendweight, z * 255 / total);
	push_byte(&blendweight, w * 255 / total);
}

static void add_custom(int i, float x, float y, float z, float w)
{
	if (custom_format[i] == 'b') {
		if (custom_count[i] > 0) push_byte(&customb[i], x * 255);
		if (custom_count[i] > 1) push_byte(&customb[i], y * 255);
		if (custom_count[i] > 2) push_byte(&customb[i], z * 255);
		if (custom_count[i] > 3) push_byte(&customb[i], w * 255);
	}
	if (custom_format[i] == 'f') {
		if (custom_count[i] > 0) push_float(&customf[i], x * 255);
		if (custom_count[i] > 1) push_float(&customf[i], y * 255);
		if (custom_count[i] > 2) push_float(&customf[i], z * 255);
		if (custom_count[i] > 3) push_float(&customf[i], w * 255);
	}
}

static void add_triangle(int a, int b, int c)
{
	// flip triangle winding
	push_int(&element, c);
	push_int(&element, b);
	push_int(&element, a);
}

static char *parsestring(char **stringp)
{
	char *start, *end, *s = *stringp;
	while (isspace(*s)) s++;
	if (*s == '"') {
		s++;
		start = end = s;
		while (*end && *end != '"') end++;
		if (*end) *end++ = 0;
	} else {
		start = end = s;
		while (*end && !isspace(*end)) end++;
		if (*end) *end++ = 0;
	}
	*stringp = end;
	return start;
}

static char *parseword(char **stringp)
{
	char *start, *end, *s = *stringp;
	while (isspace(*s)) s++;
	start = end = s;
	while (*end && !isspace(*end)) end++;
	if (*end) *end++ = 0;
	*stringp = end;
	return start;
}

static inline float parsefloat(char **stringp, float def)
{
	char *s = parseword(stringp);
	return *s ? atof(s) : def;
}

static inline int parseint(char **stringp, int def)
{
	char *s = parseword(stringp);
	return *s ? atoi(s) : def;
}

static mat4 loc_bind_matrix[MAXBONE];
static mat4 abs_bind_matrix[MAXBONE];

struct model *load_iqe_from_memory(char *filename, unsigned char *data, int len)
{
	char dirname[1024];
	char *line, *next, *p, *s, *sp;
	struct model *model;
	struct skel *skel;
	struct mesh *mesh;
	struct anim *anim;
	char bone_name[MAXBONE][32];
	int bone_parent[MAXBONE];
	struct pose bind_pose[MAXBONE];
	struct pose *pose;
	int bone_count = 0;
	int pose_count = 0;
	int material = 0;
	int first = 0;
	int fm = 0;
	int i;

	strlcpy(dirname, filename, sizeof dirname);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strlcpy(dirname, "", sizeof dirname);

	if (memcmp(data, IQE_MAGIC, strlen(IQE_MAGIC))) {
		fprintf(stderr, "error: bad iqe magic: '%s'\n", filename);
		return NULL;
	}

	position.len = 0;
	texcoord.len = 0;
	normal.len = 0;
	color.len = 0;
	blendindex.len = 0;
	blendweight.len = 0;
	element.len = 0;
	part.len = 0;

	for (i = 0; i < 10; i++) {
		customb[i].len = 0;
		customf[i].len = 0;
		custom_format[i] = 0;
		custom_count[i] = 0;
		custom_name[i][0] = 0;
	}

	pose = bind_pose;

	data[len-1] = 0; /* over-write final newline to zero-terminate */

	for (line = (char*)data; line; line = next) {
		float x, y, z, w;
		int a, b, c, d;

		next = strchr(line, '\n');
		if (next)
			*next++ = 0;

		sp = line;
		s = parseword(&sp);
		if (!s) {
			continue;
		}

		else if (s[0] == 'v' && s[1] != 0 && s[2] == 0) {
			switch (s[1]) {
			case 'p':
				x = parsefloat(&sp, 0);
				y = parsefloat(&sp, 0);
				z = parsefloat(&sp, 0);
				add_position(x, y, z);
				break;

			case 'n':
				x = parsefloat(&sp, 0);
				y = parsefloat(&sp, 0);
				z = parsefloat(&sp, 0);
				add_normal(x, y, z);
				break;

			case 't':
				x = parsefloat(&sp, 0);
				y = parsefloat(&sp, 0);
				add_texcoord(x, y);
				break;

			case 'c':
				x = parsefloat(&sp, 0);
				y = parsefloat(&sp, 0);
				z = parsefloat(&sp, 0);
				w = parsefloat(&sp, 1);
				add_color(x, y, z, w);
				break;

			case 'b':
				a = parseint(&sp, 0);
				x = parsefloat(&sp, 1);
				b = parseint(&sp, 0);
				y = parsefloat(&sp, 0);
				c = parseint(&sp, 0);
				z = parsefloat(&sp, 0);
				d = parseint(&sp, 0);
				w = parsefloat(&sp, 0);
				add_blend(a, b, c, d, x, y, z, w);
				break;

			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				x = parsefloat(&sp, 0);
				y = parsefloat(&sp, 0);
				z = parsefloat(&sp, 0);
				w = parsefloat(&sp, 0);
				add_custom(s[1] - '0', x, y, z, w);
				break;
			}
		}

		else if (s[0] == 'f' && s[1] == 'm' && s[2] == 0) {
			a = parseint(&sp, 0);
			b = parseint(&sp, 0);
			c = parseint(&sp, -1);
			while (c > -1) {
				add_triangle(a+fm, b+fm, c+fm);
				b = c;
				c = parseint(&sp, -1);
			}
		}

		else if (s[0] == 'f' && s[1] == 'a' && s[2] == 0) {
			a = parseint(&sp, 0);
			b = parseint(&sp, 0);
			c = parseint(&sp, -1);
			while (c > -1) {
				add_triangle(a, b, c);
				b = c;
				c = parseint(&sp, -1);
			}
		}

		else if (s[0] == 'p' && s[1] == 'q' && s[2] == 0) {
			if (pose_count < MAXBONE) {
				pose[pose_count].position[0] = parsefloat(&sp, 0);
				pose[pose_count].position[1] = parsefloat(&sp, 0);
				pose[pose_count].position[2] = parsefloat(&sp, 0);
				pose[pose_count].rotation[0] = parsefloat(&sp, 0);
				pose[pose_count].rotation[1] = parsefloat(&sp, 0);
				pose[pose_count].rotation[2] = parsefloat(&sp, 0);
				pose[pose_count].rotation[3] = parsefloat(&sp, 1);
				pose[pose_count].scale[0] = parsefloat(&sp, 1);
				pose[pose_count].scale[1] = parsefloat(&sp, 1);
				pose[pose_count].scale[2] = parsefloat(&sp, 1);
				pose_count++;
			}
		}

		// TODO: "pm", "pa"

		else if (!strcmp(s, "vertexarray")) {
			char *type = parsestring(&sp);
			char *format = parsestring(&sp);
			int count = parseint(&sp, 0);
			char *name = parsestring(&sp);
			if (strstr(type, "custom") == type) {
				i = type[6] - '0';
				if (i >= 0 && i <= 9) {
					if (!strcmp(format, "ubyte"))
						custom_format[i] = 'b';
					else
						custom_format[i] = 'f';
					custom_count[i] = count;
					if (custom_format[i] == 'b' && custom_count[i] == 3)
						custom_count[i] = 4;
					strlcpy(custom_name[i], name, sizeof custom_name[i]);
				}
			}
		}

		else if (!strcmp(s, "mesh")) {
			s = parsestring(&sp);
			if (element.len > first)
				push_part(&part, first, element.len, material);
			first = element.len;
			fm = position.len / 3;
		}

		else if (!strcmp(s, "material")) {
			s = parsestring(&sp);
			material = load_material(dirname, s);
		}

		else if (!strcmp(s, "joint")) {
			if (bone_count < MAXBONE) {
				char *name = parsestring(&sp);
				strlcpy(bone_name[bone_count], name, sizeof bone_name[0]);
				bone_parent[bone_count] = parseint(&sp, -1);
				bone_count++;
			}
		}

		// TODO: animation, frame
	}

	if (element.len > first)
		push_part(&part, first, element.len, material);

	skel = NULL;
	mesh = NULL;
	anim = NULL;

	if (bone_count > 0 && pose_count >= bone_count) {
		skel = malloc(sizeof(struct skel));
		skel->count = bone_count;
		for (i = 0; i < bone_count; i++) {
			strlcpy(skel->name[i], bone_name[i], sizeof skel->name[0]);
			skel->parent[i] = bone_parent[i];
			skel->pose[i] = bind_pose[i];
		}
	}

	if (part.len) {
		mesh = malloc(sizeof(struct mesh));
		mesh->skel = skel;
		mesh->inv_bind_matrix = NULL;
		mesh->count = part.len;
		mesh->part = malloc(part.len * sizeof(struct part));
		memcpy(mesh->part, part.data, part.len * sizeof(struct part));

		if (skel) {
			mesh->inv_bind_matrix = malloc(sizeof(mat4) * skel->count);
			calc_matrix_from_pose(loc_bind_matrix, skel->pose, skel->count);
			calc_abs_matrix(abs_bind_matrix, loc_bind_matrix, skel->parent, skel->count);
			calc_inv_matrix(mesh->inv_bind_matrix, abs_bind_matrix, skel->count);
		}

		int vertexcount = position.len / 3;
		int total = 12;
		if (normal.len / 3 == vertexcount) total += 12;
		if (texcoord.len / 2 == vertexcount) total += 8;
		if (color.len / 4 == vertexcount) total += 4;
		if (blendindex.len / 4 == vertexcount) total += 4;
		if (blendweight.len / 4 == vertexcount) total += 4;

		glGenVertexArrays(1, &mesh->vao);
		glGenBuffers(1, &mesh->vbo);
		glGenBuffers(1, &mesh->ibo);

		glBindVertexArray(mesh->vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

		glBufferData(GL_ARRAY_BUFFER, vertexcount * total * 4, NULL, GL_STATIC_DRAW);

		glEnableVertexAttribArray(ATT_POSITION);
		glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 0, (void*)0);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexcount * 12, position.data);
		total = vertexcount * 12;
	
		if (normal.len / 3 == vertexcount) {
			glEnableVertexAttribArray(ATT_NORMAL);
			glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 0, (void*)total);
			glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 12, normal.data);
			total += vertexcount * 12;
		}
		if (texcoord.len / 2 == vertexcount) {
			glEnableVertexAttribArray(ATT_TEXCOORD);
			glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 0, (void*)total);
			glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 8, texcoord.data);
			total += vertexcount * 8;
		}
		if (color.len / 4 == vertexcount) {
			glEnableVertexAttribArray(ATT_COLOR);
			glVertexAttribPointer(ATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)total);
			glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, color.data);
			total += vertexcount * 4;
		}
		if (blendindex.len / 4 == vertexcount) {
			glEnableVertexAttribArray(ATT_BLEND_INDEX);
			glVertexAttribPointer(ATT_BLEND_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, (void*)total);
			glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, blendindex.data);
			total += vertexcount * 4;
		}
		if (blendweight.len / 4 == vertexcount) {
			glEnableVertexAttribArray(ATT_BLEND_WEIGHT);
			glVertexAttribPointer(ATT_BLEND_WEIGHT, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)total);
			glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, blendweight.data);
			total += vertexcount * 4;
		}
	
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element.len * 2, element.data, GL_STATIC_DRAW);
	}

	model = malloc(sizeof *model);
	model->skel = skel;
	model->mesh = mesh;
	model->anim = NULL;
	return model;
}
