#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "queue.h" // freebsd sys/queue.h
#include "tree.h" // freebsd sys/tree.h

#include <GL/gl3w.h>

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextFlags(a)
#define glutInitContextProfile(a)
#define GLUT_SRGB GLUT_RGB
#include <GLUT/glut.h>
#else
#define GLUT_3_2_CORE_PROFILE 0
#include <GL/freeglut.h>
#endif

void warn(const char *fmt, ...);

const char *gl_error_string(GLenum code);
void gl_assert(const char *msg);
void gl_assert_framebuffer(GLenum target, const char *msg);

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

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#define CLAMP(x,a,b) MIN(MAX(x,a),b)

#define strsep xstrsep
#define strlcpy xstrlcpy
#define strlcat xstrlcat

int chartorune(int *rune, char *str);
int runetochar(char *str, int *rune);

char *xstrsep(char **stringp, const char *delim);
int xstrlcpy(char *dst, const char *src, int siz);
int xstrlcat(char *dst, const char *src, int siz);

#define SLUM(x) ((x)*(x)) /* approximate sRGB to Linear conversion */
#define SRGB(r,g,b) SLUM(r),SLUM(g),SLUM(b)
#define SRGBA(r,g,b,a) SRGB(r,g,b),(a)

/* matrix math utils */

#include "vector.h"

struct pose {
	vec3 position;
	vec4 rotation;
	vec3 scale;
};

void calc_mul_matrix(mat4 *model_from_bind_pose, mat4 *abs_pose_matrix, mat4 *inv_bind_matrix, int count);
void calc_inv_matrix(mat4 *inv_bind_matrix, mat4 *abs_bind_matrix, int count);
void calc_abs_matrix(mat4 *abs_pose_matrix, mat4 *pose_matrix, int *parent, int count);
void calc_matrix_from_pose(mat4 *pose_matrix, struct pose *pose, int count);

/* archive data file loading */

void register_directory(const char *dirname);
void register_archive(const char *zipname);
unsigned char *load_file(const char *filename, int *lenp);

/* resource cache */

struct cache;

void *lookup(struct cache *cache, const char *key);
struct cache *insert(struct cache *cache, const char *key, void *value);
void print_cache(struct cache *cache);

/* texture loader based on stb_image */

unsigned char *stbi_load(const char *filename, int *x, int *y, int *comp, int req_comp);
unsigned char *stbi_load_from_memory(const unsigned char *buffer, int len, int *x, int *y, int *comp, int req_comp);

int make_texture(unsigned char *data, int w, int h, int n, int srgb);
int load_texture(char *filename, int srgb);

int make_texture_array(unsigned char *data, int w, int h, int d, int n, int srgb);
int load_texture_array(char *filename, int srgb, int *d);

void icon_set_color(float r, float g, float b, float a);
void icon_begin(mat4 clip_from_view, mat4 view_from_world);
void icon_end(void);
void icon_show(int texture,
		float x0, float y0, float x1, float y1,
		float s0, float t0, float s1, float t1);

/* fonts based on stb_truetype */

struct font *load_font(char *filename);
struct font *load_font_from_memory(char *filename, unsigned char *data, int len);
void free_font(struct font *font);
float font_width(struct font *font, float size, char *str);

void text_begin(mat4 clip_from_view, mat4 view_from_world);
void text_set_color(float r, float g, float b, float a);
void text_set_font(struct font *font, float size);
float text_show(float x, float y, char *text);
float text_width(char *text);
void text_end(void);

/* drawing flat shaded primitives */

void draw_set_color(float r, float g, float b, float a);
void draw_begin(mat4 clip_from_view, mat4 view_from_world);
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

void init_lua(void);
void run_string(const char *cmd);
void run_file(const char *filename);

void console_keyboard(int key, int mod);
void console_special(int key, int mod);
void console_printf(const char *fmt, ...);
void console_print(const char *s);
void console_printnl(const char *s);
void console_putc(int c);
void console_draw(mat4 clip_from_view, mat4 view_from_world, struct font *font, float size);

/* shaders */

enum {
	ATT_POSITION,
	ATT_NORMAL,
	ATT_TANGENT,
	ATT_TEXCOORD,
	ATT_COLOR,
	ATT_BLEND_INDEX,
	ATT_BLEND_WEIGHT,
	ATT_LIGHTMAP,
	ATT_SPLAT,
	ATT_WIND,
};

enum {
	FRAG_COLOR = 0,
	FRAG_NORMAL = 0,
	FRAG_ALBEDO = 1,
};

enum {
	MAP_COLOR = GL_TEXTURE0,
	MAP_GLOSS,
	MAP_NORMAL,
	MAP_SHADOW,
	MAP_DEPTH,
	MAP_EMISSION,
	MAP_LIGHT,
	MAP_SPLAT,
};

int compile_shader(const char *vert_src, const char *frag_src);

/* materials */

int load_material(char *dirname, char *material);

/* models and animations */

#define MAXBONE 80

struct model {
	struct skel *skel;
	struct mesh *mesh;
	struct anim *anim;
};

struct part {
	unsigned int material;
	int first, count;
};

struct skel {
	int count;
	char name[MAXBONE][32];
	int parent[MAXBONE];
	struct pose pose[MAXBONE];
};

struct mesh {
	unsigned int vao, vbo, ibo;
	int enabled;
	int count;
	struct part *part;
	struct skel *skel;
	mat4 *inv_bind_matrix;
};

struct anim_map {
	struct skel *skel;
	struct anim_map *next;
	int anim_map[MAXBONE];
};

struct anim {
	char name[32];
	int frames, channels;
	struct skel *skel;
	float *data;
	struct anim *next;
	int mask[MAXBONE];
	struct pose pose[MAXBONE];
	struct anim_map *anim_map_head;
};

struct model *load_iqe_from_memory(const char *filename, unsigned char *data, int len);
struct model *load_iqm_from_memory(const char *filename, unsigned char *data, int len);
struct model *load_obj_from_memory(const char *filename, unsigned char *data, int len);
struct model *load_model(const char *filename);

struct skel *load_skel(const char *filename);
struct mesh *load_mesh(const char *filename);
struct anim *load_anim(const char *filename);

void extract_frame(struct pose *pose, struct anim *anim, int frame);
void draw_skel(mat4 *abs_pose_matrix, int *parent, int count);

/* scene graph */

enum { TAG_SCENE = 42, TAG_ARMATURE, TAG_OBJECT, TAG_LAMP };

struct armature
{
	int tag;

	LIST_ENTRY(armature) list;

	struct skel *skel;

	struct armature *parent;
	int parent_tag;

	int dirty;
	vec3 position;
	vec4 rotation;
	vec3 scale;
	mat4 transform;

	struct anim *anim;
	float time;
	int *anim_map;

	mat4 model_pose[MAXBONE];
};

struct object
{
	int tag;

	LIST_ENTRY(object) list;

	struct mesh *mesh;

	struct armature *parent;
	int parent_tag;
	unsigned char parent_map[MAXBONE]; // TODO: move into mesh, like for anim_map

	int dirty;
	vec3 position;
	vec4 rotation;
	vec3 scale;
	mat4 transform;

	vec3 color;

	mat4 *model_from_bind_pose;
};

enum { LAMP_POINT, LAMP_SPOT, LAMP_SUN };

struct lamp
{
	int tag;

	LIST_ENTRY(lamp) list;

	struct armature *parent;
	int parent_tag;

	int dirty;
	vec3 position;
	vec4 rotation;
	vec3 scale;
	mat4 transform;

	int type;
	vec3 color;
	float energy;
	float distance;
	float spot_angle;
	float spot_blend;
	int use_sphere;
	int use_shadow;
};

struct scene
{
	int tag;
	LIST_HEAD(armature_list, armature) armatures;
	LIST_HEAD(object_list, object) objects;
	LIST_HEAD(lamp_list, lamp) lamps;
};

struct scene *new_scene(void);
struct armature *new_armature(struct scene *scene, const char *skelname);
struct object *new_object(struct scene *scene, const char *meshname);
struct lamp *new_lamp(struct scene *scene);

int armature_set_parent(struct armature *node, struct armature *parent, const char *tagname);
int object_set_parent(struct object *node, struct armature *parent, const char *tagname);
int lamp_set_parent(struct lamp *node, struct armature *parent, const char *tagname);

void armature_clear_parent(struct armature *node);
void object_clear_parent(struct object *node);
void lamp_clear_parent(struct lamp *node);

void play_anim(struct armature *node, struct anim *anim, float time);
void stop_anim(struct armature *node);

void update_scene(struct scene *scene, float time);
void draw_scene(struct scene *scene, mat4 projection, mat4 view);

void render_scene_geometry(struct scene *scene, mat4 proj, mat4 view);
void render_scene_light(struct scene *scene, mat4 proj, mat4 view);

/* deferred shading */

void render_static_mesh(struct mesh *mesh, mat4 clip_from_view, mat4 view_from_model);
void render_skinned_mesh(struct mesh *mesh, mat4 clip_from_view, mat4 view_from_model, mat4 *model_from_bind_pose);

void render_point_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world);
void render_spot_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world);
void render_sun_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world);

void render_sky(void);

void render_reshape(int w, int h);
void render_geometry_pass(void);
void render_light_pass(void);
void render_forward_pass(void);
void render_finish(void);

void render_blit(mat4 proj, mat4 view, int w, int h);
void render_debug_buffers(mat4 proj, mat4 view);
