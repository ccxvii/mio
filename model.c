#include "mio.h"

static struct cache *model_cache = NULL;
static struct cache *animation_cache = NULL;

struct model *load_model(char *filename)
{
	struct model *model;
	unsigned char *data;
	int len;

	model = lookup(model_cache, filename);
	if (model)
		return model;

	data = load_file(filename, &len);
	if (!data) {
		fprintf(stderr, "error: cannot load model file: '%s'\n", filename);
		return NULL;
	}

	model = NULL;
	if (strstr(filename, ".iqm")) model = load_iqm_model_from_memory(filename, data, len);
	if (strstr(filename, ".iqe")) model = load_iqe_model_from_memory(filename, data, len);
	if (strstr(filename, ".obj")) model = load_obj_model_from_memory(filename, data, len);
	if (!model)
		fprintf(stderr, "error: cannot load model: '%s'\n", filename);

	free(data);

	if (model)
		model_cache = insert(model_cache, filename, model);

	return model;
}

struct animation *load_animation(char *filename)
{
	struct animation *anim;
	unsigned char *data;
	int len;
	data = load_file(filename, &len);
	if (!data) {
		fprintf(stderr, "error: cannot load animation file: '%s'\n", filename);
		return NULL;
	}
	anim = load_iqm_animation_from_memory(filename, data, len);
	free(data);
	return anim;
}

static const char *static_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"attribute vec4 att_Position;\n"
	"attribute vec3 att_Normal;\n"
	"attribute vec2 att_TexCoord;\n"
	"varying vec3 var_Position;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 position = ModelView * att_Position;\n"
	"	vec4 normal = ModelView * vec4(att_Normal, 0.0);\n"
	"	gl_Position = Projection * position;\n"
	"	var_Position = position.xyz;\n"
	"	var_Normal = normal.xyz;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *bone_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"uniform mat4 BoneMatrix[80];\n"
	"attribute vec4 att_Position;\n"
	"attribute vec3 att_Normal;\n"
	"attribute vec2 att_TexCoord;\n"
	"attribute vec4 att_BlendIndex;\n"
	"attribute vec4 att_BlendWeight;\n"
	"varying vec3 var_Position;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 position = vec4(0);\n"
	"	vec4 normal = vec4(0);\n"
	"	vec4 index = att_BlendIndex;\n"
	"	vec4 weight = att_BlendWeight;\n"
	"	for (int i = 0; i < 4; i++) {\n"
	"		mat4 m = BoneMatrix[int(index.x)];\n"
	"		position += m * att_Position * weight.x;\n"
	"		normal += m * vec4(att_Normal, 0) * weight.x;\n"
	"		index = index.yzwx;\n"
	"		weight = weight.yzwx;\n"
	"	}\n"
	"	position = ModelView * position;\n"
	"	normal = ModelView * normal;\n"
	"	gl_Position = Projection * position;\n"
	"	var_Position = position.xyz;\n"
	"	var_Normal = normal.xyz;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *model_frag_src =
	"#version 120\n"
	"uniform sampler2D Texture;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"const vec3 LightDirection = vec3(-0.5773, 0.5773, 0.5773);\n"
	"const vec3 LightAmbient = vec3(0.1);\n"
	"const vec3 LightDiffuse = vec3(0.9);\n"
	"void main() {\n"
	"	vec4 albedo = texture2D(Texture, var_TexCoord);\n"
	"	vec3 N = normalize(var_Normal);\n"
	"	float diffuse = max(dot(N, LightDirection), 0.0);\n"
	"	vec3 Ka = albedo.rgb * LightAmbient;\n"
	"	vec3 Kd = albedo.rgb * LightDiffuse * diffuse;\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	gl_FragColor = vec4(Ka + Kd, albedo.a);\n"
	"}\n"
;

static const char *gloss_frag_src =
	"#version 120\n"
	"uniform sampler2D Texture;\n"
	"uniform sampler2D GlossMap;\n"
	"varying vec3 var_Position;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"const vec3 LightDirection = vec3(-0.5773, 0.5773, 0.5773);\n"
	"const vec3 LightAmbient = vec3(0.1);\n"
	"const vec3 LightDiffuse = vec3(0.9);\n"
	"const vec3 LightSpecular = vec3(1.0);\n"
	"const float Shininess = 64.0;\n"
	"void main() {\n"
	"	vec4 albedo = texture2D(Texture, var_TexCoord);\n"
	"	vec4 gloss = texture2D(GlossMap, var_TexCoord);\n"
	"	vec3 N = normalize(var_Normal);\n"
	"	vec3 V = normalize(-var_Position);\n"
	"	vec3 H = normalize(LightDirection + V);\n"
	"	float diffuse = max(dot(N, LightDirection), 0.0);\n"
	"	float specular = pow(max(dot(N, H), 0), Shininess);\n"
	"	vec3 Ka = albedo.rgb * LightAmbient;\n"
	"	vec3 Kd = albedo.rgb * LightDiffuse * diffuse;\n"
	"	vec3 Ks = LightSpecular * specular * gloss.r;\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	gl_FragColor = vec4(Ka + Kd + Ks, albedo.a);\n"
	"}\n"
;

static const char *ghost_frag_src =
	"#version 120\n"
	"uniform sampler2D Texture;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 albedo = texture2D(Texture, var_TexCoord);\n"
	"	if (albedo.a == 0.0) discard;\n"
	"	gl_FragColor = vec4(albedo.rgb, 1);\n"
	"}\n"
;

void calc_mul_matrix(mat4 *skin_matrix, mat4 *abs_pose_matrix, mat4 *inv_bind_matrix, int count)
{
	int i;
	for (i = 0; i < count; i++)
		mat_mul44(skin_matrix[i], abs_pose_matrix[i], inv_bind_matrix[i]);
}

void calc_inv_matrix(mat4 *inv_bind_matrix, mat4 *abs_bind_matrix, int count)
{
	int i;
	for (i = 0; i < count; i++)
		mat_invert(inv_bind_matrix[i], abs_bind_matrix[i]);
}

void calc_abs_matrix(mat4 *abs_pose_matrix, mat4 *pose_matrix, int *parent, int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (parent[i] >= 0)
			mat_mul44(abs_pose_matrix[i], abs_pose_matrix[parent[i]], pose_matrix[i]);
		else
			mat_copy(abs_pose_matrix[i], pose_matrix[i]);
}

void calc_matrix_from_pose(mat4 *pose_matrix, struct pose *pose, int count)
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

void fix_anim_skel(struct pose *animpose, struct pose *skelpose, char (*animnames)[32], char (*skelnames)[32], int animcount, int skelcount)
{
	int i, k;
	for (i = 0; i < skelcount; i++) {
		k = findbone(animnames, animcount, skelnames[i]);
		if (k >= 0) {
			animpose[k] = skelpose[i];
		}
	}
}

// Model = ModelBind * inv(AnimBind) * Anim
// Basis = inv(AnimBind) * Anim
void apply_animation_delta(mat4 *out_matrix,
		mat4 *dst_bind_matrix, char (*dst_names)[32], int dst_count,
		mat4 *src_bind_matrix, char (*src_names)[32], int src_count,
		mat4 *src_anim_matrix)
{
	mat4 basis, inv_src_bind_matrix;
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			mat_copy(out_matrix[dst], dst_bind_matrix[dst]);
		} else {
			mat_invert(inv_src_bind_matrix, src_bind_matrix[src]);
			mat_mul44(basis, inv_src_bind_matrix, src_anim_matrix[src]);
			mat_mul44(out_matrix[dst], dst_bind_matrix[dst], basis);
		}
	}
}

// Model = ModelBind * inv(AnimBind) * Anim
// Basis = inv(AnimBind) * Anim
void apply_animation_delta_q(struct pose *out_pose,
		struct pose *dst_bind_pose, char (*dst_names)[32], int dst_count,
		struct pose *src_bind_pose, char (*src_names)[32], int src_count,
		struct pose *src_anim_pose)
{
	struct pose basis;
	vec4 q;
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src < 0) {
			out_pose[dst] = dst_bind_pose[dst];
		} else {
			// basis = inv(animbind) * anim
			quat_conjugate(q, src_bind_pose[src].rotate);
			quat_mul(basis.rotate, q, src_anim_pose[src].rotate);
			vec_sub(basis.translate, src_anim_pose[src].translate, src_bind_pose[src].translate);
			vec_div(basis.scale, src_anim_pose[src].scale, src_bind_pose[src].scale);

			// out = bind * basis
			vec_add(out_pose[dst].translate, dst_bind_pose[dst].translate, basis.translate);
			quat_mul(out_pose[dst].rotate, dst_bind_pose[dst].rotate, basis.rotate);
			vec_mul(out_pose[dst].scale, dst_bind_pose[dst].scale, basis.scale);
		}
	}
}

void apply_animation_rot(
		struct pose *dst_pose, char (*dst_names)[32], int dst_count,
		struct pose *src_pose, char (*src_names)[32], int src_count)
{
	int src, dst;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src >= 0) {
			memcpy(dst_pose[dst].rotate, src_pose[src].rotate, 4*4);
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

static const char *ryzom_bones[] = {
	"bip01", "bip01_pelvis", "bip01_l_clavicle", "bip01_r_clavicle", "bip01_spine", "bip01_tail"
};

void apply_animation_ryzom(
	struct pose *dst_pose, char (*dst_names)[32], int dst_count,
	struct pose *src_pose, char (*src_names)[32], int src_count)
{
	int src, dst, i;
	for (dst = 0; dst < dst_count; dst++) {
		src = findbone(src_names, src_count, dst_names[dst]);
		if (src >= 0) {
			for (i = 0; i < nelem(ryzom_bones); i++) {
				if (!strcmp(dst_names[dst], ryzom_bones[i])) {
					dst_pose[dst] = src_pose[src];
					goto next;
				}
			}
			memcpy(dst_pose[dst].rotate, src_pose[src].rotate, 4*sizeof(float));
		}
next:;
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
draw_skeleton_with_axis(mat4 *abs_pose_matrix, int *parent, int count)
{
	vec3 x = { 0.1, 0, 0 };
	vec3 y = { 0, 0.1, 0 };
	vec3 z = { 0, 0, 0.1 };
	int i;
	for (i = 0; i < count; i++) {
		float *a = abs_pose_matrix[i];
		if (parent[i] >= 0) {
			float *b = abs_pose_matrix[parent[i]];
			draw_set_color(0.3, 0.3, 0.3, 1);
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_set_color(1, 1, 0, 1);
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
		vec3 b;
		mat_vec_mul(b, abs_pose_matrix[i], x);
		draw_set_color(1, 0, 0, 1);
		draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
		mat_vec_mul(b, abs_pose_matrix[i], y);
		draw_set_color(0, 1, 0, 1);
		draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
		mat_vec_mul(b, abs_pose_matrix[i], z);
		draw_set_color(0, 0, 1, 1);
		draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
	}
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

static int static_prog = 0;
static int static_uni_projection;
static int static_uni_model_view;

void draw_model(struct model *model, mat4 projection, mat4 model_view)
{
	int i;

	if (!model)
		return;

	if (model->mesh[0].ghost) {
		draw_model_ghost(model, projection, model_view);
		return;
	}

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

static int ghost_prog = 0;
static int ghost_uni_projection;
static int ghost_uni_model_view;

void draw_model_ghost(struct model *model, mat4 projection, mat4 model_view)
{
	int i;

	if (!model)
		return;

	if (!ghost_prog) {
		ghost_prog = compile_shader(static_vert_src, ghost_frag_src);
		ghost_uni_projection = glGetUniformLocation(ghost_prog, "Projection");
		ghost_uni_model_view = glGetUniformLocation(ghost_prog, "ModelView");
	}

	glUseProgram(ghost_prog);
	glUniformMatrix4fv(ghost_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(ghost_uni_model_view, 1, 0, model_view);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	for (i = 0; i < model->mesh_count; i++) {
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].texture);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);

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
