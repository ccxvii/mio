#version 120

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

attribute vec4 blend_index;
attribute vec4 blend_weight;

uniform mat4 bones[256];

void main()
{
	mat4 m0 = bones[int(blend_index[0])];
	mat4 m1 = bones[int(blend_index[1])];
	mat4 m2 = bones[int(blend_index[2])];
	mat4 m3 = bones[int(blend_index[3])];

	vec4 v = (m0 * gl_Vertex) * blend_weight[0];
	v += (m1 * gl_Vertex) * blend_weight[1];
	v += (m2 * gl_Vertex) * blend_weight[2];
	v += (m3 * gl_Vertex) * blend_weight[3];

	vec3 n = mat3(m0[0].xyz, m0[1].xyz, m0[2].xyz) * gl_Normal;

	gl_Position = gl_ModelViewProjectionMatrix * v;
	normal = normalize(gl_NormalMatrix * n);
	vec4 position = gl_ModelViewMatrix * v;
	texcoord = vec2(gl_MultiTexCoord0);
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
	gl_FogFragCoord = length(position);
	fogfactor = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
}
