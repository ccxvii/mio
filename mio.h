#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include <GL3/gl3w.h>

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

enum {
	ATT_POSITION,
	ATT_TEXCOORD,
	ATT_NORMAL,
	ATT_COLOR,
	ATT_BLEND_INDEX,
	ATT_BLEND_WEIGHT,
};

int compile_shader(const char *vert_src, const char *frag_src);

/* texture loader based on stb_image */

unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);

int make_texture(unsigned int texid, unsigned char *data, int w, int h, int n, int srgb);
int load_texture_from_memory(unsigned int texid, unsigned char *data, int len, int srgb);
int load_texture(unsigned int texid, char *filename, int srgb);

/* fonts based on stb_truetype */

struct font *load_font(char *filename);
struct font *load_font_from_memory(unsigned char *data, int len);
void free_font(struct font *font);
float font_width(struct font *font, float size, char *str);

void text_begin(float projection[16]);
void text_set_color(float r, float g, float b, float a);
void text_set_font(struct font *font, float size);
float text_show(float x, float y, char *text);
float text_width(char *text);
void text_end(void);

/* drawing flat shaded primitives */

void draw_set_color(float r, float g, float b, float a);
void draw_begin(float projection[16], float model_view[16]);
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
void console_draw(float projection[16], struct font *font, float size);

/* 4x4 column major matrices, vectors and quaternions */

void mat_identity(float m[16]);
void mat_copy(float p[16], const float m[16]);
void mat_mix(float m[16], const float a[16], const float b[16], float v);
void mat_mul(float m[16], const float a[16], const float b[16]);
void mat_mul44(float m[16], const float a[16], const float b[16]);
void mat_frustum(float m[16], float left, float right, float bottom, float top, float n, float f);
void mat_perspective(float m[16], float fov, float aspect, float near, float far);
void mat_ortho(float m[16], float left, float right, float bottom, float top, float n, float f);
void mat_scale(float m[16], float x, float y, float z);
void mat_rotate_x(float m[16], float angle);
void mat_rotate_y(float m[16], float angle);
void mat_rotate_z(float m[16], float angle);
void mat_translate(float m[16], float x, float y, float z);
void mat_transpose(float to[16], const float from[16]);
void mat_invert(float out[16], const float m[16]);

void mat_vec_mul(float p[3], const float m[16], const float v[3]);
void mat_vec_mul_n(float p[3], const float m[16], const float v[3]);
void mat_vec_mul_t(float p[3], const float m[16], const float v[3]);
void vec_scale(float p[3], const float v[3], float s);
void vec_add(float p[3], const float a[3], const float b[3]);
void vec_sub(float p[3], const float a[3], const float b[3]);
void vec_lerp(float p[3], const float a[3], const float b[3], float t);
void vec_average(float p[3], const float a[3], const float b[3]);
void vec_cross(float p[3], const float a[3], const float b[3]);
float vec_dot(const float a[3], const float b[3]);
float vec_dist2(const float a[3], const float b[3]);
float vec_dist(const float a[3], const float b[3]);
void vec_normalize(float v[3]);
void vec_face_normal(float n[3], const float *p0, const float *p1, const float *p2);
void vec_yup_to_zup(float v[3]);

void quat_normalize(float q[4]);
void quat_lerp(float p[4], const float a[4], const float b[4], float t);
void quat_lerp_normalize(float p[4], const float a[4], const float b[4], float t);
void quat_lerp_neighbor_normalize(float p[4], float a[4], float b[4], float t);
void mat_from_quat_vec(float m[16], const float q[4], const float v[3]);

/* 3d model loading: Wavefront OBJ and Inter-Quake Model */

struct model *load_obj_model(char *filename);
void draw_obj_model(struct model *model);
float measure_obj_radius(struct model *model);
void draw_obj_bbox(struct model *model);

struct model *load_iqm_model(char *filename);
void draw_iqm_model(struct model *model);
void draw_iqm_bones(struct model *model);
void animate_iqm_model(struct model *model, int anim, int frame, float v);
float measure_iqm_radius(struct model *model);
char *get_iqm_animation_name(struct model *model, int anim);

/* Console */

void init_console(char *fontname, float fontsize);
void update_console(int key, int mod);
void draw_console(int screenw, int screenh);
