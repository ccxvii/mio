#include "mio.h"

#include <stdint.h>

// Assumes little-endian!

#define IQM_MAGIC "INTERQUAKEMODEL\0"
#define IQM_VERSION 2

#define noFLIP

enum {
	IQM_POSITION = 0,
	IQM_TEXCOORD = 1,
	IQM_NORMAL = 2,
	IQM_TANGENT = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR = 6,
	IQM_CUSTOM = 0x10
};

enum {
	IQM_BYTE = 0,
	IQM_UBYTE = 1,
	IQM_SHORT = 2,
	IQM_USHORT = 3,
	IQM_INT = 4,
	IQM_UINT = 5,
	IQM_HALF = 6,
	IQM_FLOAT = 7,
	IQM_DOUBLE = 8,
};

struct iqm_header
{
	char magic[16]; // the string "INTERQUAKEMODEL\0", 0 terminated
	uint32_t version; // must be version 2
	uint32_t filesize;
	uint32_t flags;
	uint32_t num_text, ofs_text;
	uint32_t num_meshes, ofs_meshes;
	uint32_t num_vertexarrays, num_vertexes, ofs_vertexarrays;
	uint32_t num_triangles, ofs_triangles, ofs_adjacency;
	uint32_t num_joints, ofs_joints;
	uint32_t num_poses, ofs_poses;
	uint32_t num_anims, ofs_anims;
	uint32_t num_frames, num_framechannels, ofs_frames, ofs_bounds;
	uint32_t num_comment, ofs_comment;
	uint32_t num_extensions, ofs_extensions; // these are stored as a linked list, not as a contiguous array
};

struct iqm_mesh
{
	uint32_t name;		// unique name for the mesh, if desired
	uint32_t material;	// set to a name of a non-unique material or texture
	uint32_t first_vertex, num_vertexes;
	uint32_t first_triangle, num_triangles;
};

struct iqm_vertexarray
{
	uint32_t type;		// type or custom name
	uint32_t flags;
	uint32_t format;	// component format
	uint32_t size;		// number of components
	uint32_t offset;	// offset to array of tightly packed components, with num_vertexes * size total entries
				// offset must be aligned to max(sizeof(format), 4)
};

struct iqm_joint
{
	uint32_t name;
	int32_t parent; // parent < 0 means this is a root bone
	float translate[3], rotate[4], scale[3]; 
	// translate is translation <Tx, Ty, Tz>, and rotate is quaternion rotation <Qx, Qy, Qz, Qw>
	// rotation is in relative/parent local space
	// scale is pre-scaling <Sx, Sy, Sz>
	// output = (input*scale)*rotation + translation
};

struct iqm_pose
{
	int32_t parent; // parent < 0 means this is a root bone
	uint32_t channelmask; // mask of which 10 channels are present for this joint pose
	float channeloffset[10], channelscale[10]; 
	// channels 0..2 are translation <Tx, Ty, Tz> and channels 3..6 are quaternion rotation <Qx, Qy, Qz, Qw>
	// rotation is in relative/parent local space
	// channels 7..9 are scale <Sx, Sy, Sz>
	// output = (input*scale)*rotation + translation
};

static void die(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

static int use_vertex_array(int format)
{
		switch (format) {
		case IQM_POSITION: return 1;
		case IQM_TEXCOORD: return 1;
		case IQM_NORMAL: return 1;
		case IQM_TANGENT: return 0;
		case IQM_BLENDINDEXES: return 1;
		case IQM_BLENDWEIGHTS: return 1;
		case IQM_COLOR: return 0;
		}
		return 0;
}

static int size_of_format(int format)
{
	switch (format) {
	case IQM_BYTE: return 1;
	case IQM_UBYTE: return 1;
	case IQM_SHORT: return 2;
	case IQM_USHORT: return 2;
	case IQM_INT: return 4;
	case IQM_UINT: return 4;
	case IQM_HALF: return 2;
	case IQM_FLOAT: return 4;
	case IQM_DOUBLE: return 8;
	}
	return 0;
}

static int enum_of_format(int format)
{
	switch (format) {
	case IQM_BYTE: return GL_BYTE;
	case IQM_UBYTE: return GL_UNSIGNED_BYTE;
	case IQM_SHORT: return GL_SHORT;
	case IQM_USHORT: return GL_UNSIGNED_SHORT;
	case IQM_INT: return GL_INT;
	case IQM_UINT: return GL_UNSIGNED_INT;
	case IQM_HALF: return GL_HALF_FLOAT;
	case IQM_FLOAT: return GL_FLOAT;
	case IQM_DOUBLE: return GL_DOUBLE;
	}
	return 0;
}

static void flip_normals(float *p, int count)
{
#ifdef FLIP
	count = count * 3;
	while (count--) { *p = -*p; p++; }
#endif
}

static void flip_triangles(uint16_t *dst, uint32_t *src, int count)
{
	while (count--) {
#ifdef FLIP
		dst[0] = src[0];
		dst[1] = src[2];
		dst[2] = src[1];
#else
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
#endif
		dst += 3;
		src += 3;
	}
}

static int load_iqm_material(char *dir, char *name)
{
	char buf[256], *p;
	strlcpy(buf, dir, sizeof buf);
	strlcat(buf, name, sizeof buf);
	p = strrchr(buf, ',');
	if (p) strlcpy(p, ".png", sizeof buf - (p-buf));
	return load_texture(0, buf, 1);
}

struct model *load_iqm_model_from_memory(char *filename, unsigned char *data, int len)
{
	struct iqm_header *header = (void*) data;
	char *text = (void*) &data[header->ofs_text];
	struct iqm_mesh *meshes = (void*) &data[header->ofs_meshes];
	struct iqm_vertexarray *vertexarrays = (void*) &data[header->ofs_vertexarrays];
	struct iqm_joint *joints = (void*) &data[header->ofs_joints];
	// struct iqm_pose *poses = (void*) &data[header->ofs_poses];
	int i, total;
	char *p;
	mat4 local_matrix, world_matrix[MAXBONE];
	char dir[256];

	strlcpy(dir, filename, sizeof dir);
	p = strrchr(dir, '/');
	if (!p) p = strrchr(dir, '\\');
	if (p) p[1] = 0; else strlcpy(dir, "./", sizeof dir);

	if (memcmp(header->magic, IQM_MAGIC, 16))
		die("bad iqm magic");
	if (header->version != IQM_VERSION)
		die("bad iqm version");
	if (header->filesize > len)
		die("bad iqm file size");
	if (header->num_vertexes > 0xffff)
		die("too many vertices in iqm");
	if (header->num_joints > MAXBONE)
		die("too many joints in iqm");
	if (header->num_joints != header->num_poses)
		die("bad joint/pose data");

	struct model *model = malloc(sizeof *model);
	model->mesh_count = header->num_meshes;
	model->bone_count = header->num_joints;

	model->mesh = malloc(model->mesh_count * sizeof *model->mesh);
	model->bone = malloc(model->bone_count * sizeof *model->bone);
	model->bind_pose = malloc(model->bone_count * sizeof *model->bind_pose);

	for (i = 0; i < model->mesh_count; i++) {
		model->mesh[i].texture = load_iqm_material(dir, text + meshes[i].material);
		model->mesh[i].first = meshes[i].first_triangle * 3;
		model->mesh[i].count = meshes[i].num_triangles * 3;
	}

	for (i = 0; i < model->bone_count; i++) {
		struct bone *bone = model->bone + i;
		struct pose *pose = model->bind_pose + i;
		strlcpy(bone->name, text + joints[i].name, sizeof bone->name);
		bone->parent = joints[i].parent;
		memcpy(pose->translate, joints[i].translate, 3*4);
		memcpy(pose->rotate, joints[i].rotate, 4*4);
		memcpy(pose->scale, joints[i].scale, 3*4);
		if (bone->parent >= 0) {
			mat_from_pose(local_matrix, pose->translate, pose->rotate, pose->scale);
			mat_mul(world_matrix[i], world_matrix[bone->parent], local_matrix);
		} else {
			mat_from_pose(world_matrix[i], pose->translate, pose->rotate, pose->scale);
		}
		mat_invert(bone->inv_bind_matrix, world_matrix[i]);
	}

	glGenVertexArrays(1, &model->vao);
	glGenBuffers(1, &model->vbo);
	glGenBuffers(1, &model->ibo);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	total = 0;
	for (i = 0; i < header->num_vertexarrays; i++) {
		struct iqm_vertexarray *va = vertexarrays + i;
		if (use_vertex_array(va->type)) {
				total += size_of_format(va->format) * va->size * header->num_vertexes;
		}
	}

	glBufferData(GL_ARRAY_BUFFER, total, NULL, GL_STATIC_DRAW);

	total = 0;
	for (i = 0; i < header->num_vertexarrays; i++) {
		struct iqm_vertexarray *va = vertexarrays + i;
		if (use_vertex_array(va->type)) {
				int current = size_of_format(va->format) * va->size * header->num_vertexes;
				int format = enum_of_format(va->format);
				int normalize = va->type != IQM_BLENDINDEXES;
				if (va->type == IQM_NORMAL && va->format == IQM_FLOAT && va->size == 3)
						flip_normals((float*)&data[va->offset], header->num_vertexes);
				glBufferSubData(GL_ARRAY_BUFFER, total, current, &data[va->offset]);
				glEnableVertexAttribArray(va->type);
				glVertexAttribPointer(va->type, va->size, format, normalize, 0, (void*)total);
				total += current;
		}
	}

	uint16_t *triangles = malloc(header->num_triangles * 3*2);
	flip_triangles(triangles, (void*)&data[header->ofs_triangles], header->num_triangles);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, header->num_triangles * 3*2, triangles, GL_STATIC_DRAW);
	free(triangles);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return model;
}

struct model *load_iqm_model(char *filename)
{
	struct model *model;
	unsigned char *data;
	int len;
	data = load_file(filename, &len);
	if (!data) return NULL;
	model = load_iqm_model_from_memory(filename, data, len);
	free(data);
	return model;
}

void pose_model(mat4 *out_matrix, struct bone *bone, struct pose *pose, int count)
{
	int i;
	mat4 local_matrix, world_matrix[MAXBONE];
	for (i = 0; i < count; i++) {
		if (bone[i].parent >= 0) {
			mat_from_pose(local_matrix, pose[i].translate, pose[i].rotate, pose[i].scale);
			mat_mul(world_matrix[i], world_matrix[bone->parent], local_matrix);
		} else {
			mat_from_pose(world_matrix[i], pose[i].translate, pose[i].rotate, pose[i].scale);
		}
		mat_mul(out_matrix[i], world_matrix[i], bone[i].inv_bind_matrix);
	}
}

static const char *static_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"attribute vec3 att_Position;\n"
	"attribute vec2 att_TexCoord;\n"
	"attribute vec3 att_Normal;\n"
	"varying vec3 var_Position;\n"
	"varying vec2 var_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"void main() {\n"
	"	vec4 position = ModelView * vec4(att_Position, 1.0);\n"
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
	"attribute vec3 att_Position;\n"
	"attribute vec2 att_TexCoord;\n"
	"attribute vec3 att_Normal;\n"
	"attribute vec4 att_BlendIndex;\n"
	"attribute vec4 att_BlendWeight;\n"
	"varying vec3 var_Position;\n"
	"varying vec2 var_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"void main() {\n"
	"	vec4 position = vec4(0);\n"
	"	vec4 normal = vec4(0);\n"
	"	vec4 index = att_BlendIndex;\n"
	"	vec4 weight = att_BlendWeight;\n"
	"	for (int i = 0; i < 4; i++) {\n"
	"		position += BoneMatrix[int(index.x)] * vec4(att_Position, 1) * weight.x;\n"
	"		normal += BoneMatrix[int(index.x)] * vec4(att_Normal, 0) * weight.x;\n"
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
	"uniform vec3 LightDirection = vec3(0.0, 0.0, 1.0);\n"
	"uniform vec3 LightAmbient = vec3(0.2, 0.2, 0.2);\n"
	"uniform vec3 LightDiffuse = vec3(0.8, 0.8, 0.8);\n"
	"varying vec3 var_Position;\n"
	"varying vec2 var_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"void main() {\n"
	"	vec4 color = texture2D(Texture, var_TexCoord);\n"
	"	vec3 N = normalize(gl_FrontFacing ? var_Normal : -var_Normal);\n"
	"	float diffuse = max(dot(N, LightDirection), 0.0);\n"
	"	vec3 Ka = color.rgb * LightAmbient;\n"
	"	vec3 Kd = color.rgb * LightDiffuse * diffuse;\n"
	"	gl_FragColor = vec4(Ka + Kd, 1.0);\n"
	"}\n"
;

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
static int bone_uni_bone_matrix;

void draw_model_with_pose(struct model *model, mat4 projection, mat4 model_view, mat4 *bone_matrix)
{
	int i;

	if (!model)
		return;

	if (!bone_prog) {
		bone_prog = compile_shader(bone_vert_src, model_frag_src);
		bone_uni_projection = glGetUniformLocation(bone_prog, "Projection");
		bone_uni_model_view = glGetUniformLocation(bone_prog, "ModelView");
		bone_uni_bone_matrix = glGetUniformLocation(bone_prog, "BoneMatrix");
	}

	glUseProgram(bone_prog);
	glUniformMatrix4fv(bone_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(bone_uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(bone_uni_bone_matrix, model->bone_count, 0, bone_matrix[0]);

	glBindVertexArray(model->vao);

	for (i = 0; i < model->mesh_count; i++) {
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].texture);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}

#if 0

struct model *
load_iqm_model(char *filename)
{
	struct model *model;
	unsigned char *data;
	unsigned char hdr[16+27*4];
	int filesize;
	FILE *file;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "cannot open model '%s'\n", filename);
		return NULL;
	}

	printf("loading iqm model '%s'\n", filename);

	fread(&hdr, 1, sizeof hdr, file);
	if (memcmp(hdr, IQM_MAGIC, 16)) {
		fprintf(stderr, "not an IQM file '%s'\n", filename);
		fclose(file);
		return NULL;
	}
	if (read32(hdr+16) != IQM_VERSION) {
		fprintf(stderr, "unknown IQM version '%s'\n", filename);
		fclose(file);
		return NULL;
	}

	filesize = read32(hdr+20);
	data = malloc(filesize);
	memcpy(data, hdr, sizeof hdr);
	fread(data + sizeof hdr, 1, filesize - sizeof hdr, file);
	fclose(file);
	model = load_iqm_model_from_memory(data, filename);
	free(data);

	return model;
}

float measure_iqm_radius(struct model *model)
{
	return model->radius;
}

char *
get_iqm_animation_name(struct model *model, int anim)
{
	if (!model->num_bones || !model->num_anims)
		return "none";
	if (anim < 0) anim = 0;
	if (anim >= model->num_anims) anim = model->num_anims - 1;
	return model->anims[anim].name;
}

void
animate_iqm_model(struct model *model, int anim, int frame, float t)
{
	struct pose *pose0, *pose1;
	float m[16], q[4], v[3];
	int frame0, frame1;
	int i;

	if (!model->num_bones || !model->num_anims)
		return;

	if (!model->outbone) {
		model->outbone = malloc(model->num_bones * sizeof(float[16]));
		model->outskin = malloc(model->num_bones * sizeof(float[16]));
	}

	if (anim < 0) anim = 0;
	if (anim >= model->num_anims) anim = model->num_anims - 1;

	frame0 = frame % model->anims[anim].count;
	frame1 = (frame + 1) % model->anims[anim].count;
	pose0 = model->poses[model->anims[anim].first + frame0];
	pose1 = model->poses[model->anims[anim].first + frame1];

	for (i = 0; i < model->num_bones; i++) {
		int parent = model->bones[i].parent;
		quat_lerp_neighbor_normalize(q, pose0[i].rotate, pose1[i].rotate, t);
		vec_lerp(v, pose0[i].translate, pose1[i].translate, t);
		if (parent >= 0) {
			mat_from_pose(m, q, v);
			mat_mul(model->outbone[i], model->outbone[parent], m);
		} else {
			mat_from_pose(model->outbone[i], q, v);
		}
		mat_mul(model->outskin[i], model->outbone[i], model->bones[i].inv_bind_matrix);
	}
}

static void make_vbo(struct model *model)
{
	int norm_ofs = 0, texcoord_ofs = 0, color_ofs = 0, blend_index_ofs = 0, blend_weight_ofs = 0;
	int n = 12;
	if (model->norm) { norm_ofs = n; n += 12; }
	if (model->texcoord) { texcoord_ofs = n; n += 8; }
	if (model->color) { color_ofs = n; n += 4; }
	if (model->blend_index && model->blend_weight) {
		blend_index_ofs = n; n += 4;
		blend_weight_ofs = n; n += 4;
	}

	glGenBuffers(1, &model->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * model->num_verts, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * model->num_verts, model->pos);
	if (model->norm)
		glBufferSubData(GL_ARRAY_BUFFER, norm_ofs * model->num_verts, 12 * model->num_verts, model->norm);
	if (model->texcoord)
		glBufferSubData(GL_ARRAY_BUFFER, texcoord_ofs * model->num_verts, 8 * model->num_verts, model->texcoord);
	if (model->color)
		glBufferSubData(GL_ARRAY_BUFFER, color_ofs * model->num_verts, 4 * model->num_verts, model->color);
	if (model->blend_index && model->blend_weight) {
		glBufferSubData(GL_ARRAY_BUFFER, blend_index_ofs * model->num_verts, 4 * model->num_verts, model->blend_index);
		glBufferSubData(GL_ARRAY_BUFFER, blend_weight_ofs * model->num_verts, 4 * model->num_verts, model->blend_weight);
	}

	glGenBuffers(1, &model->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->num_tris * 3 * sizeof(int), model->tris, GL_STATIC_DRAW);
}

void
draw_iqm_instances(struct model *model, float *trafo, int count)
{
	int i, k, n;
	int prog, loc, bi_loc, bw_loc;

	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
	loc = glGetUniformLocation(prog, "BoneMatrix");
	bi_loc = glGetAttribLocation(prog, "in_BlendIndex");
	bw_loc = glGetAttribLocation(prog, "in_BlendWeight");

	if (!model->vbo)
		make_vbo(model);

	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(ATT_POSITION);
	n = 12 * model->num_verts;

	if (model->norm) {
		glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 0, (char*)n);
		glEnableVertexAttribArray(ATT_NORMAL);
		n += 12 * model->num_verts;
	}
	if (model->texcoord) {
		glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 0, (char*)n);
		glEnableVertexAttribArray(ATT_TEXCOORD);
		n += 8 * model->num_verts;
	}
	if (model->color) {
		glVertexAttribPointer(ATT_COLOR, 4, GL_UNSIGNED_BYTE, 1, 0, (char*)n);
		glEnableVertexAttribArray(ATT_COLOR);
		n += 4 * model->num_verts;
	}

	if (loc >= 0 && bi_loc >= 0 && bw_loc >= 0) {
		if (model->outskin && model->blend_index && model->blend_weight) {
			glUniformMatrix4fv(loc, model->num_bones, 0, model->outskin[0]);
			glVertexAttribPointer(bi_loc, 4, GL_UNSIGNED_BYTE, 0, 0, (char*)n);
			n += 4 * model->num_verts;
			glVertexAttribPointer(bw_loc, 4, GL_UNSIGNED_BYTE, 1, 0, (char*)n);
			n += 4 * model->num_verts;
			glEnableVertexAttribArray(bi_loc);
			glEnableVertexAttribArray(bw_loc);
		}
	}

	for (i = 0; i < model->num_meshes; i++) {
		struct mesh *mesh = model->meshes + i;
		glBindTexture(GL_TEXTURE_2D, mesh->material);
		for (k = 0; k < count; k++) {
			// dog slow! should use our own uniforms, or instanced array
//			glPushMatrix();
//			glMultMatrixf(&trafo[k*16]);
			glDrawElements(GL_TRIANGLES, 3 * mesh->count, GL_UNSIGNED_INT, (char*)(mesh->first*12));
//			glPopMatrix();
		}
	}

	glDisableVertexAttribArray(ATT_POSITION);
	glDisableVertexAttribArray(ATT_NORMAL);
	glDisableVertexAttribArray(ATT_TEXCOORD);
	glDisableVertexAttribArray(ATT_COLOR);
	if (loc >= 0 && bi_loc >= 0 && bw_loc >= 0) {
		glDisableVertexAttribArray(bi_loc);
		glDisableVertexAttribArray(bw_loc);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
draw_iqm_model(struct model *model)
{
	float transform[16];
	mat_identity(transform);
	draw_iqm_instances(model, transform, 1);
}

void
draw_iqm_bones(struct model *model)
{
#if 0
	int i;

	glBegin(GL_LINES);

	// bind pose in green
	for (i = 0; i < model->num_bones; i++) {
		struct bone *bone = model->bones + i;
		if (bone->parent >= 0) {
			struct bone *pb = model->bones + bone->parent;
			glColor3f(0, 1, 0);
			glVertex3f(pb->bind_matrix[12], pb->bind_matrix[13], pb->bind_matrix[14]);
		} else {
			glColor3f(0, 1, 0.5);
			glVertex3f(0, 0, 0);
		}
		glVertex3f(bone->bind_matrix[12], bone->bind_matrix[13], bone->bind_matrix[14]);
	}

	// current pose in red
	if (model->outbone) {
		for (i = 0; i < model->num_bones; i++) {
			struct bone *bone = model->bones + i;
			if (bone->parent >= 0) {
				float *p = model->outbone[bone->parent];
				glColor3f(1, 0, 0);
				glVertex3f(p[12], p[13], p[14]);
			} else {
				glColor3f(1, 0.5, 0);
				glVertex3f(0, 0, 0);
			}
			float *m = model->outbone[i];
			glVertex3f(m[12], m[13], m[14]);
		}
	}

	glEnd();
#endif
}
#endif
