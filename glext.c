#define MIO_GLEXT_C
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

PFNGLCOMPRESSEDTEXIMAGE2DPROC mioCompressedTexImage2D = 0;

PFNGLCREATEPROGRAMPROC mioCreateProgram = 0;
PFNGLCREATESHADERPROC mioCreateShader = 0;
PFNGLSHADERSOURCEPROC mioShaderSource = 0;
PFNGLCOMPILESHADERPROC mioCompileShader = 0;
PFNGLATTACHSHADERPROC mioAttachShader = 0;
PFNGLLINKPROGRAMPROC mioLinkProgram = 0;
PFNGLVALIDATEPROGRAMPROC mioValidateProgram = 0;
PFNGLGETPROGRAMIVPROC mioGetProgramiv = 0;
PFNGLGETPROGRAMINFOLOGPROC mioGetProgramInfoLog = 0;
PFNGLUSEPROGRAMPROC mioUseProgram = 0;

void init_glext(void)
{
	mioCompressedTexImage2D = F(glCompressedTexImage2D);
	mioCreateProgram = F(glCreateProgram);
	mioCreateShader = F(glCreateShader);
	mioShaderSource = F(glShaderSource);
	mioCompileShader = F(glCompileShader);
	mioAttachShader = F(glAttachShader);
	mioLinkProgram = F(glLinkProgram);
	mioValidateProgram = F(glValidateProgram);
	mioGetProgramiv = F(glGetProgramiv);
	mioGetProgramInfoLog = F(glGetProgramInfoLog);
	mioUseProgram = F(glUseProgram);
}

#endif
