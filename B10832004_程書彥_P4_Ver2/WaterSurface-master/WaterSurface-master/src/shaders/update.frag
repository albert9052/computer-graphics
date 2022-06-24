#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{ 
    float time = 2.0f;
    FragColor = texture(screenTexture, TexCoords);
    //FragColor = texture( screenTexture, TexCoords + 0.005*vec2( sin(time+1024.0*TexCoords.x),cos(time+768.0*TexCoords.y)) ).xyz;
    //FragColor = vec3(1.0 - texture(screenTexture, TexCoords));
}
