#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

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

/* OpenGL headers and extension voodoo */

#ifdef __APPLE__
#include <OpenGL/gl.h>
// no extension voodoo for apple
#else
#include <GL/gl.h>
#include "glext.h"

#ifndef MIO_GLEXT_C
#define glCompressedTexImage2D mioCompressedTexImage2D
#define glCreateProgram mioCreateProgram
#define glCreateShader mioCreateShader
#define glShaderSource mioShaderSource
#define glCompileShader mioCompileShader
#define glAttachShader mioAttachShader
#define glLinkProgram mioLinkProgram
#define glValidateProgram mioValidateProgram
#define glGetProgramiv mioGetProgramiv
#define glGetProgramInfoLog mioGetProgramInfoLog
#define glUseProgram mioUseProgram
#define glMultiTexCoord4fv mioMultiTexCoord4fv
#endif

extern PFNGLCOMPRESSEDTEXIMAGE2DPROC mioCompressedTexImage2D;
extern PFNGLCREATEPROGRAMPROC mioCreateProgram;
extern PFNGLCREATESHADERPROC mioCreateShader;
extern PFNGLSHADERSOURCEPROC mioShaderSource;
extern PFNGLCOMPILESHADERPROC mioCompileShader;
extern PFNGLATTACHSHADERPROC mioAttachShader;
extern PFNGLLINKPROGRAMPROC mioLinkProgram;
extern PFNGLVALIDATEPROGRAMPROC mioValidateProgram;
extern PFNGLGETPROGRAMIVPROC mioGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC mioGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC mioUseProgram;
extern PFNGLMULTITEXCOORD4FVPROC mioMultiTexCoord4fv;

#endif

extern void init_glext();

int compile_shader(char *vertfile, char *fragfile);

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
void mat_mul(float m[16], const float a[16], const float b[16]);
void mat_mul44(float m[16], const float a[16], const float b[16]);
void mat_frustum(float m[16], float left, float right, float bottom, float top, float n, float f);
void mat_ortho(float m[16], float left, float right, float bottom, float top, float n, float f);
void mat_scale(float m[16], float x, float y, float z);
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
void vec_face_normal(float n[3], const float *face, int p0, int p1, int p2);
void vec_yup_to_zup(float v[3]);

void quat_normalize(float q[4]);
void quat_lerp(float p[4], const float a[4], const float b[4], float t);
void quat_lerp_normalize(float p[4], const float a[4], const float b[4], float t);
void quat_lerp_neighbor_normalize(float p[4], float a[4], float b[4], float t);
void mat_from_quat_vec(float m[16], const float q[4], const float v[3]);

/* 3d model loading: Wavefront OBJ and Inter-Quake Model */

struct model *load_obj_model(char *filename);
void draw_obj_model(struct model *model);
void load_obj_animation(struct model *model, char *filename);
void draw_obj_model_frame(struct model *model, int frame);

struct model *load_iqm_model(char *filename);
void draw_iqm_model(struct model *model);
void draw_iqm_bones(struct model *model);
void animate_iqm_model(struct model *model, int anim, int frame);

struct model *load_iqe_model(char *filename);
void draw_iqe_model(struct model *model);
void draw_iqe_bones(struct model *model);
void animate_iqe_model(struct model *model, int anim, int frame);

// terrain height field

struct tile *load_tile(char *filename);
void draw_tile(struct tile *tile);

void init_console(char *fontname, float fontsize);
void update_console(int key, int mod);
void draw_console(int screenw, int screenh);
