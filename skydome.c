#include "mio.h"

#ifndef M_PI
#define M_PI 3.1415926535897
#endif

#define DTOR (M_PI / 180.0)
#define radius 450

static int skyslice = 0;
static int skystrip = 0;
static float *skyxyz = NULL;
static float *skyst = NULL;

void init_sky_dome(int dtheta, int dphi)
{
	int skycount;
	int phi, theta;
	int nv = 0;
	int nt = 0;

	skyslice = (90 / dphi);
	skystrip = (360 / dtheta + 1);
	skycount = skystrip * skyslice * 2;
	skyxyz = malloc(skycount * 3 * sizeof(float));
	skyst = malloc(skycount * 2 * sizeof(float));

	for (phi = 0; phi <= 90 - dphi; phi += dphi)
	{
		for (theta = 0; theta <= 360; theta += dtheta)
		{
			skyxyz[nv++] = sin((phi) * DTOR) * sin((theta) * DTOR) * radius;
			skyxyz[nv++] = sin((phi) * DTOR) * cos((theta) * DTOR) * radius;
			skyxyz[nv++] = cos((phi) * DTOR) * radius;
			skyst[nt++] = 1.0 - theta / 360.0;
			skyst[nt++] = phi / 90.0;

			skyxyz[nv++] = sin((phi+dphi) * DTOR) * sin((theta) * DTOR) * radius;
			skyxyz[nv++] = sin((phi+dphi) * DTOR) * cos((theta) * DTOR) * radius;
			skyxyz[nv++] = cos((phi+dphi) * DTOR) * radius;
			skyst[nt++] = 1.0 - theta / 360.0;
			skyst[nt++] = (phi+dphi) / 90.0;
		}
	}
}

void draw_sky_dome(int skytex)
{
	GLfloat m[16];
	int x, z, i;

	/* Kill the translation component */
	glPushMatrix();
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	glLoadMatrixf(m);
	glTranslatef(0, 0, -radius * 0.03);

	glColor3f(1.0, 1.0, 1.0);

	glBindTexture(GL_TEXTURE_2D, skytex);

	i = 0;
	for (z = 0; z < skyslice; z++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (x = 0; x < skystrip * 2; x++)
		{
			glTexCoord2f(skyst[i*2+0], skyst[i*2+1]);
			glVertex3f(skyxyz[i*3+0], skyxyz[i*3+1], skyxyz[i*3+2]);
			i++;
		}
		glEnd();
	}

	glPopMatrix();
}
