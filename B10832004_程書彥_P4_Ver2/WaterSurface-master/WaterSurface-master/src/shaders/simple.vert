#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

uniform mat4 u_model;

const float pi = 3.14159;
uniform float waterHeight;
uniform float time;
uniform float amplitude;
uniform float waveLength;
uniform float speed;
uniform vec2 direction;
uniform sampler2D u_texture_0;
uniform sampler2D u_texture_1;
uniform int mode;
uniform vec2 vecSize;
uniform int hasHeightMap;
uniform samplerCube skybox;

float wave(float x, float y) {

    float frequency = 2*pi/waveLength;
    float phase = speed * frequency;
    float theta = dot(direction, vec2(x, y));
    return amplitude * sin(mod(theta * frequency + time * phase, pi * 2));
}

float waveHeight(float x, float y) {

    float height = wave(x, y);
    return height;
}

float dWavedx(float x, float y) {

    float frequency = 2*pi/waveLength;
    float phase = speed * frequency;
    float theta = dot(direction, vec2(x, y));
    float A = amplitude * direction.x * frequency;
    return A * cos(theta * frequency + time * phase);
}

float dWavedy(float x, float y) {

    float frequency = 2*pi/waveLength;
    float phase = speed * frequency;
    float theta = dot(direction, vec2(x, y));
    float A = amplitude * direction.y * frequency;
    return A * cos(mod(theta * frequency + time * phase, pi * 2));
}

vec3 waveNormal(float x, float y) {

    float dx = dWavedx(x, y);
    float dy = dWavedy(x, y);
    vec3 n = vec3(-dx, 1.0, -dy);
    return normalize(n);
}

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
    vec3 cameraPos;
};

out V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
    vec3 cameraPos;
} v_out;

const ivec3 off = ivec3(-1,0,1);

void main()
{

    float temp = position.y;
    //if (waveHeight(position.x, position.z) < 0.0f) {

    //    temp = 30;
    //}
    //if (waveHeight(position.x, position.z) > 0.0f) {

    //    temp = 30.0;
    //}

    if (mode == 0) {

        v_out.position = vec3(u_model * vec4(vec3(position.x, waterHeight + waveHeight(position.x, position.z), position.z), 1.0f));
        //v_out.position = vec3(u_model * vec4(position, 1.0f));
        v_out.normal = mat3(transpose(inverse(u_model))) * waveNormal(position.x, position.z);
        //v_out.normal = mat3(transpose(inverse(u_model))) * normal;
        v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);

        gl_Position = u_projection * u_view * vec4(v_out.position, 1.0f);
    }
    else if (mode == 1) {

	if (hasHeightMap == 1) {

		v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
		vec4 wave = texture(u_texture_0, texture_coordinate);
		float s11 = wave.x;
		float s01 = textureOffset(u_texture_0, v_out.texture_coordinate, off.xy).x;
		float s21 = textureOffset(u_texture_0, v_out.texture_coordinate, off.zy).x;
		float s10 = textureOffset(u_texture_0, v_out.texture_coordinate, off.yx).x;
		float s12 = textureOffset(u_texture_0, v_out.texture_coordinate, off.yz).x;
		vec3 va = normalize(vec3(vecSize.xy, s21 - s01));
		vec3 vb = normalize(vec3(vecSize.yx, s12 - s10));
		vec4 bump = vec4(cross(va, vb), s11 );

		v_out.position = vec3(u_model * vec4(vec3(position.x, s11, position.z), 1.0f));
		v_out.normal = mat3(transpose(inverse(u_model))) * bump.xyz * (-1);
		v_out.cameraPos = cameraPos;
	}
	else {

		v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
		v_out.position = vec3(u_model * vec4(vec3(position.x, position.y, position.z), 1.0f));
		v_out.normal = mat3(transpose(inverse(u_model))) * normal;
		v_out.cameraPos = cameraPos;
	}
        
        gl_Position = u_projection * u_view * vec4(v_out.position, 1.0f);
    }
}
