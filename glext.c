#include "mio.h"

#ifdef __APPLE__
void init_glext(void) { }
#else

#ifdef _WIN32
#include <windows.h>
#define F(n) (void*)wglGetProcAddress(#n)
#else
#include "GL/glx.h"
#define F(n) (void*)glXGetProcAddressARB((const unsigned char*)#n)
#endif

PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;

void init_glext(void)
{
	glCompressedTexImage2D = F(glCompressedTexImage2D);
}

#endif
