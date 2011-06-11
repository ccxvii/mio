#version 120
#extension GL_EXT_texture_array : enable

uniform sampler2D control_tex;
uniform sampler2DArray tile_tex;
uniform sampler2DArray mask_tex;

varying vec3 normal;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

void main()
{
	const float size = 1024.0;
	const float offset = 1.0 / 1024.0;
	vec2 ctc = texcoord / size;

	float s = fract(texcoord.s);
	float t = fract(texcoord.t);

	float idx00 = texture2D(control_tex, ctc).r * 255.0;
	float idx01 = texture2D(control_tex, vec2(ctc.x, ctc.y+offset)).r * 255.0;
	float idx11 = texture2D(control_tex, vec2(ctc.x+offset, ctc.y+offset)).r * 255.0;
	float idx10 = texture2D(control_tex, vec2(ctc.x+offset, ctc.y)).r * 255.0;

	vec4 diff00 = texture2DArray(tile_tex, vec3(texcoord, idx00));
	vec4 diff01 = texture2DArray(tile_tex, vec3(texcoord, idx01));
	vec4 diff11 = texture2DArray(tile_tex, vec3(texcoord, idx11));
	vec4 diff10 = texture2DArray(tile_tex, vec3(texcoord, idx10));

	float mask_st = texture2DArray(mask_tex, vec3(s, t, 0)).r;
	float mask_ts = texture2DArray(mask_tex, vec3(t, s, 0)).r;

	vec4 diffAB = mix(diff00, diff01, t);
	vec4 diffBC = mix(diff10, diff11, t);
	vec4 diffuse = mix(diffAB, diffBC, s);

	vec3 N = normalize(normal);
	vec3 L = normalize(light_dir);
	float term = clamp(dot(N, L)*0.7, 0.0, 0.7) + 0.3;
	gl_FragColor = mix(gl_Fog.color, diffuse * term, fogfactor);
}
