#include "mio.h"

#define SKY_WEST 0
#define SKY_NORTH 1
#define SKY_EAST 2
#define SKY_SOUTH 3
#define SKY_UP 4
#define SKY_DOWN 5

void load_sky_box(int *sky_box, char *fmt)
{
	char name[64];
	char *compass[6] = { "w", "n", "e", "s", "u", "d" };
	int i;

	for (i = 0; i < 6; i++)
	{
		sprintf(name, fmt, compass[i]);
		sky_box[i] = load_texture(name, NULL, NULL, NULL, 0);
		glBindTexture(GL_TEXTURE_2D, sky_box[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

void draw_sky_box(int *sky_box)
{
	float bottom = -0;
	GLfloat m[16];

	glPushMatrix();
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	glLoadMatrixf(m);

	glColor3f(1.0, 1.0, 1.0);

	/* west */
	glBindTexture(GL_TEXTURE_2D, sky_box[SKY_WEST]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-10, -10, 10);
	glTexCoord2f(1.0, 0.0); glVertex3f(-10,	 10, 10);
	glTexCoord2f(1.0, 1.0); glVertex3f(-10,	 10, bottom);
	glTexCoord2f(0.0, 1.0); glVertex3f(-10, -10, bottom);
	glEnd();

	/* north */
	glBindTexture(GL_TEXTURE_2D, sky_box[SKY_NORTH]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-10,	 10, 10);
	glTexCoord2f(1.0, 0.0); glVertex3f( 10,	 10, 10);
	glTexCoord2f(1.0, 1.0); glVertex3f( 10,	 10, bottom);
	glTexCoord2f(0.0, 1.0); glVertex3f(-10,	 10, bottom);
	glEnd();

	/* east */
	glBindTexture(GL_TEXTURE_2D, sky_box[SKY_EAST]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f( 10,	 10, 10);
	glTexCoord2f(1.0, 0.0); glVertex3f( 10, -10, 10);
	glTexCoord2f(1.0, 1.0); glVertex3f( 10, -10, bottom);
	glTexCoord2f(0.0, 1.0); glVertex3f( 10,	 10, bottom);
	glEnd();

	/* south */
	glBindTexture(GL_TEXTURE_2D, sky_box[SKY_SOUTH]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f( 10, -10, 10);
	glTexCoord2f(1.0, 0.0); glVertex3f(-10, -10, 10);
	glTexCoord2f(1.0, 1.0); glVertex3f(-10, -10, bottom);
	glTexCoord2f(0.0, 1.0); glVertex3f( 10, -10, bottom);
	glEnd();

	/* zenith */
	glBindTexture(GL_TEXTURE_2D, sky_box[SKY_UP]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-10, -10,  10);
	glTexCoord2f(1.0, 0.0); glVertex3f( 10, -10,  10);
	glTexCoord2f(1.0, 1.0); glVertex3f( 10,	 10,  10);
	glTexCoord2f(0.0, 1.0); glVertex3f(-10,	 10,  10);
	glEnd();

	glPopMatrix();
}
