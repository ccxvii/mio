varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	texcoord = vec2(gl_MultiTexCoord0);
	vec4 position = gl_ModelViewMatrix * gl_Vertex;
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
}
