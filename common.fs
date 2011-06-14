#version 120

uniform sampler2D texture;

varying vec3 normal;
varying vec4 position;
varying vec3 light_dir;
varying vec2 texcoord;
varying float fogfactor;

const float ambient = 0.2;
const float shininess = 32.0;

void main()
{
	vec4 diffuse = texture2D(texture, texcoord);
	vec3 N = normalize(gl_FrontFacing ? normal : -normal);
	vec3 L = normalize(light_dir);
	vec3 E = normalize(-vec3(position));
	vec3 R = reflect(-L, N);

	float specular = pow(max(dot(R, E), 0.0), shininess);
	float lambert_term = clamp(dot(N, L), 0.0, 1.0) * (1.0 - ambient) + ambient;
	vec4 final_color = diffuse * lambert_term;
	if (diffuse.a < 1.0)
		final_color += vec4(specular * diffuse.a * 2);
//	final_color = vec4(N*0.5+0.5, 1.0);
	gl_FragColor = mix(gl_Fog.color, final_color, fogfactor);
}
