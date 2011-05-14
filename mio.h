#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opengl.h"
#include "syshook.h"
#include "utf.h"

#define strsep xstrsep
#define strlcpy xstrlcpy
#define strlcat xstrlcat

#undef nelem
#define nelem(x) (sizeof(x)/sizeof(x)[0])

struct font *load_font(char *filename);
struct font *keep_font(struct font *font);
void drop_font(struct font *font);
float measure_string(struct font *font, float size, char *str);
float draw_string(struct font *font, float size, float x, float y, char *str);

int make_texture(unsigned char *data, int w, int h, int n);
int load_texture(char *filename, int *w, int *h, int *n, int req_n);

struct md2_model * md2_load_model(char *modelname);
void md2_free_model(struct md2_model *self);
int md2_get_frame_count(struct md2_model *self);
char *md2_get_frame_name(struct md2_model *self, int idx);
void md2_draw_frame(struct md2_model *self, int skin, int frame);
void md2_draw_frame_lerp(struct md2_model *self, int skin, int idx0, int idx1, float lerp);
