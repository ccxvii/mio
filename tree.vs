varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying vec4 color;

void main()
{
	float wind = gl_Color.r;
	color = vec4(wind, wind, wind, 1);
	float time = gl_MultiTexCoord1.x / 60.0;
	gl_Vertex.x += wind * sin(time);
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	texcoord = vec2(gl_MultiTexCoord0);
	vec4 position = gl_ModelViewMatrix * gl_Vertex;
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
}
