#version 120

uniform mat4 bone_matrix[256];

attribute vec4 blend_index;
attribute vec4 blend_weight;

varying vec3 normal;
varying vec4 position;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	vec4 v = vec4(0.0);
	vec3 n = vec3(0.0);

	vec4 cur_index = blend_index;
	vec4 cur_weight = blend_weight;

	for (int i = 0; i < 4; i++) {
		mat4 m44 = bone_matrix[int(cur_index.x)];
		mat3 m33 = mat3(m44[0].xyz, m44[1].xyz, m44[2].xyz);
		v += m44 * gl_Vertex * cur_weight.x;
		n += m33 * gl_Normal * cur_weight.x;
		cur_index = cur_index.yzwx;
		cur_weight = cur_weight.yzwx;
	}

	gl_Position = gl_ModelViewProjectionMatrix * v;
	position = gl_ModelViewMatrix * v;
	normal = normalize(gl_NormalMatrix * n);
	texcoord = vec2(gl_MultiTexCoord0);
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
	gl_FogFragCoord = length(position);
	fogfactor = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
}
