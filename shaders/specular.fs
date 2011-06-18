#version 120

const float Shininess = 48.0;

uniform sampler2D Texture;

uniform vec3 LightDirection;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;
uniform vec3 LightSpecular;

varying vec3 ex_Position;
varying vec3 ex_Normal;
varying vec2 ex_TexCoord;

void main()
{
	vec4 color = texture2D(Texture, ex_TexCoord);
	vec3 N = normalize(gl_FrontFacing ? ex_Normal : -ex_Normal);
	vec3 V = normalize(-ex_Position);
	vec3 H = normalize(LightDirection + V);

	float diffuse = max(dot(N, LightDirection), 0.0);
	float specular = pow(max(dot(N, H), 0.0), Shininess);

	vec3 Ka = color.rgb * LightAmbient;
	vec3 Kd = color.rgb * LightDiffuse * diffuse;
	vec3 Ks = color.rgb * LightSpecular * specular * color.a;

	gl_FragColor = vec4(Ka + Kd + Ks, 1.0);
}
