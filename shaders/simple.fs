#version 120

uniform sampler2D Texture;

uniform vec3 LightDirection;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;

varying vec3 ex_Normal;
varying vec2 ex_TexCoord;

void main()
{
	vec4 color = texture2D(Texture, ex_TexCoord);
	vec3 N = normalize(gl_FrontFacing ? ex_Normal : -ex_Normal);

	float diffuse = max(dot(N, LightDirection), 0.0);

	vec3 Ka = color.rgb * LightAmbient;
	vec3 Kd = color.rgb * LightDiffuse * diffuse;

	gl_FragColor = vec4(Ka + Kd, 1.0);
}
