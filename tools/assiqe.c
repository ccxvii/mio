#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <assimp.h>
#include <aiMatrix4x4.h>
#include <aiMatrix3x3.h>
#include <aiColor4D.h>
#include <aiVector3D.h>
#include <aiPostProcess.h>
#include <aiScene.h>

#include "getopt.h"

#define EPSILON 0.00001
#define NEAR_0(x) (fabs((x)) < EPSILON)
#define NEAR_1(x) (NEAR_0((x)-1))
#define KILL_0(x) (NEAR_0((x)) ? 0 : (x))
#define KILL_N(x,n) (NEAR_0((x)-(n)) ? (n) : (x))
#define KILL(x) KILL_0(KILL_N(KILL_N(x, 1), -1))

float aiDeterminant(struct aiMatrix4x4 *m)
{
	return
		m->a1*m->b2*m->c3*m->d4 - m->a1*m->b2*m->c4*m->d3 +
		m->a1*m->b3*m->c4*m->d2 - m->a1*m->b3*m->c2*m->d4 +
		m->a1*m->b4*m->c2*m->d3 - m->a1*m->b4*m->c3*m->d2 -
		m->a2*m->b3*m->c4*m->d1 + m->a2*m->b3*m->c1*m->d4 -
		m->a2*m->b4*m->c1*m->d3 + m->a2*m->b4*m->c3*m->d1 -
		m->a2*m->b1*m->c3*m->d4 + m->a2*m->b1*m->c4*m->d3 +
		m->a3*m->b4*m->c1*m->d2 - m->a3*m->b4*m->c2*m->d1 +
		m->a3*m->b1*m->c2*m->d4 - m->a3*m->b1*m->c4*m->d2 +
		m->a3*m->b2*m->c4*m->d1 - m->a3*m->b2*m->c1*m->d4 -
		m->a4*m->b1*m->c2*m->d3 + m->a4*m->b1*m->c3*m->d2 -
		m->a4*m->b2*m->c3*m->d1 + m->a4*m->b2*m->c1*m->d3 -
		m->a4*m->b3*m->c1*m->d2 + m->a4*m->b3*m->c2*m->d1;
}

void aiInverseMatrix(struct aiMatrix4x4 *p, struct aiMatrix4x4 *m)
{
	float det = aiDeterminant(m);
	assert(det != 0.0f);
	float invdet = 1.0f / det;
	p->a1= invdet * (m->b2*(m->c3*m->d4-m->c4*m->d3) + m->b3*(m->c4*m->d2-m->c2*m->d4) + m->b4*(m->c2*m->d3-m->c3*m->d2));
	p->a2=-invdet * (m->a2*(m->c3*m->d4-m->c4*m->d3) + m->a3*(m->c4*m->d2-m->c2*m->d4) + m->a4*(m->c2*m->d3-m->c3*m->d2));
	p->a3= invdet * (m->a2*(m->b3*m->d4-m->b4*m->d3) + m->a3*(m->b4*m->d2-m->b2*m->d4) + m->a4*(m->b2*m->d3-m->b3*m->d2));
	p->a4=-invdet * (m->a2*(m->b3*m->c4-m->b4*m->c3) + m->a3*(m->b4*m->c2-m->b2*m->c4) + m->a4*(m->b2*m->c3-m->b3*m->c2));
	p->b1=-invdet * (m->b1*(m->c3*m->d4-m->c4*m->d3) + m->b3*(m->c4*m->d1-m->c1*m->d4) + m->b4*(m->c1*m->d3-m->c3*m->d1));
	p->b2= invdet * (m->a1*(m->c3*m->d4-m->c4*m->d3) + m->a3*(m->c4*m->d1-m->c1*m->d4) + m->a4*(m->c1*m->d3-m->c3*m->d1));
	p->b3=-invdet * (m->a1*(m->b3*m->d4-m->b4*m->d3) + m->a3*(m->b4*m->d1-m->b1*m->d4) + m->a4*(m->b1*m->d3-m->b3*m->d1));
	p->b4= invdet * (m->a1*(m->b3*m->c4-m->b4*m->c3) + m->a3*(m->b4*m->c1-m->b1*m->c4) + m->a4*(m->b1*m->c3-m->b3*m->c1));
	p->c1= invdet * (m->b1*(m->c2*m->d4-m->c4*m->d2) + m->b2*(m->c4*m->d1-m->c1*m->d4) + m->b4*(m->c1*m->d2-m->c2*m->d1));
	p->c2=-invdet * (m->a1*(m->c2*m->d4-m->c4*m->d2) + m->a2*(m->c4*m->d1-m->c1*m->d4) + m->a4*(m->c1*m->d2-m->c2*m->d1));
	p->c3= invdet * (m->a1*(m->b2*m->d4-m->b4*m->d2) + m->a2*(m->b4*m->d1-m->b1*m->d4) + m->a4*(m->b1*m->d2-m->b2*m->d1));
	p->c4=-invdet * (m->a1*(m->b2*m->c4-m->b4*m->c2) + m->a2*(m->b4*m->c1-m->b1*m->c4) + m->a4*(m->b1*m->c2-m->b2*m->c1));
	p->d1=-invdet * (m->b1*(m->c2*m->d3-m->c3*m->d2) + m->b2*(m->c3*m->d1-m->c1*m->d3) + m->b3*(m->c1*m->d2-m->c2*m->d1));
	p->d2= invdet * (m->a1*(m->c2*m->d3-m->c3*m->d2) + m->a2*(m->c3*m->d1-m->c1*m->d3) + m->a3*(m->c1*m->d2-m->c2*m->d1));
	p->d3=-invdet * (m->a1*(m->b2*m->d3-m->b3*m->d2) + m->a2*(m->b3*m->d1-m->b1*m->d3) + m->a3*(m->b1*m->d2-m->b2*m->d1));
	p->d4= invdet * (m->a1*(m->b2*m->c3-m->b3*m->c2) + m->a2*(m->b3*m->c1-m->b1*m->c3) + m->a3*(m->b1*m->c2-m->b2*m->c1));
}

void print_matrix(struct aiMatrix4x4 *m)
{
	fprintf(stderr, "matrix %g %g %g %g %g %g %g %g %g (det=%g)\n",
			m->a1, m->a2, m->a3,
			m->b1, m->b2, m->b3,
			m->c1, m->c2, m->c3,
			aiDeterminant(m));
}

int is_identity_matrix(struct aiMatrix4x4 *m)
{
	return
		NEAR_1(m->a1) && NEAR_0(m->a2) && NEAR_0(m->a3) &&
		NEAR_0(m->b1) && NEAR_1(m->b2) && NEAR_0(m->b3) &&
		NEAR_0(m->c1) && NEAR_0(m->c2) && NEAR_1(m->c3) &&
		NEAR_0(m->a4) && NEAR_0(m->b4) && NEAR_0(m->c4);
}

char basename[1024];

int numtags = 0;
char **taglist = NULL;

#define MAXBLEND 12
#define MIN(a,b) ((a)<(b)?(a):(b))

struct vb {
	int b[MAXBLEND];
	float w[MAXBLEND];
	int n;
};

struct material {
	struct aiMaterial *material;
	char file[100];
	char name[100];
};

struct material matmap[1000];
int nummats = 0;

struct bone {
	char *name;
	int number; // for iqe export
	int parent;
	int isbone;
	struct aiNode *node;
	struct aiMatrix4x4 invpose; // inv(parent * pose)
	struct aiMatrix4x4 abspose; // (parent * pose)
	struct aiMatrix4x4 pose;

	// current pose in decomposed form
	struct aiVector3D translate;
	struct aiQuaternion rotate;
	struct aiVector3D scale;
};

int need_to_bake_skin = 0;
int save_all_bones = 0;

int doanim = 0;	// export animations
int dobone = 0;	// export skeleton
int doflip = 0;	// export flipped (quake-style) triangles and normals

struct bone bonelist[1000];
int numbones = 0;

int find_bone(char *name)
{
	int i;
	for (i = 0; i < numbones; i++)
		if (!strcmp(name, bonelist[i].name))
			return i;
	return -1;
}

char *get_base_name(char *s)
{
	char *p = strrchr(s, '/');
	if (!p) p = strrchr(s, '\\');
	if (!p) return s;
	return p + 1;
}

char *node_name(char *orig)
{
	static char buf[200];
	if (orig == buf) return orig; // no need to clean the same name twice (hack!)
	char *p = orig;
	if (strstr(orig, "node-") == orig)
		orig += 5;
	strcpy(buf, orig);
	for (p = buf; *p; p++)
		*p = tolower(*p);
	return buf;
}

char *find_material(struct aiMaterial *material)
{
	int i, count;
	char buf[200], *p;
	struct aiString str;

	for (i = 0; i < nummats; i++)
		if (matmap[i].material == material)
			return matmap[i].name;

	if (!aiGetMaterialString(material, AI_MATKEY_TEXTURE_DIFFUSE(0), &str))
		strcpy(buf, get_base_name(str.data));
	else
		strcpy(buf, "unknown.png");
	p = strrchr(buf, '.');
	if (p) *p = 0;

	count = 0;
	for (i = 0; i < nummats; i++)
		if (!strcmp(matmap[i].file, buf))
			count++;

	matmap[nummats].material = material;
	strcpy(matmap[nummats].file, buf);
	sprintf(matmap[nummats].name, "%s,%d", buf, count);
	nummats++;

	p = matmap[nummats-1].name;
	while (*p) { *p = tolower(*p); p++; }

	return matmap[nummats-1].name;
}

// --- figure out which bones are part of armature ---

void mark_bone_parents(int i)
{
	while (i >= 0) {
		bonelist[i].isbone = 1;
		i = bonelist[i].parent;
	}
}

void mark_tags(void)
{
	int i, k;
	for (k = 0; k < numtags; k++) {
		fprintf(stderr, "marking tag %s\n", taglist[k]);
		for (i = 0; i < numbones; i++) {
			if (!strcmp(taglist[k], node_name(bonelist[i].name))) {
				bonelist[i].isbone = 1;
				break;
			}
		}
		if (i == numbones)
			fprintf(stderr, "\tnot found!\n");
	}
}

void build_bone_list_imp(struct aiNode *node, int parent)
{
	int i;

	bonelist[numbones].name = node->mName.data;
	bonelist[numbones].parent = parent;
	bonelist[numbones].isbone = 0;
	bonelist[numbones].node = node;

	// all non-bone nodes in our animated models have identity matrix
	// or are bones with no weights, so don't matter
	aiIdentityMatrix4(&bonelist[numbones].invpose);
	aiIdentityMatrix4(&bonelist[numbones].abspose);
	aiIdentityMatrix4(&bonelist[numbones].pose);

	parent = numbones++;
	for (i = 0; i < node->mNumChildren; i++)
		build_bone_list_imp(node->mChildren[i], parent);
}

void build_bone_list(const struct aiScene *scene)
{
	int i, k, a, b;
	int number = 0;

	// build list of all nodes
	build_bone_list_imp(scene->mRootNode, -1);

	// walk through all meshes and mark used bones
	for (k = 0; k < scene->mNumMeshes; k++) {
		struct aiMesh *mesh = scene->mMeshes[k];
		for (a = 0; a < mesh->mNumBones; a++) {
			b = find_bone(mesh->mBones[a]->mName.data);
			if (!bonelist[b].isbone) {
				bonelist[b].invpose = mesh->mBones[a]->mOffsetMatrix;
				bonelist[b].isbone = 1;
			} else if (!need_to_bake_skin) {
				if (memcmp(&bonelist[b].invpose, &mesh->mBones[a]->mOffsetMatrix, sizeof bonelist[b].invpose))
					need_to_bake_skin = 1;
			}
		}
	}

	// we now (in the single mesh case) have our bind pose
	// our invpose is set to the inv_bind_pose matrix
	// compute forward abspose and pose matrices here
	for (i = 0; i < numbones; i++) {
		if (bonelist[i].isbone) {
			aiInverseMatrix(&bonelist[i].abspose, &bonelist[i].invpose);
			bonelist[i].pose = bonelist[i].abspose;
			if (bonelist[i].parent >= 0) {
				struct aiMatrix4x4 m = bonelist[bonelist[i].parent].invpose;
				aiMultiplyMatrix4(&m, &bonelist[i].pose);
				bonelist[i].pose = m;
			}
		} else {
			// no inv_bind_pose matrix (so not used by skin)
			// take the pose from the first frame of animation
			bonelist[i].pose = bonelist[i].node->mTransformation;
			bonelist[i].abspose = bonelist[i].pose;
			if (bonelist[i].parent >= 0) {
				bonelist[i].abspose = bonelist[bonelist[i].parent].abspose;
				aiMultiplyMatrix4(&bonelist[i].abspose, &bonelist[i].pose);
			}
			aiInverseMatrix(&bonelist[i].invpose, &bonelist[i].abspose);
		}
	}

	// walk through all anims and mark used bones
	for (i = 0; i < scene->mNumAnimations; i++) {
		const struct aiAnimation *anim = scene->mAnimations[i];
		for (k = 0; k < anim->mNumChannels; k++) {
			b = find_bone(anim->mChannels[k]->mNodeName.data);
			bonelist[b].isbone = 1;
		}
	}

	// mark special bones named on command line as "tags" to attach stuff
	mark_tags();

	// select all parents of bones
	for (i = 0; i < numbones; i++) {
		if (bonelist[i].isbone)
			mark_bone_parents(i);
	}

	// skip root node if it has 1 child and identity transform
	int count = 0;
	for (i = 0; i < numbones; i++)
		if (bonelist[i].isbone && bonelist[i].parent == 0)
			count++;
	if (count == 1 && is_identity_matrix(&bonelist[0].node->mTransformation)) {
		fprintf(stderr, "skipping root with one child and identity transform\n");
		bonelist[0].isbone = 0;
		bonelist[0].number = -1;
	}

	// select all children of bones as well.
	// this will select 'dead' leaves in the skeleton
 	// with no mesh and no animation keys.
	// could be useful for 'tag' objects if you don't use the mark_tags feature.
	if (save_all_bones) {
		for (i = 0; i < numbones; i++) {
			if (!bonelist[i].isbone)
				if (bonelist[i].parent >= 0 && bonelist[bonelist[i].parent].isbone)
					bonelist[i].isbone = 1;
		}
	}

	for (i = 0; i < numbones; i++)
		if (!bonelist[i].isbone)
			fprintf(stderr, "skipping bone %s\n", node_name(bonelist[i].name));

	// assign IQE numbers to bones
	for (i = 0; i < numbones; i++)
		if (bonelist[i].isbone)
			bonelist[i].number = number++;
}

// --- export poses and animation frames ---

void export_pm(FILE *out, struct aiMatrix4x4 *m)
{
	fprintf(out, "pm %g %g %g %g %g %g %g %g %g %g %g %g\n",
			KILL(m->a4), KILL(m->b4), KILL(m->c4),
			KILL(m->a1), KILL(m->a2), KILL(m->a3),
			KILL(m->b1), KILL(m->b2), KILL(m->b3),
			KILL(m->c1), KILL(m->c2), KILL(m->c3));
}

void export_pose(FILE *out, int i)
{
	struct aiQuaternion rotate = bonelist[i].rotate;
	struct aiVector3D scale = bonelist[i].scale;
	struct aiVector3D translate = bonelist[i].translate;

	if (NEAR_1(scale.x) && NEAR_1(scale.y) && NEAR_1(scale.z))
		fprintf(out, "pq %g %g %g %g %g %g %g\n",
			KILL(translate.x), KILL(translate.y), KILL(translate.z),
			KILL(rotate.x), KILL(rotate.y), KILL(rotate.z), KILL(rotate.w));
	else
		fprintf(out, "pq %g %g %g %g %g %g %g %g %g %g\n",
			KILL(translate.x), KILL(translate.y), KILL(translate.z),
			KILL(rotate.x), KILL(rotate.y), KILL(rotate.z), KILL(rotate.w),
			KILL(scale.x), KILL(scale.y), KILL(scale.z));
}

void export_bone_list(FILE *out)
{
	int i;

	fprintf(out, "\n");
	for (i = 0; i < numbones; i++) {
		if (bonelist[i].isbone) {
			if (bonelist[i].parent >= 0)
				fprintf(out, "joint %s %d\n",
					node_name(bonelist[i].name),
					bonelist[bonelist[i].parent].number);
			else
				fprintf(out, "joint %s -1\n", node_name(bonelist[i].name));
		}
	}

	fprintf(out, "\n");
	for (i = 0; i < numbones; i++) {
		if (bonelist[i].isbone) {
			aiDecomposeMatrix(&bonelist[i].pose, &bonelist[i].scale, &bonelist[i].rotate, &bonelist[i].translate);
			// export_pose(out, i);
			export_pm(out, &bonelist[i].pose);
		}
	}
}

void apply_initial_frame(void)
{
	int i;
	for (i = 0; i < numbones; i++) {
		bonelist[i].pose = bonelist[i].node->mTransformation;
		aiDecomposeMatrix(&bonelist[i].pose, &bonelist[i].scale, &bonelist[i].rotate, &bonelist[i].translate);
	}
}

void export_frame(FILE *out, const struct aiAnimation *anim, int frame)
{
	int i;
	for (i = 0; i < anim->mNumChannels; i++) {
		struct aiNodeAnim *chan = anim->mChannels[i];
		int a = find_bone(chan->mNodeName.data);
		int tframe = MIN(frame, chan->mNumPositionKeys - 1);
		int rframe = MIN(frame, chan->mNumRotationKeys - 1);
		int sframe = MIN(frame, chan->mNumScalingKeys - 1);
		bonelist[a].translate = chan->mPositionKeys[tframe].mValue;
 		bonelist[a].rotate = chan->mRotationKeys[rframe].mValue;
 		bonelist[a].scale = chan->mScalingKeys[sframe].mValue;
	}

	fprintf(out, "\n");
	fprintf(out, "frame\n");
	for (i = 0; i < numbones; i++) {
		if (bonelist[i].isbone)
			export_pose(out, i);
	}
}

int animation_length(const struct aiAnimation *anim)
{
	int i, len = 0;
	for (i = 0; i < anim->mNumChannels; i++) {
		struct aiNodeAnim *chan = anim->mChannels[i];
		if (chan->mNumPositionKeys > len) len = chan->mNumPositionKeys;
		if (chan->mNumRotationKeys > len) len = chan->mNumRotationKeys;
		if (chan->mNumScalingKeys > len) len = chan->mNumScalingKeys;
	}
	return len;
}

void export_animations(FILE *out, const struct aiScene *scene)
{
	int i, k, len;
	for (i = 0; i < scene->mNumAnimations; i++) {
		const struct aiAnimation *anim = scene->mAnimations[i];
		if (scene->mNumAnimations > 1)
			fprintf(out, "\nanimation %s,%02d\n", basename, i);
		else
			fprintf(out, "\nanimation %s\n", basename);
		fprintf(out, "framerate 30\n");
		len = animation_length(anim);
		apply_initial_frame();
		for (k = 0; k < len; k++)
			export_frame(out, anim, k);
	}
}

/*
 * For multi-mesh models, sometimes each mesh has its own inv_bind_matrix set
 * for each bone. To export to IQE we must have only one inv_bind_matrix per
 * bone. We can bake the mesh by animating it to the first animation frame.
 * Once this is done, set the inv_bind_matrix to be the inverse of the forward
 * bind_matrix of this pose.
 */

void bake_mesh_skin(const struct aiMesh *mesh)
{
	int i, k, b;
	struct aiMatrix3x3 mat3;
	struct aiMatrix4x4 bonemat[1000], mat;
	struct aiVector3D *outpos, *outnorm;

	if (mesh->mNumBones == 0)
		return;

	outpos = malloc(mesh->mNumVertices * sizeof *outpos);
	outnorm = malloc(mesh->mNumVertices * sizeof *outnorm);
	memset(outpos, 0, mesh->mNumVertices * sizeof *outpos);
	memset(outnorm, 0, mesh->mNumVertices * sizeof *outpos);

	for (i = 0; i < numbones; i++) {
		bonelist[i].abspose = bonelist[i].pose;
		if (bonelist[i].parent >= 0) {
			bonelist[i].abspose = bonelist[bonelist[i].parent].abspose;
			aiMultiplyMatrix4(&bonelist[i].abspose, &bonelist[i].pose);
		}
	}

	for (i = 0; i < mesh->mNumBones; i++) {
		b = find_bone(mesh->mBones[i]->mName.data);
		bonemat[i] = bonelist[b].abspose;
		aiMultiplyMatrix4(&bonemat[i], &mesh->mBones[i]->mOffsetMatrix);
	}

	for (k = 0; k < mesh->mNumBones; k++) {
		struct aiBone *bone = mesh->mBones[k];
		b = find_bone(mesh->mBones[k]->mName.data);
		mat = bonemat[k];
		mat3.a1 = mat.a1; mat3.a2 = mat.a2; mat3.a3 = mat.a3;
		mat3.b1 = mat.b1; mat3.b2 = mat.b2; mat3.b3 = mat.b3;
		mat3.c1 = mat.c1; mat3.c2 = mat.c2; mat3.c3 = mat.c3;
		for (i = 0; i < bone->mNumWeights; i++) {
			struct aiVertexWeight vw = bone->mWeights[i];
			int v = vw.mVertexId;
			float w = vw.mWeight;
			struct aiVector3D srcpos = mesh->mVertices[v];
			struct aiVector3D srcnorm = mesh->mNormals[v];
			aiTransformVecByMatrix4(&srcpos, &mat);
			aiTransformVecByMatrix3(&srcnorm, &mat3);
			outpos[v].x += srcpos.x * w;
			outpos[v].y += srcpos.y * w;
			outpos[v].z += srcpos.z * w;
			outnorm[v].x += srcnorm.x * w;
			outnorm[v].y += srcnorm.y * w;
			outnorm[v].z += srcnorm.z * w;
		}
	}

	memcpy(mesh->mVertices, outpos, mesh->mNumVertices * sizeof *outpos);
	memcpy(mesh->mNormals, outnorm, mesh->mNumVertices * sizeof *outnorm);

	free(outpos);
	free(outnorm);
}

void bake_scene_skin(const struct aiScene *scene)
{
	int i;
	fprintf(stderr, "baking skin to recreate base pose in multi-mesh model\n");
	for (i = 0; i < scene->mNumMeshes; i++)
		bake_mesh_skin(scene->mMeshes[i]);
}

/*
 * Export meshes. Group them by materials. Also apply the node transform
 * to the vertices. IQE does not have a concept of per-group transforms.
 *
 * If we are exporting a rigged model, we have to skip any meshes which
 * are not deformed by the armature. If we are exporting a non-rigged model,
 * we have to pre-transform all meshes.
 *
 * TODO: turn non-rigged meshes into rigged meshes by hooking them up to
 * a synthesized bone for its node.
 */

void export_node(FILE *out, const struct aiScene *scene, const struct aiNode *node,
	struct aiMatrix4x4 mat, char *nodename)
{
	struct aiMatrix3x3 mat3;
	int i, k, t, a;

	aiMultiplyMatrix4(&mat, &node->mTransformation);
	mat3.a1 = mat.a1; mat3.a2 = mat.a2; mat3.a3 = mat.a3;
	mat3.b1 = mat.b1; mat3.b2 = mat.b2; mat3.b3 = mat.b3;
	mat3.c1 = mat.c1; mat3.c2 = mat.c2; mat3.c3 = mat.c3;

	if (!strstr(node->mName.data, "$ColladaAutoName$"))
		nodename = (char*)node->mName.data;

	nodename = node_name(nodename);

	for (i = 0; i < node->mNumMeshes; i++) {
		struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

		/* skip non-boned meshes if we have a skeleton */
		// TODO: we could fake bones here by adding a bone for this node
		// and hard coding the blend index and weight for it.
		if (mesh->mNumBones == 0 && dobone) {
			fprintf(stderr, "skipping mesh %d in node %s (no bones)\n", i, nodename);
			continue;
		}

		struct vb *vb = (struct vb*) malloc(mesh->mNumVertices * sizeof(*vb));
		memset(vb, 0, mesh->mNumVertices * sizeof(*vb));

		fprintf(out, "\n");
		if (node->mNumMeshes > 99)
			fprintf(out, "mesh %s,%03d\n", nodename, i);
		else if (node->mNumMeshes > 9)
			fprintf(out, "mesh %s,%02d\n", nodename, i);
		else if (node->mNumMeshes > 1)
			fprintf(out, "mesh %s,%d\n", nodename, i);
		else
			fprintf(out, "mesh %s\n", nodename);

		fprintf(out, "material %s\n", find_material(material));

		for (k = 0; k < mesh->mNumBones; k++) {
			struct aiBone *bone = mesh->mBones[k];
			a = find_bone(bone->mName.data);
			for (t = 0; t < bone->mNumWeights; t++) {
				struct aiVertexWeight *w = mesh->mBones[k]->mWeights + t;
				int idx = w->mVertexId;
				if (vb[idx].n < MAXBLEND) {
					vb[idx].b[vb[idx].n] = bonelist[a].number;
					vb[idx].w[vb[idx].n] = w->mWeight;
					vb[idx].n++;
				}
			}
		}

		for (k = 0; k < mesh->mNumVertices; k++) {
			struct aiVector3D vp = mesh->mVertices[k];
			if (!dobone)
				aiTransformVecByMatrix4(&vp, &mat);
			fprintf(out, "vp %g %g %g\n", vp.x, vp.y, vp.z);
			if (mesh->mTextureCoords[0]) {
				float u = mesh->mTextureCoords[0][k].x;
				float v = 1 - mesh->mTextureCoords[0][k].y;
				fprintf(out, "vt %g %g\n", u, v);
			} else fprintf(out, "vt 0 0\n");
			if (mesh->mNormals) {
				struct aiVector3D vn = mesh->mNormals[k];
				if (!dobone)
					aiTransformVecByMatrix3(&vn, &mat3);
				if (doflip)
						fprintf(out, "vn %g %g %g\n", -vn.x, -vn.y, -vn.z);
				else
						fprintf(out, "vn %g %g %g\n", vn.x, vn.y, vn.z);
			}
			if (mesh->mColors[0]) {
				float r = mesh->mColors[0][k].r; r = floorf(r * 255) / 255;
				float g = mesh->mColors[0][k].g; g = floorf(g * 255) / 255;
				float b = mesh->mColors[0][k].b; b = floorf(b * 255) / 255;
				float a = mesh->mColors[0][k].a; a = floorf(a * 255) / 255;
				fprintf(out, "vc %g %g %g %g\n", r, g, b, 1.0);
			}
			if (mesh->mNumBones > 0) {
				fprintf(out, "vb");
				for (t = 0; t < vb[k].n; t++)
					fprintf(out, " %d %g", vb[k].b[t], vb[k].w[t]);
				fprintf(out, "\n");
			}
		}

		for (k = 0; k < mesh->mNumFaces; k++) {
			struct aiFace *face = mesh->mFaces + k;
			assert(face->mNumIndices == 3);
			if (doflip)
					fprintf(out, "fm %d %d %d\n", face->mIndices[0], face->mIndices[2], face->mIndices[1]);
			else
					fprintf(out, "fm %d %d %d\n", face->mIndices[0], face->mIndices[1], face->mIndices[2]);
		}

		free(vb);
	}

	for (i = 0; i < node->mNumChildren; i++)
		export_node(out, scene, node->mChildren[i], mat, nodename);
}

void usage()
{
	fprintf(stderr, "usage: assiqe [options] [-o out.iqe] input.dae [tags ...]\n");
	fprintf(stderr, "\t-a -- only export animations\n");
	fprintf(stderr, "\t-b -- export unused bones too\n");
	fprintf(stderr, "\t-m -- bake mesh to bind pose / first frame\n");
	fprintf(stderr, "\t-f -- flip normals and winding order\n");
	fprintf(stderr, "\t-o filename -- save output to file\n");
	exit(1);
}

int main(int argc, char **argv)
{
	FILE *file;
	const struct aiScene *scene;
	char *p;
	int c, k;

	char *output = NULL;
	char *input = NULL;
	int domesh = 1;

	while ((c = getopt(argc, argv, "abfmo:")) != -1) {
		switch (c) {
		case 'a': domesh = 0; break;
		case 'b': save_all_bones = 1; break;
		case 'f': doflip = 1; break;
		case 'm': need_to_bake_skin = 1; break;
		case 'o': output = optarg++; break;
		default: usage(); break;
		}
	}

	if (optind == argc)
		usage();

	input = argv[optind++];

	p = strrchr(input, '/');
	if (!p) p = strrchr(input, '\\');
	if (!p) p = input; else p++;
	strcpy(basename, p);
	p = strrchr(basename, '.');
	if (p) *p = 0;

	numtags = argc - optind;
	taglist = argv + optind;


	/*
	 * Read input file and process with assimp
	 */

	int flags = aiProcess_Triangulate;
	flags |= aiProcess_JoinIdenticalVertices;
	flags |= aiProcess_GenSmoothNormals;
	flags |= aiProcess_GenUVCoords;
	flags |= aiProcess_TransformUVCoords;
	flags |= aiProcess_LimitBoneWeights;
	//flags |= aiProcess_FindInvalidData;
	flags |= aiProcess_ImproveCacheLocality;
	//flags |= aiProcess_RemoveRedundantMaterials;
	//flags |= aiProcess_OptimizeMeshes;

	fprintf(stderr, "loading %s\n", input);
	scene = aiImportFile(input, flags);
	if (!scene) {
		fprintf(stderr, "cannot import '%s'\n", input);
		exit(1);
	}

	/*
	 * Fiddle with the data and bang it into a shape suitable for
	 * exporting to IQE.
	 */

	// Convert to Z-UP coordinate system
	struct aiMatrix4x4 yup_to_zup = {
		1, 0, 0, 0,
		0, 0, -1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
	aiMultiplyMatrix4(&scene->mRootNode->mTransformation, &yup_to_zup);

	// Do we have a skeleton?
	for (k = 0; k < scene->mNumMeshes; k++)
		if (scene->mMeshes[k]->mNumBones > 0)
			dobone = 1;

	// Do we have animations?
	if (scene->mNumAnimations > 0)
		doanim = 1;

	// Make a list of useful bones
	build_bone_list(scene);

	if (need_to_bake_skin) {
		apply_initial_frame();
		bake_scene_skin(scene);
	}

	/*
	 * Export scene as mesh/skeleton/animation
	 */

	if (output) {
		fprintf(stderr, "saving %s\n", output);
		file = fopen(output, "w");
		if (!file) {
			fprintf(stderr, "cannot open output file: '%s'\n", output);
			exit(1);
		}
	} else {
		file = stdout;
	}

	fprintf(file, "# Inter-Quake Export\n");

	if (dobone) {
		export_bone_list(file);
	}

	if (domesh) {
		struct aiMatrix4x4 mat;
		aiIdentityMatrix4(&mat);
		export_node(file, scene, scene->mRootNode, mat, "SCENE");
	}

	if (doanim) {
		export_animations(file, scene);
	}

	if (output)
		fclose(file);

	aiReleaseImport(scene);

	return 0;
}
