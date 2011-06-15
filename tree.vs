#version 120

// per instance matrix is passed in multi tex coords 1-4
// per instance id is passed in multi tex coord 5
// wind phase is passed in mult tex coord 6 (should be part of tex coord 5)
// wind direction should be uniform, but is global

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

const vec4 wind_dir = vec4(-1, 0, 0, 0);

void main()
{
	mat4 instance_matrix = mat4(
		gl_MultiTexCoord1,
		gl_MultiTexCoord2,
		gl_MultiTexCoord3,
		gl_MultiTexCoord4);
	mat3 instance_normal_matrix = mat3(
		vec3(gl_MultiTexCoord1),
		vec3(gl_MultiTexCoord2),
		vec3(gl_MultiTexCoord3));

	float influence = gl_Color.r * gl_MultiTexCoord6.y;
	float phase = gl_MultiTexCoord5.x + gl_MultiTexCoord6.x / 60.0;

	vec4 vertex = instance_matrix * gl_Vertex;
	vertex += wind_dir * influence * sin(phase); // should have better wind function than sin()
	vec4 position = gl_ModelViewMatrix * vertex;
	gl_Position = gl_ProjectionMatrix * position;
	normal = normalize(gl_NormalMatrix * instance_normal_matrix * gl_Normal);
	texcoord = vec2(gl_MultiTexCoord0);
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
	gl_FogFragCoord = length(position);
	fogfactor = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
}
