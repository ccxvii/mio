varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	float wind = gl_Color.r;
	float time = gl_MultiTexCoord1.x / 60.0;
	vec4 vertex = gl_Vertex;
	vertex.x += wind * sin(time);
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	texcoord = vec2(gl_MultiTexCoord0);
	vec4 position = gl_ModelViewMatrix * vertex;
	light_dir = normalize(vec3(gl_LightSource[0].position - position));
	gl_FogFragCoord = length(position);
	fogfactor =  clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
}
