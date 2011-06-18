#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include <GL/glew.h> /* Use GLEW for OpenGL extension voodoo */

#include "utf.h"

#undef nelem
#define nelem(x) (sizeof(x)/sizeof(x)[0])

#define strsep xstrsep
#define strlcpy xstrlcpy
#define strlcat xstrlcat

char *xstrsep(char **stringp, const char *delim);
int xstrlcpy(char *dst, const char *src, int siz);
int xstrlcat(char *dst, const char *src, int siz);

unsigned char *load_file(char *filename, int *lenp);

/* shaders */

enum {
	ATT_POSITION,
	ATT_TEXCOORD,
	ATT_NORMAL,
	ATT_TANGENT,
	ATT_BLEND_INDEX,
	ATT_BLEND_WEIGHT,
	ATT_COLOR
};

struct program {
	int program;
	int vert_shader;
	int frag_shader;

	/* uniform locations */
	int model_view;
	int projection;

	int light_dir;
	int light_ambient, light_diffuse, light_specular;

	int wind, phase;
	int bone_matrix;
};

struct program *compile_shader(char *vertfile, char *fragfile);

/* image and texture loading */

unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);

int make_texture(unsigned int texid, unsigned char *data, int w, int h, int n);
int load_texture_from_memory(unsigned int texid, unsigned char *data, int len);
int load_texture(unsigned int texid, char *filename);

/* fonts based on stb_truetype */

struct font *load_font(char *filename);
struct font *keep_font(struct font *font);
void drop_font(struct font *font);
float measure_string(struct font *font, float size, char *str);
float draw_string(struct font *font, float size, float x, float y, char *str);

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
void draw_obj_instances(struct model *model, float *translate, int count);
void draw_obj_model(struct model *model, float x, float y, float z);
float measure_obj_radius(struct model *model);
void draw_obj_bbox(struct model *model);

struct model *load_iqm_model(char *filename);
void draw_iqm_instances(struct model *model, float *transform, int count);
void draw_iqm_model(struct model *model);
void draw_iqm_bones(struct model *model);
void animate_iqm_model(struct model *model, int anim, int frame, float v);
float measure_iqm_radius(struct model *model);
char *get_iqm_animation_name(struct model *model, int anim);

// terrain height field

void init_tileset(void);
struct tile *load_tile(char *filename);
void draw_tile(struct tile *tile);
float height_at_tile_location(struct tile *tile, int x, int y);

void init_console(char *fontname, float fontsize);
void update_console(int key, int mod);
void draw_console(int screenw, int screenh);
