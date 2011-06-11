#version 120

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying vec2 tile_st;
varying float fogfactor;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	texcoord = vec2(gl_MultiTexCoord0);
	vec4 position = gl_ModelViewMatrix * gl_Vertex;
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
	gl_FogFragCoord = length(position);
	fogfactor = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
}
