#include "mio.h"

#ifdef _WIN32
#include <windows.h>
#define F(n) (void*)wglGetProcAddress(#n)
#else
#ifdef __APPLE__
#define F(n) (n)
#else
#include "GL/glx.h"
#define F(n) (void*)glXGetProcAddressARB((const unsigned char*)#n)
#endif
#endif

PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;

void init_glext(void)
{
	glCompressedTexImage2D = F(glCompressedTexImage2D);
}
