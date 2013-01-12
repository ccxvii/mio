#include "mio.h"

int load_material(char *dirname, char *material)
{
	char filename[1024], *s;
	s = strrchr(material, '+');
	if (s) s++; else s = material;
	if (dirname[0]) {
		strlcpy(filename, dirname, sizeof filename);
		strlcat(filename, "/", sizeof filename);
		strlcat(filename, s, sizeof filename);
		strlcat(filename, ".png", sizeof filename);
	} else {
		strlcpy(filename, s, sizeof filename);
		strlcat(filename, ".png", sizeof filename);
	}
	return load_texture(filename, 1);
}
