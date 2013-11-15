#include "mio.h"

/*
 * Line and rect drawing.
 */

static const char *draw_vert_src =
	"uniform mat4 clip_from_view;\n"
	"uniform mat4 view_from_world;\n"
	"in vec4 att_position;\n"
	"in vec4 att_color;\n"
	"out vec4 var_color;\n"
	"void main() {\n"
	"	gl_Position = clip_from_view * view_from_world * att_position;\n"
	"	var_color = att_color;\n"
	"}\n"
;

static const char *draw_frag_src =
	"in vec4 var_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	frag_color = var_color;\n"
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

void draw_set_color(float r, float g, float b, float a)
{
	draw_color[0] = r;
	draw_color[1] = g;
	draw_color[2] = b;
	draw_color[3] = a;
}

void draw_begin(mat4 clip_from_view, mat4 view_from_world)
{
	static int draw_prog = 0;
	static int draw_uni_clip_from_view = -1;
	static int draw_uni_view_from_world = -1;

	if (!draw_prog) {
		draw_prog = compile_shader(draw_vert_src, draw_frag_src);
		draw_uni_clip_from_view = glGetUniformLocation(draw_prog, "clip_from_view");
		draw_uni_view_from_world = glGetUniformLocation(draw_prog, "view_from_world");
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
	glUniformMatrix4fv(draw_uni_clip_from_view, 1, 0, clip_from_view);
	glUniformMatrix4fv(draw_uni_view_from_world, 1, 0, view_from_world);

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
