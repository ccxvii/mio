uniform sampler2D control_tex;
uniform sampler2DArray tile_tex;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;

void main()
{
	float idx = texture2D(control_tex, texcoord).r * 255.0;
	vec3 tile_tc = vec3(texcoord.s * 1024, texcoord.t * 1024, idx);
	vec4 diffuse = texture2DArray(tile_tex, tile_tc);
//	vec4 diffuse = texture2D(control_tex, texcoord);
// diffuse = vec4(1.0, 1.0, 0.5, 1.0);

	vec3 N = normalize(normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(N, L)*0.7, 0.0, 0.7) + 0.3;
	gl_FragColor = diffuse * term;
}
