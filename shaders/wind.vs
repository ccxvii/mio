#version 120

uniform mat4 ModelView;
uniform mat4 Projection;

uniform vec3 Wind;
uniform float Phase;

attribute vec3 in_Position;
attribute vec3 in_Normal;
attribute vec2 in_TexCoord;
attribute vec4 in_Color;

varying vec3 ex_Position;
varying vec3 ex_Normal;
varying vec2 ex_TexCoord;
varying vec4 ex_Color;

void main()
{
	vec4 position = ModelView * vec4(in_Position, 1.0);
	vec4 normal = ModelView * vec4(in_Normal, 0.0);

	position.xyz += Wind * in_Color.r * Phase;

	gl_Position = Projection * position;
	ex_Position = position.xyz;
	ex_Normal = normal.xyz;
	ex_TexCoord = in_TexCoord;
	ex_Color = in_Color;
}
