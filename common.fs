#version 120

uniform sampler2D texture;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	vec4 diffuse = texture2D(texture, texcoord);
	vec3 N = normalize(normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(N, L)*0.7, 0.0, 0.7) + 0.3;
	gl_FragColor = mix(gl_Fog.color, diffuse * term, fogfactor);
}
