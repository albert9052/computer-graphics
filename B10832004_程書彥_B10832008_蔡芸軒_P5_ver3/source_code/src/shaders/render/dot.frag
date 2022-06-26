#version 330 core
in vec2 TexCoords;

out vec4 color;

uniform sampler2D Texture;
uniform float Rand1, Rand2;

void main(void)
{
    vec3 oriColor = vec3(texture2D(Texture, TexCoords));
    float brightness = 0.21*oriColor.r + 0.72*oriColor.g + 0.07*oriColor.b;
    int x=int(TexCoords.x*600)%5, y=int(TexCoords.y*600)%5;
    /*if(Rand1*x + Rand2*y > 0.5){
        if(brightness>0.5){
            oriColor *= 0.7;
        }
        else {
            //oriColor += vec3(0.5, 0.5, 0.5);
            oriColor *= 1.3;
        }
    }*/
    if(true){
        //oriColor += vec3(0.3, 0.3, 0.3);
        //oriColor.y = sqrt(oriColor.y);
        //oriColor.x = sqrt(oriColor.x);
        //oriColor.z = sqrt(oriColor.z);
        oriColor = vec3((1-oriColor.x)/2.0+oriColor.x,(1-oriColor.y)/2.0+oriColor.y,(1-oriColor.z)/2.0+oriColor.z);
    }
    
    color = vec4(oriColor, 1);
}
