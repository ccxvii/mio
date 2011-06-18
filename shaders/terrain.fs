#version 120
#extension GL_EXT_texture_array : enable

uniform sampler2D Control;
uniform sampler2DArray Tile;
uniform sampler2DArray Mask;

uniform vec3 LightDirection;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;

varying vec3 ex_Normal;
varying vec2 ex_TexCoord;

void main()
{
	const float size = 1024.0;
	const float offset = 1.0 / 1024.0;
	vec2 ctc = ex_TexCoord / size;

	float s = fract(ex_TexCoord.s);
	float t = fract(ex_TexCoord.t);

	float idx00 = texture2D(Control, ctc).r * 255.0;
	float idx01 = texture2D(Control, vec2(ctc.x, ctc.y+offset)).r * 255.0;
	float idx11 = texture2D(Control, vec2(ctc.x+offset, ctc.y+offset)).r * 255.0;
	float idx10 = texture2D(Control, vec2(ctc.x+offset, ctc.y)).r * 255.0;

	vec3 tile00 = texture2DArray(Tile, vec3(ex_TexCoord, idx00)).rgb;
	vec3 tile01 = texture2DArray(Tile, vec3(ex_TexCoord, idx01)).rgb;
	vec3 tile11 = texture2DArray(Tile, vec3(ex_TexCoord, idx11)).rgb;
	vec3 tile10 = texture2DArray(Tile, vec3(ex_TexCoord, idx10)).rgb;

//	float mask_st = texture2DArray(Mask, vec3(s, t, 0)).r;
//	float mask_ts = texture2DArray(Mask, vec3(t, s, 0)).r;

	vec3 tileAB = mix(tile00, tile01, t);
	vec3 tileBC = mix(tile10, tile11, t);
	vec3 color = mix(tileAB, tileBC, s);

	vec3 N = normalize(ex_Normal);

	float diffuse = max(dot(N, LightDirection), 0.0);

	vec3 Ka = color.rgb * LightAmbient;
	vec3 Kd = color.rgb * LightDiffuse * diffuse;

	gl_FragColor = vec4(Ka + Kd, 1.0);
}
