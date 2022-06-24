#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_model;

out vec3 TexCoords;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
    vec3 cameraPos;
};

void main()
{
    TexCoords = aPos;
    vec3 position = aPos;
    mat4 view = mat4(mat3(u_view));
    gl_Position = u_projection * view * vec4(position, 1.0);
}
