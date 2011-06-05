#include "mio.h"

#define noFLIP
#define NAMELEN 80

struct material {
	char name[NAMELEN];
	int texture;
};

struct pose {
	float translate[3];
	float rotate[4];
	float scale[3];
};

struct bone {
	char name[NAMELEN];
	int parent;
	struct pose bind_pose;
	float bind_matrix[4][4];
	float inv_bind_matrix[4][4];
};

struct anim {
	char name[NAMELEN];
	int first, count;
	float rate;
	int loop;
};

struct mesh {
	char name[NAMELEN];
	int material; // struct material *material;
	int first, count;
};

struct model {
	char dir[NAMELEN];
	int num_verts, num_tris, num_meshes, num_bones, num_frames, num_anims;
	float *pos, *norm, *texcoord;
	unsigned char *blend_index, *blend_weight, *color;
	int *tris;
	struct mesh *meshes;
	struct bone *bones;
	struct pose **poses; // poses for each frame
	struct anim *anims;

	float *outpos, *outnorm;
	float (*outbone)[4][4];
	float (*outskin)[4][4];
	struct pose *outpose;
};

static void float44_from_quat_vec(float m[4][4], float *q, float *v)
{
	float x2 = q[0] + q[0];
	float y2 = q[1] + q[1];
	float z2 = q[2] + q[2];
	{
		float xx2 = q[0] * x2;
		float yy2 = q[1] * y2;
		float zz2 = q[2] * z2;
		m[0][0] = 1 - yy2 - zz2;
		m[1][1] = 1 - xx2 - zz2;
		m[2][2] = 1 - xx2 - yy2;
	}
	{
		float yz2 = q[1] * z2;
		float wx2 = q[3] * x2;
		m[2][1] = yz2 + wx2;
		m[1][2] = yz2 - wx2;
	}
	{
		float xy2 = q[0] * y2;
		float wz2 = q[3] * z2;
		m[1][0] = xy2 + wz2;
		m[0][1] = xy2 - wz2;
	}
	{
		float xz2 = q[0] * z2;
		float wy2 = q[3] * y2;
		m[0][2] = xz2 + wy2;
		m[2][0] = xz2 - wy2;
	}

	m[0][3] = v[0];
	m[1][3] = v[1];
	m[2][3] = v[2];

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
}

static void float44_vec_mul(float *out, float m[4][4], float *v)
{
	out[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3];
	out[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3];
	out[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3];
}

static void float44_vec_mul_n(float *out, float m[4][4], float *v)
{
	out[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
	out[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
	out[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
}

static void float16_inverse(float out[16], const float m[16])
{
	float inv[16], det;
	int i;

	inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] +
		m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] -
		m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] +
		m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] -
		m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] -
		m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] +
		m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] -
		m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] +
		m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] +
		m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
	inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] -
		m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
	inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] +
		m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
	inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] -
		m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
	inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] -
		m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
	inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] +
		m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
	inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] -
		m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
	inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] +
		m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	assert (det != 0);
	det = 1.0 / det;
	for (i = 0; i < 16; i++)
		out[i] = inv[i] * det;
}

static void float44_inverse(float out[4][4], float mat[4][4])
{
	float16_inverse((float*)out, (float*)mat);
}

static void float44_mul(float out[4][4], float mat1[4][4], float mat2[4][4])
{
	float temp1[4][4], temp2[4][4];
	int i, j;
	if (mat1 == out) { memcpy(temp1, mat1, sizeof(temp1)); mat1 = temp1; }
	if (mat2 == out) { memcpy(temp2, mat2, sizeof(temp2)); mat2 = temp2; }
	for (j = 0; j < 4; ++j)
		for (i = 0; i < 4; ++i)
			out[j][i] =
				mat1[0][i] * mat2[j][0] +
				mat1[1][i] * mat2[j][1] +
				mat1[2][i] * mat2[j][2] +
				mat1[3][i] * mat2[j][3];
}

static int
load_material(struct model *model, char *name)
{
	char buf[256], *p;
	strlcpy(buf, model->dir, sizeof buf);
	strlcat(buf, name, sizeof buf);
	p = strrchr(buf, ',');
	if (p) strlcpy(p, ".png", sizeof buf - (p-buf));
	return load_texture(0, buf);
}

#define SEP " \t\r\n"

static void
count_iqe_data(struct model *model, FILE *file)
{
	char line[200], *s;
	int has_texcoord, has_normal, has_blend, has_color;
	int i;

	has_texcoord = has_normal = has_blend = has_color = 0;

	while (1) {
		if (!fgets(line, sizeof line, file))
			break;
		s = strtok(line, SEP);
		if (!s) continue;
		switch (s[0]) {
		case 'v':
			if (s[1] == 'p') model->num_verts++;
			else if (s[1] == 't') has_texcoord++;
			else if (s[1] == 'n') has_normal++;
			else if (s[1] == 'b') has_blend++;
			else if (s[1] == 'c') has_color++;
			break;
		case 'f':
			if (s[1] == 'a' || s[1] == 'm') model->num_tris++;
			else if (!strcmp(s, "frame")) model->num_frames++;
			break;
		case 'j': if (!strcmp(s, "joint")) model->num_bones++; break;
		case 'm': if (!strcmp(s, "mesh")) model->num_meshes++; break;
		case 'a': if (!strcmp(s, "animation")) model->num_anims++; break;
		}
	}

	if (model->num_verts) {
		model->pos = malloc(model->num_verts * 3 * sizeof(float));
		if (has_texcoord) model->texcoord = malloc(model->num_verts * 2 * sizeof(float));
		if (has_normal) model->norm = malloc(model->num_verts * 3 * sizeof(float));
		if (has_blend) {
			model->blend_index = malloc(model->num_verts * 4);
			model->blend_weight = malloc(model->num_verts * 4);
		}
		if (has_color) model->color = malloc(model->num_verts * 4);
	}
	if (model->num_tris) model->tris = malloc(model->num_tris * 3 * sizeof(int));
	if (model->num_meshes) model->meshes = malloc(model->num_meshes * sizeof(struct mesh));
	if (model->num_bones) model->bones = malloc(model->num_bones * sizeof(struct bone));
	if (model->num_anims) model->anims = malloc(model->num_anims * sizeof(struct anim));
	if (model->num_frames) {
		model->poses = malloc(model->num_frames * sizeof(struct pose*));
		for (i = 0; i < model->num_frames; i++)
			model->poses[i] = malloc(model->num_bones * sizeof(struct pose));
	}
}

static void
load_iqe_data(struct model *model, FILE *file)
{
	char line[200], buf[200], *s;
	int cur_position, cur_texcoord, cur_normal, cur_blend, cur_color;
	int cur_tri, cur_mesh, cur_anim, cur_frame, cur_bone, cur_pose;
	int meshvert = 0;
	int b[4];
	float w[4];
	int i;

	cur_position = cur_texcoord = cur_normal = cur_blend = cur_color = 0;
	cur_tri = cur_bone = cur_pose = 0;
	cur_mesh = cur_anim = cur_frame = -1;

	while (1) {
		if (!fgets(line, sizeof line, file))
			break;
		s = line;
		switch (s[0]) {
		case 'v':
			switch (s[1]) {
			case 'p':
				sscanf(line, "vp %f %f %f", model->pos+cur_position, model->pos+cur_position+1, model->pos+cur_position+2);
				cur_position += 3;
				break;
			case 't':
				sscanf(line, "vt %f %f", model->texcoord+cur_texcoord, model->texcoord+cur_texcoord+1);
				cur_texcoord += 2;
				break;
			case 'n':
				sscanf(line, "vn %f %f %f", model->norm+cur_normal, model->norm+cur_normal+1, model->norm+cur_normal+2);
#ifdef FLIP
				model->norm[cur_normal] = -model->norm[cur_normal];
				model->norm[cur_normal+1] = -model->norm[cur_normal+1];
				model->norm[cur_normal+2] = -model->norm[cur_normal+2];
#endif
				cur_normal += 3;
				break;
			case 'b':
				b[0] = b[1] = b[2] = b[3] = 0;
				w[0] = 1; w[1] = w[2] = w[3] = 0;
				sscanf(line, "vb %d %f %d %f %d %f %d %f", b+0, w+0, b+1, w+1, b+2, w+2, b+3, w+3);
				for (i = 0; i < 4; i++) {
					model->blend_index[cur_blend+i] = b[i];
					model->blend_weight[cur_blend+i] = w[i] * 255;
				}
				cur_blend += 4;
				break;
			case 'c':
				w[0] = w[1] = w[2] = w[3] = 0;
				sscanf(line, "vc %f %f %f %f", w+0, w+1, w+2, w+3);
				for (i = 0; i < 4; i++)
					model->color[cur_color+i] = w[i] * 255;
				cur_color += 4;
				break;
			}
			break;
		case 'f':
			if (s[1] == 'a') {
				sscanf(line, "fa %d %d %d", b+0, b+1, b+2);
#ifdef FLIP
				model->tris[cur_tri+0] = b[0];
				model->tris[cur_tri+1] = b[2];
				model->tris[cur_tri+2] = b[1];
#else
				model->tris[cur_tri+0] = b[0];
				model->tris[cur_tri+1] = b[1];
				model->tris[cur_tri+2] = b[2];
#endif
				cur_tri += 3;
			} else if (s[1] == 'm') {
				sscanf(line, "fm %d %d %d", b+0, b+1, b+2);
#ifdef FLIP
				model->tris[cur_tri+0] = b[0] + meshvert;
				model->tris[cur_tri+1] = b[2] + meshvert;
				model->tris[cur_tri+2] = b[1] + meshvert;
#else
				model->tris[cur_tri+0] = b[0] + meshvert;
				model->tris[cur_tri+1] = b[1] + meshvert;
				model->tris[cur_tri+2] = b[2] + meshvert;
#endif
				cur_tri += 3;
			} else if (strstr(s, "frame\n") == s) { // don't confuse with "framerate"
				cur_frame++;
				cur_pose = 0;
			}
			break;
		case 'j':
			if (strstr(s, "joint") == s) {
				sscanf(line, "joint %s %d", model->bones[cur_bone].name, &model->bones[cur_bone].parent);
				cur_bone++;
			}
			break;
		case 'm':
			if (strstr(s, "mesh") == s) {
				if (cur_mesh >= 0)
					model->meshes[cur_mesh].count = cur_tri/3 - model->meshes[cur_mesh].first;
				cur_mesh++;
				sscanf(line, "mesh %s", model->meshes[cur_mesh].name);
				model->meshes[cur_mesh].first = cur_tri/3;
				meshvert = cur_position / 3;
			} else if (strstr(s, "material") == s) {
				sscanf(line, "material %s", buf);
				model->meshes[cur_mesh].material = load_material(model, buf);
			}
			break;
		case 'a':
			if (strstr(s, "animation") == s) {
				if (cur_anim >= 0) {
					if (cur_pose > 0) { cur_pose = 0; cur_frame++; }
					model->anims[cur_anim].count = cur_frame - model->anims[cur_anim].first;
				}
				cur_anim++;
				sscanf(line, "animation %s", model->anims[cur_anim].name);
				model->anims[cur_anim].first = 0;
			}
			break;
		case 'p':
			if (s[1] == 'q') {
				struct pose *pose;
				if (cur_anim < 0)
					pose = &model->bones[cur_pose].bind_pose;
				else
					pose = &model->poses[cur_frame][cur_pose];
				i = sscanf(line, "pq %f %f %f %f %f %f %f %*f %*f %*f",
						&pose->translate[0], &pose->translate[1], &pose->translate[2],
						&pose->rotate[0], &pose->rotate[1], &pose->rotate[2], &pose->rotate[3]);
				cur_pose++;
			}
			break;
		}
	}

	if (cur_anim >= 0) {
		if (cur_pose > 0) { cur_pose = 0; cur_frame++; }
		model->anims[cur_anim].count = cur_frame - model->anims[cur_anim].first;
	}
	if (cur_mesh >= 0)
		model->meshes[cur_mesh].count = cur_tri/3 - model->meshes[cur_mesh].first;

	// compute inverse bind matrix
	for (i = 0; i < model->num_bones; i++) {
		struct bone *bone = model->bones + i;
		float44_from_quat_vec(bone->bind_matrix, bone->bind_pose.rotate, bone->bind_pose.translate);
		if (bone->parent >= 0) {
			struct bone *parent = model->bones + bone->parent;
			float44_mul(bone->bind_matrix, bone->bind_matrix, parent->bind_matrix);
		}
		float44_inverse(bone->inv_bind_matrix, bone->bind_matrix); // must be orthogonal
	}
}

struct model *
load_iqe_model(char *filename)
{
	struct model *model;
	FILE *file;
	char *p;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "cannot open model '%s'\n", filename);
		return NULL;
	}

	printf("loading iqe model '%s'\n", filename);

	model = malloc(sizeof *model);
	memset(model, 0, sizeof *model);

	strlcpy(model->dir, filename, NAMELEN);
	p = strrchr(model->dir, '/');
	if (!p) p = strrchr(model->dir, '\\');
	if (p) p[1] = 0; else strlcpy(model->dir, "./", NAMELEN);

	count_iqe_data(model, file);
	fseek(file, 0, 0);
	load_iqe_data(model, file);
	fclose(file);

	model->outpos = model->pos;
	model->outnorm = model->norm;
	model->outbone = NULL;
	model->outskin = NULL;

	return model;
}

void
animate_iqe_model(struct model *model, int anim, int frame)
{
	struct pose *poses;
	float *dstpos, *dstnorm, *srcpos, *srcnorm;
	unsigned char *idx, *w;
	int i, k, n;

	if (!model->num_bones || !model->num_anims)
		return;

	if (model->outpos == model->pos)
		model->outpos = malloc(model->num_verts * 3 * sizeof(float));
	if (model->outnorm == model->outnorm)
		model->outnorm = malloc(model->num_verts * 3 * sizeof(float));
	if (!model->outbone) {
		model->outbone = malloc(model->num_bones * sizeof(float[4][4]));
		model->outskin = malloc(model->num_bones * sizeof(float[4][4]));
	}

	frame %= model->anims[anim].count;
	poses = model->poses[model->anims[anim].first + frame];

	for (i = 0; i < model->num_bones; i++) {
		int parent = model->bones[i].parent;
		struct pose *pose = poses + i;
		float44_from_quat_vec(model->outbone[i], pose->rotate, pose->translate);
		if (parent >= 0)
			float44_mul(model->outbone[i], model->outbone[i], model->outbone[parent]);
		float44_mul(model->outskin[i], model->bones[i].inv_bind_matrix, model->outbone[i]);
	}

	idx = model->blend_index;
	w = model->blend_weight;
	srcpos = model->pos;
	srcnorm = model->norm;
	dstpos = model->outpos;
	dstnorm = model->outnorm;

	for (i = 0; i < model->num_verts; i++) {
		for (n = 0; n < 3; n++) {
			dstpos[n] = 0;
			dstnorm[n] = 0;
		}
		for (k = 0; k < 4; k++) {
			float (*m)[4] = model->outskin[idx[k]];
			if (w[k] > 0) {
				float pv[3], nv[3];
				float44_vec_mul(pv, m, srcpos);
				float44_vec_mul_n(nv, m, srcnorm);
				for (n = 0; n < 3; n++) {
					dstpos[n] += pv[n] * w[k];
					dstnorm[n] += nv[n] * w[k];
				}
			}
		}
		for (n = 0; n < 3; n++) {
			dstpos[n] *= 1/255.0f;
			dstnorm[n] *= 1/255.0f;
		}
		srcpos+=3; srcnorm+=3;
		dstpos+=3; dstnorm+=3;
		idx += 4; w += 4;
	}
}

void
draw_iqe_model(struct model *model)
{
	int i;

	glVertexPointer(3, GL_FLOAT, 0, model->outpos);
	if (model->norm) glNormalPointer(GL_FLOAT, 0, model->outnorm);
	if (model->texcoord) glTexCoordPointer(2, GL_FLOAT, 0, model->texcoord);
	// if (model->color) glColorPointer(4, GL_BYTE, 0, model->color);

	glEnableClientState(GL_VERTEX_ARRAY);
	if (model->norm) glEnableClientState(GL_NORMAL_ARRAY);
	if (model->texcoord) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// if (model->color) glEnableClientState(GL_COLOR_ARRAY);

	for (i = 0; i < model->num_meshes; i++) {
		struct mesh *mesh = model->meshes + i;
		glBindTexture(GL_TEXTURE_2D, mesh->material);
		glDrawElements(GL_TRIANGLES, 3 * mesh->count,
			GL_UNSIGNED_INT, &model->tris[mesh->first*3]);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	if (model->norm) glDisableClientState(GL_NORMAL_ARRAY);
	if (model->texcoord) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// if (model->color) glDisableClientState(GL_COLOR_ARRAY);

	glColor3f(1,1,1);
}

void
draw_iqe_bones(struct model *model)
{
	int i;

	glBegin(GL_LINES);

	// bind pose in green
	for (i = 0; i < model->num_bones; i++) {
		struct bone *bone = model->bones + i;
		if (bone->parent >= 0) {
			struct bone *pb = model->bones + bone->parent;
			glColor3f(0, 1, 0);
			glVertex3f(pb->bind_matrix[0][3], pb->bind_matrix[1][3], pb->bind_matrix[2][3]);
		} else {
			glColor3f(0, 1, 0.5);
			glVertex3f(0, 0, 0);
		}
		glVertex3f(bone->bind_matrix[0][3], bone->bind_matrix[1][3], bone->bind_matrix[2][3]);
	}

	// current pose in red
	if (model->outbone) {
		for (i = 0; i < model->num_bones; i++) {
			struct bone *bone = model->bones + i;
			if (bone->parent >= 0) {
				float (*p)[4] = model->outbone[bone->parent];
				glColor3f(1, 0, 0);
				glVertex3f(p[0][3], p[1][3], p[2][3]);
			} else {
				glColor3f(1, 0.5, 0);
				glVertex3f(0, 0, 0);
			}
			float (*m)[4] = model->outbone[i];
			glVertex3f(m[0][3], m[1][3], m[2][3]);
		}
	}

	glEnd();
}
