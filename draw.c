#include "mio.h"

/*
 * Line and rect drawing.
 */

static int draw_prog = 0;
static int draw_uni_projection = -1;
static int draw_uni_model_view = -1;

static const char *draw_vert_src =
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"in vec3 att_Position;\n"
	"in vec4 att_Color;\n"
	"out vec4 var_Color;\n"
	"void main() {\n"
	"	vec4 position = ModelView * vec4(att_Position, 1.0);\n"
	"	gl_Position = Projection * position;\n"
	"	var_Color = att_Color;\n"
	"}\n"
;

static const char *draw_frag_src =
	"in vec4 var_Color;\n"
	"out vec4 frag_Color;\n"
	"void main() {\n"
	"	frag_Color = var_Color;\n"
	"}\n"
;

#define MAXBUFFER (256*2)	/* size of our triangle buffer */

static unsigned int draw_vao = 0;
static unsigned int draw_vbo = 0;
static int draw_buf_len = 0;
static struct {
	float position[3];
	float color[4];
} draw_buf[MAXBUFFER];

static float draw_color[4] = { 1, 1, 1, 1 };
static int draw_kind = GL_LINES;
static float draw_color[4];

void draw_set_color(float r, float g, float b, float a)
{
	draw_color[0] = r;
	draw_color[1] = g;
	draw_color[2] = b;
	draw_color[3] = a;
}

void draw_begin(float projection[16], float model_view[16])
{
	if (!draw_prog) {
		draw_prog = compile_shader(draw_vert_src, draw_frag_src);
		draw_uni_projection = glGetUniformLocation(draw_prog, "Projection");
		draw_uni_model_view = glGetUniformLocation(draw_prog, "ModelView");
	}

	if (!draw_vao) {
		glGenVertexArrays(1, &draw_vao);
		glBindVertexArray(draw_vao);

		glGenBuffers(1, &draw_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof draw_buf, NULL, GL_STREAM_DRAW);

		glEnableVertexAttribArray(ATT_POSITION);
		glEnableVertexAttribArray(ATT_COLOR);
		glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, sizeof draw_buf[0], (void*)0);
		glVertexAttribPointer(ATT_COLOR, 4, GL_FLOAT, 0, sizeof draw_buf[0], (void*)12);
	}

	glEnable(GL_BLEND);

	glUseProgram(draw_prog);
	glUniformMatrix4fv(draw_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(draw_uni_model_view, 1, 0, model_view);

	glBindVertexArray(draw_vao);
}

static void draw_flush(void)
{
	if (draw_buf_len > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, draw_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof draw_buf[0] * draw_buf_len, draw_buf);
		glDrawArrays(draw_kind, 0, draw_buf_len);
		draw_buf_len = 0;
	}
}

void draw_end(void)
{
	draw_flush();

	glDisable(GL_BLEND);
}

static void draw_vertex(float x, float y, float z)
{
	draw_buf[draw_buf_len].position[0] = x;
	draw_buf[draw_buf_len].position[1] = y;
	draw_buf[draw_buf_len].position[2] = z;
	draw_buf[draw_buf_len].color[0] = draw_color[0];
	draw_buf[draw_buf_len].color[1] = draw_color[1];
	draw_buf[draw_buf_len].color[2] = draw_color[2];
	draw_buf[draw_buf_len].color[3] = draw_color[3];
	draw_buf_len++;
}

void draw_line(float x0, float y0, float z0, float x1, float y1, float z1)
{
	if (draw_kind != GL_LINES)
		draw_flush();
	if (draw_buf_len + 2 >= MAXBUFFER)
		draw_flush();

	draw_kind = GL_LINES;
	draw_vertex(x0, y0, z0);
	draw_vertex(x1, y1, z1);
}

void draw_triangle(float x0, float y0, float z0,
	float x1, float y1, float z1,
	float x2, float y2, float z2)
{
	if (draw_kind != GL_TRIANGLES)
		draw_flush();
	if (draw_buf_len + 3 >= MAXBUFFER)
		draw_flush();

	draw_kind = GL_TRIANGLES;
	draw_vertex(x0, y0, z0);
	draw_vertex(x1, y1, z1);
	draw_vertex(x2, y2, z2);
}

void draw_quad(float x0, float y0, float z0,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3)
{
	if (draw_kind != GL_TRIANGLES)
		draw_flush();
	if (draw_buf_len + 6 >= MAXBUFFER)
		draw_flush();

	draw_kind = GL_TRIANGLES;
	draw_vertex(x0, y0, z0);
	draw_vertex(x1, y1, z1);
	draw_vertex(x2, y2, z2);
	draw_vertex(x0, y0, z0);
	draw_vertex(x2, y2, z2);
	draw_vertex(x3, y3, z3);
}

void draw_rect(float x0, float y0, float x1, float y1)
{
	if (draw_kind != GL_TRIANGLES)
		draw_flush();
	if (draw_buf_len + 6 >= MAXBUFFER)
		draw_flush();

	draw_kind = GL_TRIANGLES;
	draw_vertex(x0, y0, 0);
	draw_vertex(x0, y1, 0);
	draw_vertex(x1, y1, 0);
	draw_vertex(x0, y0, 0);
	draw_vertex(x1, y1, 0);
	draw_vertex(x1, y0, 0);
}
