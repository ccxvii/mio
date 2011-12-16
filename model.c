#include "mio.h"

static const char *static_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"attribute vec3 att_Position;\n"
	"attribute vec2 att_TexCoord;\n"
	"attribute vec3 att_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"void main() {\n"
	"	vec4 position = ModelView * vec4(att_Position, 1.0);\n"
	"	vec4 normal = ModelView * vec4(att_Normal, 0.0);\n"
	"	gl_Position = Projection * position;\n"
	"	var_Normal = normal.xyz;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *bone_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"uniform mat4 SkinMatrix[80];\n"
	"attribute vec3 att_Position;\n"
	"attribute vec2 att_TexCoord;\n"
	"attribute vec3 att_Normal;\n"
	"attribute vec4 att_BlendIndex;\n"
	"attribute vec4 att_BlendWeight;\n"
	"varying vec2 var_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"void main() {\n"
	"	vec4 position = vec4(0);\n"
	"	vec4 normal = vec4(0);\n"
	"	vec4 index = att_BlendIndex;\n"
	"	vec4 weight = att_BlendWeight;\n"
	"	for (int i = 0; i < 4; i++) {\n"
	"		mat4 m = SkinMatrix[int(index.x)];\n"
	"		position += m * vec4(att_Position, 1) * weight.x;\n"
	"		normal += m * vec4(att_Normal, 0) * weight.x;\n"
	"		index = index.yzwx;\n"
	"		weight = weight.yzwx;\n"
	"	}\n"
	"	position = ModelView * position;\n"
	"	normal = ModelView * normal;\n"
	"	gl_Position = Projection * position;\n"
	"	var_Normal = normal.xyz;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *model_frag_src =
	"#version 120\n"
	"uniform sampler2D Texture;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 color = texture2D(Texture, var_TexCoord);\n"
	"	if (color.a < 0.2) discard;\n"
	"	gl_FragColor = color;\n"
	"}\n"
;

void apply_pose2(mat4 *skin_matrix, struct bone *bonelist, struct pose *pose, int count)
{
	int i;
	mat4 local_matrix, world_matrix[MAXBONE];
	for (i = 0; i < count; i++) {
		if (bonelist[i].parent >= 0) {
			mat_from_pose(local_matrix, pose[i].translate, pose[i].rotate, pose[i].scale);
			mat_mul(world_matrix[i], world_matrix[bonelist[i].parent], local_matrix);
		} else {
			mat_from_pose(world_matrix[i], pose[i].translate, pose[i].rotate, pose[i].scale);
		}
		mat_mul(skin_matrix[i], world_matrix[i], bonelist[i].inv_bind_matrix);
	}
}

void apply_pose(mat4 *world_matrix, struct bone *bonelist, struct pose *pose, int count)
{
	int i;
	mat4 local_matrix;
	for (i = 0; i < count; i++) {
		if (bonelist[i].parent >= 0) {
			mat_from_pose(local_matrix, pose[i].translate, pose[i].rotate, pose[i].scale);
			mat_mul(world_matrix[i], world_matrix[bonelist[i].parent], local_matrix);
		} else {
			mat_from_pose(world_matrix[i], pose[i].translate, pose[i].rotate, pose[i].scale);
		}
	}
}

void extract_pose(struct pose *pose, struct animation *anim, int frame)
{
	float *p;
	int i;

	if (frame < 0) frame = 0;
	if (frame >= anim->frame_count) frame = anim->frame_count - 1;

	p = anim->frame + frame * anim->frame_size;
	for (i = 0; i < anim->bone_count; i++) {
		int mask = anim->chan[i].mask;
		memcpy(pose + i, &anim->chan[i].pose, sizeof *pose);
		if (mask & 0x01) pose[i].translate[0] = *p++;
		if (mask & 0x02) pose[i].translate[1] = *p++;
		if (mask & 0x04) pose[i].translate[2] = *p++;
		if (mask & 0x08) pose[i].rotate[0] = *p++;
		if (mask & 0x10) pose[i].rotate[1] = *p++;
		if (mask & 0x20) pose[i].rotate[2] = *p++;
		if (mask & 0x40) pose[i].rotate[3] = *p++;
		if (mask & 0x80) pose[i].scale[0] = *p++;
		if (mask & 0x100) pose[i].scale[1] = *p++;
		if (mask & 0x200) pose[i].scale[2] = *p++;
	}
}

void
draw_skeleton(struct bone *bonelist, mat4 *bonematrix, int count)
{
	int i;
	for (i = 0; i < count; i++) {
		float *a = bonematrix[i];
		if (bonelist[i].parent >= 0) {
			float *b = bonematrix[bonelist[i].parent];
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
	}
}

static int static_prog = 0;
static int static_uni_projection;
static int static_uni_model_view;

void draw_model(struct model *model, mat4 projection, mat4 model_view)
{
	int i;

	if (!model)
		return;

	if (!static_prog) {
		static_prog = compile_shader(static_vert_src, model_frag_src);
		static_uni_projection = glGetUniformLocation(static_prog, "Projection");
		static_uni_model_view = glGetUniformLocation(static_prog, "ModelView");
	}

	glUseProgram(static_prog);
	glUniformMatrix4fv(static_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(static_uni_model_view, 1, 0, model_view);

	glBindVertexArray(model->vao);

	for (i = 0; i < model->mesh_count; i++) {
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].texture);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}

static int bone_prog = 0;
static int bone_uni_projection;
static int bone_uni_model_view;
static int bone_uni_skin_matrix;

void draw_model_with_pose(struct model *model, mat4 projection, mat4 model_view, mat4 *skin_matrix)
{
	int i;

	if (!model)
		return;

	if (!bone_prog) {
		bone_prog = compile_shader(bone_vert_src, model_frag_src);
		bone_uni_projection = glGetUniformLocation(bone_prog, "Projection");
		bone_uni_model_view = glGetUniformLocation(bone_prog, "ModelView");
		bone_uni_skin_matrix = glGetUniformLocation(bone_prog, "SkinMatrix");
	}

	glUseProgram(bone_prog);
	glUniformMatrix4fv(bone_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(bone_uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(bone_uni_skin_matrix, model->bone_count, 0, skin_matrix[0]);

	glBindVertexArray(model->vao);

	for (i = 0; i < model->mesh_count; i++) {
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].texture);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
