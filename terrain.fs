uniform sampler2D control_tex;
uniform sampler2DArray tile_tex;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	float idx00 = texture2D(control_tex, texcoord).r * 255.0;
	float idx01 = texture2D(control_tex, vec2(texcoord.s, texcoord.t+1.0/1024.0)).r * 255.0;
	float idx11 = texture2D(control_tex, vec2(texcoord.s+1.0/1024.0, texcoord.t+1.0/1024.0)).r * 255.0;
	float idx10 = texture2D(control_tex, vec2(texcoord.s+1.0/1024.0, texcoord.t)).r * 255.0;

	vec4 diff00 = texture2DArray(tile_tex, vec3(texcoord.s * 1024.0, texcoord.t * 1024.0, idx00));
	vec4 diff01 = texture2DArray(tile_tex, vec3(texcoord.s * 1024.0, texcoord.t * 1024.0, idx01));
	vec4 diff11 = texture2DArray(tile_tex, vec3(texcoord.s * 1024.0, texcoord.t * 1024.0, idx11));
	vec4 diff10 = texture2DArray(tile_tex, vec3(texcoord.s * 1024.0, texcoord.t * 1024.0, idx10));

	float s = fract(texcoord.s * 1024.0);
	float t = fract(texcoord.t * 1024.0);
//	s = smoothstep(0.5, 1.0, s);
//	t = smoothstep(0.5, 1.0, t);

	vec4 diffAB = mix(diff00, diff01, t);
	vec4 diffBC = mix(diff10, diff11, t);
	vec4 diffuse = mix(diffAB, diffBC, s);

	vec3 N = normalize(normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(N, L)*0.7, 0.0, 0.7) + 0.3;
	gl_FragColor = mix(gl_Fog.color, diffuse * term, fogfactor);
}
