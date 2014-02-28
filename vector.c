#include "mio.h"

/* column-major 4x4 matrices, as in opengl */

#define A(row,col) a[(col<<2)+row]
#define B(row,col) b[(col<<2)+row]
#define M(row,col) m[(col<<2)+row]

void mat_print(FILE *file, mat4 m)
{
	printf("pm\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n",
		m[0], m[4], m[8], m[12],
		m[1], m[5], m[9], m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]);
}

void mat_identity(mat4 m)
{
	M(0,0) = 1; M(0,1) = 0; M(0,2) = 0; M(0,3) = 0;
	M(1,0) = 0; M(1,1) = 1; M(1,2) = 0; M(1,3) = 0;
	M(2,0) = 0; M(2,1) = 0; M(2,2) = 1; M(2,3) = 0;
	M(3,0) = 0; M(3,1) = 0; M(3,2) = 0; M(3,3) = 1;
}

void mat_copy(mat4 p, const mat4 m)
{
	memcpy(p, m, sizeof(mat4));
}

void mat_mul44(mat4 m, const mat4 a, const mat4 b)
{
	int i;
	for (i = 0; i < 4; i++) {
		const float ai0=A(i,0), ai1=A(i,1), ai2=A(i,2), ai3=A(i,3);
		M(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		M(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		M(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		M(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
}

void mat_mul(mat4 m, const mat4 a, const mat4 b)
{
	int i;
	for (i = 0; i < 3; i++) {
		const float ai0=A(i,0), ai1=A(i,1), ai2=A(i,2), ai3=A(i,3);
		M(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0);
		M(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1);
		M(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2);
		M(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3;
	}
	M(3,0) = 0;
	M(3,1) = 0;
	M(3,2) = 0;
	M(3,3) = 1;
}

void mat_frustum(mat4 m,
	float left, float right,
	float bottom, float top,
	float nearval, float farval)
{
	float x, y, a, b, c, d;

	x = (2*nearval) / (right-left);
	y = (2*nearval) / (top-bottom);
	a = (right+left) / (right-left);
	b = (top+bottom) / (top-bottom);
	c = -(farval+nearval) / ( farval-nearval);
	d = -(2*farval*nearval) / (farval-nearval);

	M(0,0) = x; M(0,1) = 0; M(0,2) = a; M(0,3) = 0;
	M(1,0) = 0; M(1,1) = y; M(1,2) = b; M(1,3) = 0;
	M(2,0) = 0; M(2,1) = 0; M(2,2) = c; M(2,3) = d;
	M(3,0) = 0; M(3,1) = 0; M(3,2) = -1; M(3,3) = 0;
}

void mat_perspective(mat4 m,
	float fov, float aspect, float znear, float zfar)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * znear;
	mat_frustum(m, -fov * aspect, fov * aspect, -fov, fov, znear, zfar);
}

void mat_ortho(mat4 m,
	float left, float right,
	float bottom, float top,
	float nearval, float farval)
{
	M(0,0) = 2 / (right-left);
	M(0,1) = 0;
	M(0,2) = 0;
	M(0,3) = -(right+left) / (right-left);

	M(1,0) = 0;
	M(1,1) = 2 / (top-bottom);
	M(1,2) = 0;
	M(1,3) = -(top+bottom) / (top-bottom);

	M(2,0) = 0;
	M(2,1) = 0;
	M(2,2) = -2 / (farval-nearval);
	M(2,3) = -(farval+nearval) / (farval-nearval);

	M(3,0) = 0;
	M(3,1) = 0;
	M(3,2) = 0;
	M(3,3) = 1;
}

void mat_look_at(mat4 m, const vec3 eye, const vec3 center, const vec3 up_in)
{
	vec3 forward, side, up;
	vec_sub(forward, center, eye);
	vec_normalize(forward, forward);
	vec_cross(side, forward, up_in);
	vec_normalize(side, side);
	vec_cross(up, side, forward);
	m[0] = side[0];
	m[4] = side[1];
	m[8] = side[2];
	m[12] = 0;
	m[1] = up[0];
	m[5] = up[1];
	m[9] = up[2];
	m[13] = 0;
	m[2] = -forward[0];
	m[6] = -forward[1];
	m[10] = -forward[2];
	m[14] = 0;
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
	mat_translate(m, -eye[0], -eye[1], -eye[2]);
}

void mat_look(mat4 m, const vec3 eye, const vec3 forward_in, const vec3 up_in)
{
	vec3 forward, side, up;
	vec_normalize(forward, forward_in);
	vec_cross(side, forward, up_in);
	vec_normalize(side, side);
	vec_cross(up, side, forward);
	vec_normalize(up, up);
	m[0] = side[0];
	m[4] = side[1];
	m[8] = side[2];
	m[12] = 0;
	m[1] = up[0];
	m[5] = up[1];
	m[9] = up[2];
	m[13] = 0;
	m[2] = -forward[0];
	m[6] = -forward[1];
	m[10] = -forward[2];
	m[14] = 0;
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
	mat_translate(m, -eye[0], -eye[1], -eye[2]);
}

void mat_scale(mat4 m, float x, float y, float z)
{
	m[0] *= x; m[4] *= y; m[8] *= z;
	m[1] *= x; m[5] *= y; m[9] *= z;
	m[2] *= x; m[6] *= y; m[10] *= z;
	m[3] *= x; m[7] *= y; m[11] *= z;
}

void mat_translate(mat4 m, float x, float y, float z)
{
	m[12] = m[0] * x + m[4] * y + m[8] * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9] * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

static inline float radians(float degrees)
{
	return degrees * 3.14159265f / 180;
}

void mat_rotate_x(mat4 p, float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	mat4 m;
	mat_identity(m);
	M(1,1) = c;
	M(2,2) = c;
	M(1,2) = -s;
	M(2,1) = s;
	mat_mul44(p, p, m);
}

void mat_rotate_y(mat4 p, float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	mat4 m;
	mat_identity(m);
	M(0,0) = c;
	M(2,2) = c;
	M(0,2) = s;
	M(2,0) = -s;
	mat_mul44(p, p, m);
}

void mat_rotate_z(mat4 p, float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	mat4 m;
	mat_identity(m);
	M(0,0) = c;
	M(1,1) = c;
	M(0,1) = -s;
	M(1,0) = s;
	mat_mul44(p, p, m);
}

void mat_transpose(mat4 to, const mat4 from)
{
	assert(to != from);
	to[0] = from[0];
	to[1] = from[4];
	to[2] = from[8];
	to[3] = from[12];
	to[4] = from[1];
	to[5] = from[5];
	to[6] = from[9];
	to[7] = from[13];
	to[8] = from[2];
	to[9] = from[6];
	to[10] = from[10];
	to[11] = from[14];
	to[12] = from[3];
	to[13] = from[7];
	to[14] = from[11];
	to[15] = from[15];
}

void mat_invert(mat4 out, const mat4 m)
{
	mat4 inv;
	float det;
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

/* Transform a point (column vector) by a matrix: p = m * v */
void mat_vec_mul(vec3 p, const mat4 m, const vec3 v)
{
	assert(p != v);
	p[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	p[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	p[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}

void mat_vec_mul_n(vec3 p, const mat4 m, const vec3 v)
{
	assert(p != v);
	p[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2];
	p[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2];
	p[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2];
}

/* Transform a normal (row vector) by a matrix: [px py pz] = v * m */
void mat_vec_mul_t(vec3 p, const mat4 m, const vec3 v)
{
	assert(p != v);
	p[0] = v[0] * m[0] + v[1] * m[1] + v[2] * m[2];
	p[1] = v[0] * m[4] + v[1] * m[5] + v[2] * m[6];
	p[2] = v[0] * m[8] + v[1] * m[9] + v[2] * m[10];
}

void vec_init(vec3 p, float x, float y, float z)
{
	p[0] = x;
	p[1] = y;
	p[2] = z;
}

void vec_scale(vec3 p, const vec3 v, float s)
{
	p[0] = v[0] * s;
	p[1] = v[1] * s;
	p[2] = v[2] * s;
}

void vec_add(vec3 p, const vec3 a, const vec3 b)
{
	p[0] = a[0] + b[0];
	p[1] = a[1] + b[1];
	p[2] = a[2] + b[2];
}

void vec_sub(vec3 p, const vec3 a, const vec3 b)
{
	p[0] = a[0] - b[0];
	p[1] = a[1] - b[1];
	p[2] = a[2] - b[2];
}

void vec_mul(vec3 p, const vec3 a, const vec3 b)
{
	p[0] = a[0] * b[0];
	p[1] = a[1] * b[1];
	p[2] = a[2] * b[2];
}

void vec_div(vec3 p, const vec3 a, const vec3 b)
{
	p[0] = a[0] / b[0];
	p[1] = a[1] / b[1];
	p[2] = a[2] / b[2];
}

void vec_div_s(vec3 p, const vec3 a, float b)
{
	p[0] = a[0] / b;
	p[1] = a[1] / b;
	p[2] = a[2] / b;
}

void vec_lerp(vec3 p, const vec3 a, const vec3 b, float t)
{
	p[0] = a[0] + t * (b[0] - a[0]);
	p[1] = a[1] + t * (b[1] - a[1]);
	p[2] = a[2] + t * (b[2] - a[2]);
}

void vec_average(vec3 p, const vec3 a, const vec3 b)
{
	vec_lerp(p, a, b, 0.5f);
}

void vec_cross(vec3 p, const vec3 a, const vec3 b)
{
	assert(p != a && p != b);
	p[0] = a[1] * b[2] - a[2] * b[1];
	p[1] = a[2] * b[0] - a[0] * b[2];
	p[2] = a[0] * b[1] - a[1] * b[0];
}

float vec_dot(const vec3 a, const vec3 b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float vec_length(const vec3 a)
{
	return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

float vec_dist2(const vec3 a, const vec3 b)
{
	float d0, d1, d2;
	d0 = a[0] - b[0];
	d1 = a[1] - b[1];
	d2 = a[2] - b[2];
	return d0 * d0 + d1 * d1 + d2 * d2;
}

float vec_dist(const vec3 a, const vec3 b)
{
	return sqrtf(vec_dist2(a, b));
}

void vec_normalize(vec3 v, const vec3 a)
{
	float d = sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
	if (d >= 0.00001) {
		d = 1 / d;
		v[0] = a[0] * d;
		v[1] = a[1] * d;
		v[2] = a[2] * d;
	} else {
		v[0] = v[1] = 0;
		v[2] = 1;
	}
}

void vec_face_normal(vec3 n, const vec3 p0, const vec3 p1, const vec3 p2)
{
	vec3 u, v;
	vec_sub(u, p1, p0);
	vec_sub(v, p2, p0);
	vec_cross(n, u, v);
}

void vec_negate(vec3 p, const vec3 a)
{
	p[0] = -a[0];
	p[1] = -a[1];
	p[2] = -a[2];
}

void vec_invert(vec3 p, const vec3 a)
{
	p[0] = 1 / a[0];
	p[1] = 1 / a[1];
	p[2] = 1 / a[2];
}

void vec_yup_to_zup(vec3 v)
{
	float z = v[2];
	v[2] = v[1];
	v[1] = -z;
}

void quat_init(vec4 p, float x, float y, float z, float w)
{
	p[0] = x;
	p[1] = y;
	p[2] = z;
	p[3] = w;
}

void quat_copy(vec4 q, const vec4 a)
{
	q[0] = a[0];
	q[1] = a[1];
	q[2] = a[2];
	q[3] = a[3];
}

float quat_dot(const vec4 a, const vec4 b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}

void quat_invert(vec4 out, const vec4 q)
{
	out[0] = -q[0];
	out[1] = -q[1];
	out[2] = -q[2];
	out[3] = -q[3];
}

void quat_conjugate(vec4 out, const vec4 q)
{
	out[0] = -q[0];
	out[1] = -q[1];
	out[2] = -q[2];
	out[3] = q[3];
}

void quat_mul(vec4 q, const vec4 a, const vec4 b)
{
	q[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
	q[1] = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
	q[2] = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
	q[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
}

void quat_vec_mul(vec3 dest, const vec4 q, const vec3 v)
{
	vec4 qvec = { v[0], v[1], v[2], 0 };
	vec4 qinv = { -q[0], -q[1], -q[2], -q[3] };
	vec4 t1, t2;
	quat_mul(t1, q, qvec);
	quat_mul(t2, qinv, t1);
	dest[0] = t2[0];
	dest[1] = t2[1];
	dest[2] = t2[2];
}

void quat_normalize(vec4 q, const vec4 a)
{
	float d = sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2] + a[3]*a[3]);
	if (d >= 0.00001) {
		d = 1 / d;
		q[0] = a[0] * d;
		q[1] = a[1] * d;
		q[2] = a[2] * d;
		q[3] = a[3] * d;
	} else {
		q[0] = q[1] = q[2] = 0;
		q[3] = 1;
	}
}

void quat_lerp(vec4 p, const vec4 a, const vec4 b, float t)
{
	p[0] = a[0] + t * (b[0] - a[0]);
	p[1] = a[1] + t * (b[1] - a[1]);
	p[2] = a[2] + t * (b[2] - a[2]);
	p[3] = a[3] + t * (b[3] - a[3]);
}

void quat_lerp_normalize(vec4 p, const vec4 a, const vec4 b, float t)
{
	quat_lerp(p, a, b, t);
	quat_normalize(p, p);
}

void quat_lerp_neighbor_normalize(vec4 p, const vec4 a, const vec4 b, float t)
{
	if (a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3] < 0) {
		vec4 temp = { -a[0], -a[1], -a[2], -a[3] };
		quat_lerp(p, temp, b, t);
	} else
		quat_lerp(p, a, b, t);
	quat_normalize(p, p);
}

void mat_from_quat(mat4 m, const vec4 q)
{
	float x2 = q[0] + q[0];
	float y2 = q[1] + q[1];
	float z2 = q[2] + q[2];
	{
		float xx2 = q[0] * x2;
		float yy2 = q[1] * y2;
		float zz2 = q[2] * z2;
		M(0,0) = 1 - yy2 - zz2;
		M(1,1) = 1 - xx2 - zz2;
		M(2,2) = 1 - xx2 - yy2;
	}
	{
		float yz2 = q[1] * z2;
		float wx2 = q[3] * x2;
		M(2,1) = yz2 + wx2;
		M(1,2) = yz2 - wx2;
	}
	{
		float xy2 = q[0] * y2;
		float wz2 = q[3] * z2;
		M(1,0) = xy2 + wz2;
		M(0,1) = xy2 - wz2;
	}
	{
		float xz2 = q[0] * z2;
		float wy2 = q[3] * y2;
		M(0,2) = xz2 + wy2;
		M(2,0) = xz2 - wy2;
	}

	M(0,3) = 0;
	M(1,3) = 0;
	M(2,3) = 0;

	M(3,0) = 0;
	M(3,1) = 0;
	M(3,2) = 0;
	M(3,3) = 1;
}

void mat_from_pose(mat4 m, const vec3 t, const vec4 q, const vec3 s)
{
	float x2 = q[0] + q[0];
	float y2 = q[1] + q[1];
	float z2 = q[2] + q[2];
	{
		float xx2 = q[0] * x2;
		float yy2 = q[1] * y2;
		float zz2 = q[2] * z2;
		M(0,0) = 1 - yy2 - zz2;
		M(1,1) = 1 - xx2 - zz2;
		M(2,2) = 1 - xx2 - yy2;
	}
	{
		float yz2 = q[1] * z2;
		float wx2 = q[3] * x2;
		M(2,1) = yz2 + wx2;
		M(1,2) = yz2 - wx2;
	}
	{
		float xy2 = q[0] * y2;
		float wz2 = q[3] * z2;
		M(1,0) = xy2 + wz2;
		M(0,1) = xy2 - wz2;
	}
	{
		float xz2 = q[0] * z2;
		float wy2 = q[3] * y2;
		M(0,2) = xz2 + wy2;
		M(2,0) = xz2 - wy2;
	}

	m[0] *= s[0]; m[4] *= s[1]; m[8] *= s[2];
	m[1] *= s[0]; m[5] *= s[1]; m[9] *= s[2];
	m[2] *= s[0]; m[6] *= s[1]; m[10] *= s[2];

	M(0,3) = t[0];
	M(1,3) = t[1];
	M(2,3) = t[2];

	M(3,0) = 0;
	M(3,1) = 0;
	M(3,2) = 0;
	M(3,3) = 1;
}

// only 3x3 rotation part from matrix with no scaling
void quat_from_mat(vec4 q, const mat4 m)
{
	float trace = M(0,0) + M(1,1) + M(2,2);
	if(trace > 0) {
		float r = sqrtf(1 + trace), inv = 0.5f/r;
		q[3] = 0.5f*r;
		q[0] = (M(2,1) - M(1,2))*inv;
		q[1] = (M(0,2) - M(2,0))*inv;
		q[2] = (M(1,0) - M(0,1))*inv;
	}
	else if(M(0,0) > M(1,1) && M(0,0) > M(2,2))
	{
		float r = sqrtf(1 + M(0,0) - M(1,1) - M(2,2)), inv = 0.5f/r;
		q[0] = 0.5f*r;
		q[1] = (M(1,0) + M(0,1))*inv;
		q[2] = (M(0,2) + M(2,0))*inv;
		q[3] = (M(2,1) - M(1,2))*inv;
	}
	else if(M(1,1) > M(2,2))
	{
		float r = sqrtf(1 + M(1,1) - M(0,0) - M(2,2)), inv = 0.5f/r;
		q[0] = (M(1,0) + M(0,1))*inv;
		q[1] = 0.5f*r;
		q[2] = (M(2,1) + M(1,2))*inv;
		q[3] = (M(0,2) - M(2,0))*inv;
	}
	else
	{
		double r = sqrtf(1 + M(2,2) - M(0,0) - M(1,1)), inv = 0.5f/r;
		q[0] = (M(0,2) + M(2,0))*inv;
		q[1] = (M(2,1) + M(1,2))*inv;
		q[2] = 0.5f*r;
		q[3] = (M(1,0) - M(0,1))*inv;
	}
}

int mat_is_negative(const mat4 m)
{
	vec3 v;
	vec_cross(v, m+0, m+4);
	return vec_dot(v, m+8) < 0;
}

void mat_decompose(const mat4 m, vec3 t, vec4 q, vec3 s)
{
	mat4 mn;

	t[0] = m[12];
	t[1] = m[13];
	t[2] = m[14];

	s[0] = vec_length(m+0);
	s[1] = vec_length(m+4);
	s[2] = vec_length(m+8);

	if (mat_is_negative(m)) {
		s[0] = -s[0];
		s[1] = -s[1];
		s[2] = -s[2];
	}

	mat_copy(mn, m);
	vec_div_s(mn+0, mn+0, s[0]);
	vec_div_s(mn+4, mn+4, s[1]);
	vec_div_s(mn+8, mn+8, s[2]);

	quat_from_mat(q, mn);
}

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
		mat_from_pose(pose_matrix[i], pose[i].position, pose[i].rotation, pose[i].scale);
}
