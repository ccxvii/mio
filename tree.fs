#version 120

uniform sampler2D texture;

varying vec3 normal;
varying vec4 position;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

const float ambient = 0.2;

void main()
{
	vec4 diffuse = texture2D(texture, texcoord);
	if (diffuse.a < 0.5) discard;
	vec3 N = normalize(gl_FrontFacing ? normal : -normal);
	vec3 L = normalize(light_dir);
	float lambert_term = clamp(dot(N, L), 0.0, 1.0) * (1.0 - ambient) + ambient;
	vec4 final_color = diffuse * lambert_term;
	gl_FragColor = mix(gl_Fog.color, final_color, fogfactor);
}
