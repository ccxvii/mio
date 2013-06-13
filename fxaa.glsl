/* FXAA v3 */

#define LINEAR 1

#define EDGE_THRESHOLD_MIN 0.05
#define EDGE_THRESHOLD 0.125

#define SUBPIX_TRIM (1.0/4.0)
#define SUBPIX_CAP (3.0/4.0)
#define SUBPIX_TRIM_SCALE (1.0/(1.0-SUBPIX_TRIM))

#define SEARCH_STEPS 6
#define SEARCH_THRESHOLD (1.0/4.0)

float luma(vec3 color) {
#if LINEAR
	return sqrt(dot(color.rgb, vec3(0.299, 0.587, 0.114)));
#else
	return dot(color.rgb, vec3(0.299, 0.587, 0.114));
#endif
}

vec4 fxaa(in vec2 pos, sampler2D tex, in vec2 rcpFrame)
{
	float lumaN = luma(textureOffset(tex, pos.xy, ivec2(0, -1)).rgb);
	float lumaW = luma(textureOffset(tex, pos.xy, ivec2(-1, 0)).rgb);
	vec4 rgbyM = texture(tex, pos.xy);
	rgbyM.a = luma(rgbyM.rgb);
	float lumaE = luma(textureOffset(tex, pos.xy, ivec2( 1, 0)).rgb);
	float lumaS = luma(textureOffset(tex, pos.xy, ivec2( 0, 1)).rgb);
	float lumaM = rgbyM.w;

	float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
	float range = rangeMax - rangeMin;
	if (range < max(EDGE_THRESHOLD_MIN, rangeMax * EDGE_THRESHOLD))
		return rgbyM;

	float lumaNW = luma(textureOffset(tex, pos.xy, ivec2(-1,-1)).rgb);
	float lumaNE = luma(textureOffset(tex, pos.xy, ivec2( 1,-1)).rgb);
	float lumaSW = luma(textureOffset(tex, pos.xy, ivec2(-1, 1)).rgb);
	float lumaSE = luma(textureOffset(tex, pos.xy, ivec2( 1, 1)).rgb);

	float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25;
	float rangeL = abs(lumaL - lumaM);
	float blendL = clamp((rangeL / range) - SUBPIX_TRIM, 0.0, 1.0) * SUBPIX_TRIM_SCALE; 
	blendL = min(SUBPIX_CAP, blendL);

	float edgeVert = 
		abs(lumaNW + (-2.0 * lumaN) + lumaNE) +
		2.0 * abs(lumaW + (-2.0 * lumaM) + lumaE ) +
		abs(lumaSW + (-2.0 * lumaS) + lumaSE);
	float edgeHorz = 
		abs(lumaNW + (-2.0 * lumaW) + lumaSW) +
		2.0 * abs(lumaN + (-2.0 * lumaM) + lumaS ) +
		abs(lumaNE + (-2.0 * lumaE) + lumaSE);
	bool horzSpan = edgeHorz >= edgeVert;

	float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
	if(!horzSpan) lumaN = lumaW;
	if(!horzSpan) lumaS = lumaE;
	float gradientN = abs(lumaN - lumaM);
	float gradientS = abs(lumaS - lumaM);
	lumaN = (lumaN + lumaM) * 0.5;
	lumaS = (lumaS + lumaM) * 0.5;

	if (gradientN < gradientS) {
		lumaN = lumaS;
		gradientN = gradientS;
		lengthSign *= -1.0;
	}
	vec2 posN;
	posN.x = pos.x + (horzSpan ? 0.0 : lengthSign * 0.5);
	posN.y = pos.y + (horzSpan ? lengthSign * 0.5 : 0.0);

	gradientN *= SEARCH_THRESHOLD;

	vec2 posP = posN;
	vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0f, rcpFrame.y); 
	float lumaEndN;
	float lumaEndP;
	bool doneN = false;
	bool doneP = false;
	posN += offNP * (-1.5);
	posP += offNP * ( 1.5);
	for(int i = 0; i < SEARCH_STEPS; i++) {
		lumaEndN = luma(texture(tex, posN.xy).rgb);
		lumaEndP = luma(texture(tex, posP.xy).rgb);
		bool doneN2 = abs(lumaEndN - lumaN) >= gradientN;
		bool doneP2 = abs(lumaEndP - lumaN) >= gradientN;
		if(doneN2 && !doneN) posN += offNP;
		if(doneP2 && !doneP) posP -= offNP;
		if(doneN2 && doneP2) break;
		doneN = doneN2;
		doneP = doneP2;
		if(!doneN) posN -= offNP * 2.0;
		if(!doneP) posP += offNP * 2.0; }
		float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
		float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;

		bool directionN = dstN < dstP;
		lumaEndN = directionN ? lumaEndN : lumaEndP;

		if (((lumaM - lumaN) < 0.0) == ((lumaEndN - lumaN) < 0.0)) 
			lengthSign = 0.0;

		float spanLength = (dstP + dstN);
		dstN = directionN ? dstN : dstP;
		float subPixelOffset = 0.5 + (dstN * (-1.0/spanLength));
		subPixelOffset += blendL * (1.0/8.0);
		subPixelOffset *= lengthSign;
		vec3 rgbF = texture(tex, vec2(
					pos.x + (horzSpan ? 0.0 : subPixelOffset),
					pos.y + (horzSpan ? subPixelOffset : 0.0))).xyz;

#if LINEAR
		lumaL *= lumaL;
#endif
		float lumaF = dot(rgbF, vec3(0.299, 0.587, 0.114)) + (1.0/(65536.0*256.0));
		float lumaB = mix(lumaF, lumaL, blendL);
		float scale = min(4.0, lumaB/lumaF);
		rgbF *= scale;
		return vec4(rgbF, lumaM);
}

uniform sampler2D map_color;
uniform vec2 rcp_viewport;
in vec2 var_texcoord;
out vec4 frag_color;
void main() {
	frag_color = fxaa(var_texcoord, map_color, rcp_viewport);
}
