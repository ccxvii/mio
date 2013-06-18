#include "mio.h"

int load_material(char *dirname, char *material)
{
	char filename[1024], *s;
	int texture;
	s = strrchr(material, ';');
	if (s) s++; else s = material;
	if (dirname[0]) {
		strlcpy(filename, dirname, sizeof filename);
		strlcat(filename, "/textures/", sizeof filename);
		strlcat(filename, s, sizeof filename);
		strlcat(filename, ".png", sizeof filename);
	} else {
		strlcpy(filename, "textures/", sizeof filename);
		strlcat(filename, s, sizeof filename);
		strlcat(filename, ".png", sizeof filename);
	}
	texture = load_texture(filename, 1);
	if (strstr(material, "clamp;")) {
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	return texture;
}
