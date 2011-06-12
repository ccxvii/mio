#version 120

uniform sampler2D texture;

varying vec3 normal;
varying vec4 position;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	vec4 diffuse = texture2D(texture, texcoord);
	vec3 N = normalize(gl_FrontFacing ? normal : -normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(normal, light_dir)*0.6, 0.0, 0.6) + 0.4;
	if (diffuse.a < 0.5) { discard; }
	gl_FragColor = mix(gl_Fog.color, diffuse * term, fogfactor);
}
