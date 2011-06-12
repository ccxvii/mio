#include "mio.h"

/* column-major 4x4 matrices, as in opengl */

#define A(row,col) a[(col<<2)+row]
#define B(row,col) b[(col<<2)+row]
#define M(row,col) m[(col<<2)+row]

void mat_print(FILE *file, float m[16])
{
	printf("pm\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n",
		m[0], m[4], m[8], m[12],
		m[1], m[5], m[9], m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]);
}

void mat_identity(float m[16])
{
	M(0,0) = 1; M(0,1) = 0; M(0,2) = 0; M(0,3) = 0;
	M(1,0) = 0; M(1,1) = 1; M(1,2) = 0; M(1,3) = 0;
	M(2,0) = 0; M(2,1) = 0; M(2,2) = 1; M(2,3) = 0;
	M(3,0) = 0; M(3,1) = 0; M(3,2) = 0; M(3,3) = 1;
}

void mat_copy(float p[16], const float m[16])
{
	memcpy(p, m, sizeof(float[16]));
}

void mat_mul44(float m[16], const float a[16], const float b[16])
{
	int i;
	for (i = 0; i < 4; i++) {
		const float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		M(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		M(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		M(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		M(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
}

void mat_mul(float m[16], const float a[16], const float b[16])
{
	int i;
	for (i = 0; i < 3; i++) {
		const float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
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

void mat_frustum(float m[16],
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

void mat_ortho(float m[16],
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

void mat_mix(float m[16], const float a[16], const float b[16], float v)
{
	float iv = 1 - v;
	int i;
	for (i = 0; i < 16; i++)
		m[i] = a[i] * iv + b[i] * v;
}

void mat_scale(float m[16], float x, float y, float z)
{
	m[0] *= x; m[4] *= y; m[8] *= z;
	m[1] *= x; m[5] *= y; m[9] *= z;
	m[2] *= x; m[6] *= y; m[10] *= z;
	m[3] *= x; m[7] *= y; m[11] *= z;
}

void mat_translate(float m[16], float x, float y, float z)
{
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

static inline float radians(float degrees)
{
	return degrees * 3.14159265f / 180;
}

void mat_rotate_x(float p[16], float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	float m[16];
	mat_identity(m);
	M(1,1) = c;
	M(2,2) = c;
	M(1,2) = -s;
	M(2,1) = s;
	mat_mul44(p, p, m);
}

void mat_rotate_y(float p[16], float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	float m[16];
	mat_identity(m);
	M(0,0) = c;
	M(2,2) = c;
	M(0,2) = s;
	M(2,0) = -s;
	mat_mul44(p, p, m);
}

void mat_rotate_z(float p[16], float angle)
{
	float s = sin(radians(angle)), c = cos(radians(angle));
	float m[16];
	mat_identity(m);
	M(0,0) = c;
	M(1,1) = c;
	M(0,1) = -s;
	M(1,0) = s;
	mat_mul44(p, p, m);
}

void mat_transpose(float to[16], const float from[16])
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

void mat_invert(float out[16], const float m[16])
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

/* Transform a point (column vector) by a matrix: p = m * v */
void mat_vec_mul(float p[3], const float m[16], const float v[3])
{
	assert(p != v);
	p[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	p[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	p[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}

void mat_vec_mul_n(float p[3], const float m[16], const float v[3])
{
	assert(p != v);
	p[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2];
	p[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2];
	p[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2];
}

/* Transform a normal (row vector) by a matrix:  [px py pz] = v * m */
void mat_vec_mul_t(float p[3], const float m[16], const float v[3])
{
	assert(p != v);
	p[0] = v[0] * m[0] + v[1] * m[1] + v[2] * m[2];
	p[1] = v[0] * m[4] + v[1] * m[5] + v[2] * m[6];
	p[2] = v[0] * m[8] + v[1] * m[9] + v[2] * m[10];
}

void vec_scale(float p[3], const float v[3], float s)
{
	p[0] = v[0] * s;
	p[1] = v[1] * s;
	p[2] = v[2] * s;
}

void vec_add(float p[3], const float a[3], const float b[3])
{
	p[0] = a[0] + b[0];
	p[1] = a[1] + b[1];
	p[2] = a[2] + b[2];
}

void vec_sub(float p[3], const float a[3], const float b[3])
{
	p[0] = a[0] - b[0];
	p[1] = a[1] - b[1];
	p[2] = a[2] - b[2];
}

void vec_lerp(float p[3], const float a[3], const float b[3], float t)
{
	p[0] = a[0] + t * (b[0] - a[0]);
	p[1] = a[1] + t * (b[1] - a[1]);
	p[2] = a[2] + t * (b[2] - a[2]);
}

void vec_average(float p[3], const float a[3], const float b[3])
{
	vec_lerp(p, a, b, 0.5f);
}

void vec_cross(float p[3], const float a[3], const float b[3])
{
	assert(p != a && p != b);
	p[0] = a[1] * b[2] - a[2] * b[1];
	p[1] = a[2] * b[0] - a[0] * b[2];
	p[2] = a[0] * b[1] - a[1] * b[0];
}

float vec_dot(const float a[3], const float b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float vec_dist2(const float a[3], const float b[3])
{
	float d0, d1, d2;
	d0 = a[0] - b[0];
	d1 = a[1] - b[1];
	d2 = a[2] - b[2];
	return d0 * d0 + d1 * d1 + d2 * d2;
}

float vec_dist(const float a[3], const float b[3])
{
	return sqrtf(vec_dist2(a, b));
}

void vec_normalize(float v[3])
{
	float d = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (d >= 0.00001) {
		d = 1 / d;
		v[0] *= d;
		v[1] *= d;
		v[2] *= d;
	} else {
		v[0] = v[1] = 0;
		v[2] = 1;
	}
}

void vec_face_normal(float n[3], const float *p0, const float *p1, const float *p2)
{
	float u[3], v[3];
	vec_sub(u, p1, p0);
	vec_sub(v, p2, p0);
	vec_cross(n, u, v);
}

void vec_yup_to_zup(float v[3])
{
	float z = v[2];
	v[2] = v[1];
	v[1] = -z;
}

void quat_normalize(float q[4])
{
	float d = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	if (d >= 0.00001) {
		d = 1 / d;
		q[0] *= d;
		q[1] *= d;
		q[2] *= d;
		q[3] *= d;
	} else {
		q[0] = q[1] = q[2] = 0;
		q[3] = 1;
	}
}

void quat_lerp(float p[4], const float a[4], const float b[4], float t)
{
	p[0] = a[0] + t * (b[0] - a[0]);
	p[1] = a[1] + t * (b[1] - a[1]);
	p[2] = a[2] + t * (b[2] - a[2]);
	p[3] = a[3] + t * (b[3] - a[3]);
}

void quat_lerp_normalize(float p[4], const float a[4], const float b[4], float t)
{
	quat_lerp(p, a, b, t);
	quat_normalize(p);
}

void quat_lerp_neighbor_normalize(float p[4], float a[4], float b[4], float t)
{
	if (a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3] < 0) {
		float temp[4] = { -a[0], -a[1], -a[2], -a[3] };
		quat_lerp(p, temp, b, t);
	} else
		quat_lerp(p, a, b, t);
	quat_normalize(p);
}

void mat_from_quat_vec(float m[16], const float q[4], const float v[3])
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

	M(0,3) = v[0];
	M(1,3) = v[1];
	M(2,3) = v[2];

	M(3,0) = 0;
	M(3,1) = 0;
	M(3,2) = 0;
	M(3,3) = 1;
}
