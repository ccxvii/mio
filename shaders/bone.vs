#version 120

uniform mat4 ModelView;
uniform mat4 Projection;

uniform mat4 BoneMatrix[80];

attribute vec3 in_Position;
attribute vec3 in_Normal;
attribute vec2 in_TexCoord;
attribute vec4 in_Color;

attribute vec4 in_BlendIndex;
attribute vec4 in_BlendWeight;

varying vec3 ex_Position;
varying vec3 ex_Normal;
varying vec2 ex_TexCoord;
varying vec4 ex_Color;

void main()
{
	vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 normal = vec4(0.0, 0.0, 0.0, 0.0);

	vec4 index = in_BlendIndex;
	vec4 weight = in_BlendWeight;

	for (int i = 0; i < 4; i++) {
		mat4 m = BoneMatrix[int(index.x)];
		position += m * vec4(in_Position, 1.0) * weight.x;
		normal += m * vec4(in_Normal, 0.0) * weight.x;
		index = index.yzwx;
		weight = weight.yzwx;
	}

	position = ModelView * position;
	normal = ModelView * normal;

	gl_Position = Projection * position;
	ex_Position = position.xyz;
	ex_Normal = normal.xyz;
	ex_TexCoord = in_TexCoord;
	ex_Color = in_Color;
}
