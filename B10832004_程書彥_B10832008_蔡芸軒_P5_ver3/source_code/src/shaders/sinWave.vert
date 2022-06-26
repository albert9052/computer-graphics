#version 330 core
layout (location = 0) in vec3 position;

out vec3 Normal;
out vec3 Position;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float height, amplitude, wavelength, time, speed;

void main()
{
    vec3 pos = position;
    float k = 2 * 3.14159 / wavelength * 100;
    float f = k * (pos.x - speed * time / 200);
    pos.y = amplitude * sin(f) + height;
    vec3 tangent = normalize(vec3(1,k*amplitude*cos(f),0));
    Normal = normalize(vec3(-cos(f), 1, 0));
    gl_Position = projection * view * model * vec4(pos, 1.0f);
    Position = vec3(model * vec4(pos, 1.0f));
    TexCoords = vec2(position.x, position.z);
}