#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#else
#include <GL3/gl3w.h>
#endif

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C

#undef nelem
#define nelem(x) (sizeof(x)/sizeof(x)[0])

#define strsep xstrsep
#define strlcpy xstrlcpy
#define strlcat xstrlcat

int chartorune(int *rune, char *str);
int runetochar(char *str, int *rune);

char *xstrsep(char **stringp, const char *delim);
int xstrlcpy(char *dst, const char *src, int siz);
int xstrlcat(char *dst, const char *src, int siz);

#include "sys_hook.h"

#define SLUM(x) ((x)*(x)) /* approximate sRGB to Linear conversion */
#define SRGB(r,g,b) SLUM(r),SLUM(g),SLUM(b)
#define SRGBA(r,g,b,a) SRGB(r,g,b),(a)

/* shaders */

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef float mat4[16];

enum {
	ATT_POSITION,
	ATT_TEXCOORD,
	ATT_NORMAL,
	ATT_TANGENT,
	ATT_BLEND_INDEX,
	ATT_BLEND_WEIGHT,
	ATT_COLOR,
};

unsigned char *load_file(char *filename, int *lenp);
int compile_shader(const char *vert_src, const char *frag_src);

/* models */

#define MAXBONE 80

struct mesh {
	unsigned int texture;
	int first, count;
};

struct bone {
	char name[32];
	int parent;
	mat4 inv_bind_matrix;
};

struct pose {
	vec3 translate;
	vec4 rotate;
	vec3 scale;
};

struct model {
	unsigned int vao, vbo, ibo;
	int mesh_count, bone_count;
	struct mesh *mesh;
	struct bone *bone;
	struct pose *bind_pose;
};

struct model *load_obj_model(char *filename);
struct model *load_iqm_model(char *filename);
struct model *load_iqm_model_from_memory(char *filename, unsigned char *data, int len);

void draw_model(struct model *model, mat4 projection, mat4 model_view);


/* texture loader based on stb_image */

unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);

int make_texture(unsigned int texid, unsigned char *data, int w, int h, int n, int srgb);
int load_texture_from_memory(unsigned int texid, unsigned char *data, int len, int srgb);
int load_texture(unsigned int texid, char *filename, int srgb);

void icon_set_color(float r, float g, float b, float a);
void icon_begin(mat4 projection);
void icon_end(void);
void icon_show(int texture,
		float x0, float y0, float x1, float y1,
		float s0, float t0, float s1, float t1);

/* fonts based on stb_truetype */

struct font *load_font(char *filename);
struct font *load_font_from_memory(unsigned char *data, int len);
void free_font(struct font *font);
float font_width(struct font *font, float size, char *str);

void text_begin(mat4 projection);
void text_set_color(float r, float g, float b, float a);
void text_set_font(struct font *font, float size);
float text_show(float x, float y, char *text);
float text_width(char *text);
void text_end(void);

/* drawing flat shaded primitives */

void draw_set_color(float r, float g, float b, float a);
void draw_begin(mat4 projection, mat4 model_view);
void draw_end(void);
void draw_line(float x0, float y0, float z0, float x1, float y1, float z1);
void draw_rect(float x0, float y0, float x1, float y1);
void draw_triangle(float x0, float y0, float z0,
	float x1, float y1, float z1,
	float x2, float y2, float z2);
void draw_quad(float x0, float y0, float z0,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3);

/* console */

void console_init(void);
void console_update(int key, int mod);
void console_print(const char *s);
void console_printnl(const char *s);
void console_draw(mat4 projection, struct font *font, float size);

/* 4x4 column major matrices, vectors and quaternions */

void mat_identity(mat4 m);
void mat_copy(mat4 p, const mat4 m);
void mat_mix(mat4 m, const mat4 a, const mat4 b, float v);
void mat_mul(mat4 m, const mat4 a, const mat4 b);
void mat_mul44(mat4 m, const mat4 a, const mat4 b);
void mat_frustum(mat4 m, float left, float right, float bottom, float top, float n, float f);
void mat_perspective(mat4 m, float fov, float aspect, float near, float far);
void mat_ortho(mat4 m, float left, float right, float bottom, float top, float n, float f);
void mat_scale(mat4 m, float x, float y, float z);
void mat_rotate_x(mat4 m, float angle);
void mat_rotate_y(mat4 m, float angle);
void mat_rotate_z(mat4 m, float angle);
void mat_translate(mat4 m, float x, float y, float z);
void mat_transpose(mat4 to, const mat4 from);
void mat_invert(mat4 out, const mat4 m);

void mat_vec_mul(vec3 p, const mat4 m, const vec3 v);
void mat_vec_mul_n(vec3 p, const mat4 m, const vec3 v);
void mat_vec_mul_t(vec3 p, const mat4 m, const vec3 v);
void vec_scale(vec3 p, const vec3 v, float s);
void vec_add(vec3 p, const vec3 a, const vec3 b);
void vec_sub(vec3 p, const vec3 a, const vec3 b);
void vec_lerp(vec3 p, const vec3 a, const vec3 b, float t);
void vec_average(vec3 p, const vec3 a, const vec3 b);
void vec_cross(vec3 p, const vec3 a, const vec3 b);
float vec_dot(const vec3 a, const vec3 b);
float vec_dist2(const vec3 a, const vec3 b);
float vec_dist(const vec3 a, const vec3 b);
void vec_normalize(vec3 v);
void vec_face_normal(vec3 n, const vec3 p0, const vec3 p1, const vec3 p2);
void vec_negate(vec3 p);
void vec_yup_to_zup(vec3 v);

void quat_normalize(vec4 q);
void quat_lerp(vec4 p, const vec4 a, const vec4 b, float t);
void quat_lerp_normalize(vec4 p, const vec4 a, const vec4 b, float t);
void quat_lerp_neighbor_normalize(vec4 p, const vec4 a, const vec4 b, float t);
void mat_from_quat(mat4 m, const vec4 q);
void mat_from_pose(mat4 m, const vec3 t, const vec4 q, const vec3 s);
