#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 TexCoords;

void main()
{
    TexCoords = aPos * 0.5f + 0.5f;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
} 
//void main(void)
//{
//  gl_Position = ftransform();
//  gl_TexCoord[0] = gl_MultiTexCoord0;
//}
