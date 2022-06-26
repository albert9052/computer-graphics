#version 330 core

in vec3 Normal;
in vec3 Position;
in vec2 TexCoords;

out vec4 color;

uniform vec3 cameraPos;
uniform samplerCube reflectSkybox;

void main()
{
    float ratio = 1.00 / 1.33;
    vec3 I = normalize(Position - cameraPos);
    vec4 reflection = texture(reflectSkybox, reflect(I, normalize(Normal)));
	vec4 refraction = texture(reflectSkybox, refract(I, normalize(Normal), ratio));
    vec4 blue = vec4(0.14, 0.3, 0.35, 1);
    //color = mix(reflection, blue, 0.6);
    color = reflection;
    //color.a = 0.7;
}