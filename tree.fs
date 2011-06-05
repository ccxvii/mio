uniform sampler2D texture;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying vec4 color;

void main()
{
	const vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);
	vec4 diffuse = texture2D(texture, texcoord);
	float term = clamp(dot(normal, light_dir)*0.8, 0.0, 0.8) + 0.2;
	gl_FragColor = diffuse * term;
	if (diffuse.a < 0.5) { discard; }
}