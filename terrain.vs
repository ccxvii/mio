varying vec3 normal;
varying vec3 light_dir;
varying vec2 control_tc;
varying vec2 tile_tc;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	control_tc = vec2(gl_MultiTexCoord0);
	tile_tc = vec2(control_tc.s*1024.0, control_tc.t*1024.0);
	vec4 position = gl_ModelViewMatrix * gl_Vertex;
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
}
