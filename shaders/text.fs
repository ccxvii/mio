#version 120

uniform sampler2D Texture;

varying vec2 ex_TexCoord;
varying vec4 ex_Color;

void main()
{
	float a = texture2D(Texture, ex_TexCoord).a;
	gl_FragColor = vec4(ex_Color.rgb, ex_Color.a * a);
}
