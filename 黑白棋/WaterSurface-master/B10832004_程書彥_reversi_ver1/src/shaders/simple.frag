#version 430 core
out vec4 f_color;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
} f_in;

uniform vec3 u_color;

void main()
{   
    vec3 color = u_color;
    f_color = vec4(color, 1.0f);
}
