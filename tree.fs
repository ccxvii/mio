uniform sampler2D texture;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;

void main()
{
	vec4 diffuse = texture2D(texture, texcoord);
	if (!gl_FrontFacing) normal = -1 * normal;
	float term = clamp(dot(normal, light_dir)*0.6, 0.0, 0.6) + 0.4;
	if (diffuse.a < 0.5) { discard; }
	gl_FragColor = diffuse * term;
}
