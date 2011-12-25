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
	"uniform mat4 BoneMatrix[80];\n"
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
	"		mat4 m = BoneMatrix[int(index.x)];\n"
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
	"varying vec3 var_Normal;\n"
	"const vec3 LightDirection = vec3(-0.5773, 0.5773, 0.5773);\n"
	"const vec3 LightAmbient = vec3(0.1);\n"
	"const vec3 LightDiffuse = vec3(1.0);\n"
	"void main() {\n"
//	"	vec4 color = texture2D(Texture, var_TexCoord);\n"
//	"	vec4 color = vec4(0.9, 0.8, 0.5, 1);\n"
	"	vec4 color = vec4(var_TexCoord.x, var_TexCoord.y, 1, 1);\n"
//	"	if (color.a < 0.2) discard;\n"
	"	vec3 N = normalize(var_Normal);\n"
	"	float diffuse = max(dot(N, LightDirection), 0.0);\n"
	"	vec3 Ka = color.rgb * LightAmbient;\n"
	"	vec3 Kd = color.rgb * LightDiffuse * diffuse;\n"
	"	gl_FragColor = vec4(Ka + Kd, color.a);\n"
	"}\n"
;

void calc_skin_matrix(mat4 *skin_matrix, mat4 *abs_pose_matrix, mat4 *inv_bind_matrix, int count)
{
	int i;
	for (i = 0; i < count; i++)
		mat_mul(skin_matrix[i], abs_pose_matrix[i], inv_bind_matrix[i]);
}

void calc_inv_bind_matrix(mat4 *inv_bind_matrix, mat4 *abs_bind_matrix, int count)
{
	int i;
	for (i = 0; i < count; i++)
		mat_invert(inv_bind_matrix[i], abs_bind_matrix[i]);
}

void calc_abs_pose_matrix(mat4 *abs_pose_matrix, mat4 *pose_matrix, int *parent, int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (parent[i] >= 0)
			mat_mul(abs_pose_matrix[i], abs_pose_matrix[parent[i]], pose_matrix[i]);
		else
			mat_copy(abs_pose_matrix[i], pose_matrix[i]);
}

void calc_pose_matrix(mat4 *pose_matrix, struct pose *pose, int count)
{
	int i;
	for (i = 0; i < count; i++)
		mat_from_pose(pose_matrix[i], pose[i].translate, pose[i].rotate, pose[i].scale);
}

static int findbone(char (*bone_name)[32], int count, char *name)
{
	int i;
	for (i = 0; i < count; i++)
		if (!strcmp(bone_name[i], name))
			return i;
	return -1;
}

void retarget_skeleton(mat4 *out_matrix,
		mat4 *dst_matrix, char (*dst_names)[32], int dst_count,
		mat4 *src_matrix, char (*src_names)[32], int src_count,
		mat4 *src_anim_matrix)
{
	mat4 diff_matrix, inv_src_matrix;
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			mat_copy(out_matrix[dst], dst_matrix[dst]);
		} else {
			mat_invert(inv_src_matrix, src_matrix[src]);
			mat_mul(diff_matrix, inv_src_matrix, dst_matrix[dst]);
			mat_mul(out_matrix[dst], src_anim_matrix[src], diff_matrix);
		}
	}
}

void retarget_skeleton_world(mat4 *out_matrix, mat4 *dst_local, int *dst_parent,
		mat4 *dst_matrix, char (*dst_names)[32], int dst_count,
		mat4 *src_matrix, char (*src_names)[32], int src_count,
		mat4 *src_anim_matrix)
{
	mat4 diff_matrix, inv_src_matrix;
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			if (dst_parent[dst] >= 0)
				mat_mul(out_matrix[dst], out_matrix[dst_parent[dst]], dst_local[dst]);
			else
				mat_copy(out_matrix[dst], dst_matrix[dst]);
			mat_identity(out_matrix[dst]);
		} else {
			mat_invert(inv_src_matrix, src_matrix[src]);
			mat_mul(diff_matrix, inv_src_matrix, dst_matrix[dst]);
			mat_mul(out_matrix[dst], src_anim_matrix[src], diff_matrix);
		}
	}
}


void retarget_skeleton_pose(struct pose *out_pose,
		struct pose *dst_pose, char (*dst_names)[32], int dst_count,
		struct pose *src_pose, char (*src_names)[32], int src_count,
		struct pose *src_anim_pose)
{
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			out_pose[dst] = dst_pose[dst];
		} else {
			out_pose[dst] = dst_pose[dst];
			if (dst < 2)
				out_pose[dst] = src_anim_pose[src];

			vec4 inv_src_quat, diff_quat;
			quat_conjugate(inv_src_quat, src_pose[src].rotate);
			quat_mul(diff_quat, inv_src_quat, dst_pose[dst].rotate);
			quat_mul(out_pose[dst].rotate, src_anim_pose[src].rotate, diff_quat);
		}
	}
}

void retarget_skeleton_pose_rotate(struct pose *out_pose,
		struct pose *dst_pose, char (*dst_names)[32], int dst_count,
		struct pose *src_pose, char (*src_names)[32], int src_count,
		struct pose *src_anim_pose)
{
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			out_pose[dst] = dst_pose[dst];
		} else {
			out_pose[dst] = dst_pose[dst];
		//	if (dst < 2)
		//		out_pose[dst] = src_anim_pose[src];
			memcpy(out_pose[dst].rotate, src_anim_pose[src].rotate, 4*4);
		}
	}
}

void apply_animation(
	struct pose *dst_pose, char (*dst_names)[32], int dst_count,
	struct pose *src_pose, char (*src_names)[32], int src_count)
{
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src >= 0) {
			dst_pose[dst] = src_pose[src];
		}
	}
}

void apply_animation2(
	struct pose *dst_pose, char (*dst_names)[32], int dst_count,
	struct pose *src_pose, char (*src_names)[32], int src_count,
	int *mask)
{
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src >= 0) {
			if (mask[src] & 0x01) dst_pose[dst].translate[0] = src_pose[src].translate[0];
			if (mask[src] & 0x02) dst_pose[dst].translate[1] = src_pose[src].translate[1];
			if (mask[src] & 0x04) dst_pose[dst].translate[2] = src_pose[src].translate[2];
			if (mask[src] & 0x08) dst_pose[dst].rotate[0] = src_pose[src].rotate[0];
			if (mask[src] & 0x10) dst_pose[dst].rotate[1] = src_pose[src].rotate[1];
			if (mask[src] & 0x20) dst_pose[dst].rotate[2] = src_pose[src].rotate[2];
			if (mask[src] & 0x40) dst_pose[dst].rotate[3] = src_pose[src].rotate[3];
			if (mask[src] & 0x80) dst_pose[dst].scale[0] = src_pose[src].scale[0];
			if (mask[src] & 0x100) dst_pose[dst].scale[1] = src_pose[src].scale[1];
			if (mask[src] & 0x200) dst_pose[dst].scale[2] = src_pose[src].scale[2];
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
		int mask = anim->mask[i];
		memcpy(pose + i, &anim->offset[i], sizeof *pose);
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

static int haschildren(int *parent, int count, int x)
{
	int i;
	for (i = x; i < count; i++)
		if (parent[i] == x)
			return 1;
	return 0;
}

void
draw_skeleton(mat4 *abs_pose_matrix, int *parent, int count)
{
	vec3 x = { 0.1, 0, 0 };
	int i;
	for (i = 0; i < count; i++) {
		float *a = abs_pose_matrix[i];
		if (parent[i] >= 0) {
			float *b = abs_pose_matrix[parent[i]];
			draw_set_color(1, 1, 1, 1);
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_set_color(1, 1, 0, 1);
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
		if (!haschildren(parent, count, i)) {
			vec3 b;
			mat_vec_mul(b, abs_pose_matrix[i], x);
			draw_set_color(1, 1, 0, 1);
			draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
		}
	}
}

void
draw_skeleton_2(mat4 *abs_pose_matrix, int *parent, int count)
{
	vec3 x = { 0.1, 0, 0 };
	int i;
	for (i = 0; i < count; i++) {
		float *a = abs_pose_matrix[i];
		if (parent[i] >= 0) {
			float *b = abs_pose_matrix[parent[i]];
			draw_set_color(0, 1, 1, 1);
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_set_color(0, 1, 0, 1);
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
		if (!haschildren(parent, count, i)) {
			vec3 b;
			mat_vec_mul(b, abs_pose_matrix[i], x);
			draw_set_color(0, 1, 0, 1);
			draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
		}
	}
}

void
draw_skeleton_3(mat4 *abs_pose_matrix, int *parent, int count)
{
	vec3 x = { 0.1, 0, 0 };
	int i;
	for (i = 0; i < count; i++) {
		float *a = abs_pose_matrix[i];
		if (parent[i] >= 0) {
			float *b = abs_pose_matrix[parent[i]];
			draw_set_color(1, 0, 1, 1);
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_set_color(0, 0, 1, 1);
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
		if (!haschildren(parent, count, i)) {
			vec3 b;
			mat_vec_mul(b, abs_pose_matrix[i], x);
			draw_set_color(0, 0, 1, 1);
			draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

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
		bone_uni_skin_matrix = glGetUniformLocation(bone_prog, "BoneMatrix");
	}

	glUseProgram(bone_prog);
	glUniformMatrix4fv(bone_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(bone_uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(bone_uni_skin_matrix, model->bone_count, 0, skin_matrix[0]);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	for (i = 0; i < model->mesh_count; i++) {
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].texture);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
