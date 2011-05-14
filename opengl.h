/* Every stupid system has a different way to include OpenGL headers... */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#undef near
#undef far
#define GL_GENERATE_MIPMAP GL_GENERATE_MIPMAP_SGIS
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include "GL/glext.h"
#endif
