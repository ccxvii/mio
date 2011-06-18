#version 120

#define ALPHA_TEST 0.5

uniform sampler2D Texture;

uniform vec3 LightDirection;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;

varying vec3 ex_Normal;
varying vec2 ex_TexCoord;
varying vec4 ex_Color;

void main()
{
	vec4 color = texture2D(Texture, ex_TexCoord);

#ifdef ALPHA_TEST
	if (color.a < ALPHA_TEST)
		discard;
#endif

	vec3 N = normalize(gl_FrontFacing ? ex_Normal : -ex_Normal);

	float diffuse = max(dot(N, LightDirection), 0.0);

	vec3 Ka = color.rgb * LightAmbient;
	vec3 Kd = color.rgb * LightDiffuse * diffuse;

	gl_FragColor = vec4(Ka + Kd, color.a);
}
