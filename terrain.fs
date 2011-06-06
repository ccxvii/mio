uniform sampler2D control_tex;
uniform sampler2D tile_tex[4];

varying vec3 normal;
varying vec3 light_dir;
varying vec2 control_tc;
varying vec2 tile_tc;

void main()
{
	vec4 diffuse = texture2D(tile_tex[0], tile_tc);
	vec3 N = normalize(normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(N, L)*0.7, 0.0, 0.7) + 0.3;
	gl_FragColor = diffuse * term;
}
