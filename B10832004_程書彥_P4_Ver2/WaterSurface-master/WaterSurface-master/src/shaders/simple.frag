#version 430 core
#extension GL_NV_shadow_samplers_cube:enable

out vec4 f_color;

in V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
    vec3 cameraPos;
} f_in;

uniform vec3 cameraPosUniformVersion;

uniform int hasHeightMap;

uniform vec3 u_color;

uniform vec3 viewPos;

uniform sampler2D u_texture_0;
uniform sampler2D u_texture_1;

uniform samplerCube skybox;

// Directional light
struct DirLight {
	vec3 direction;
  
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};	
uniform DirLight dirLight;

//vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);  

// Point light
//struct PointLight {	 
//	  vec3 position;
//	  
//	  float constant;
//	  float linear;
//	  float quadratic;	
//
//	  vec3 ambient;
//	  vec3 diffuse;
//	  vec3 specular;
//};  
//#define NR_POINT_LIGHTS 4  
//uniform PointLight pointLights[NR_POINT_LIGHTS];
//
//vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  

/*
void main()
{	
	vec3 color = vec3(texture(u_texture_1, f_in.texture_coordinate));
	f_color = vec4(color, 1.0f);
}
*/
float fresnel(vec3 I, vec3 N, float ior) 
{ 
	float kr;
    float cosi = clamp(-1, 1, dot(I, N)); 
    float etai = 1, etat = ior; 
    if (cosi > 0) {

		float temp = etai;
		etai = etat;
		etat = temp;
	} 
    // Compute sini using Snell's law
    float sint = etai / etat * sqrt(max(0.f, 1 - cosi * cosi)); 
    // Total internal reflection
    if (sint >= 1) { 
        kr = 1; 
    } 
    else { 
        float cost = sqrt(max(0.f, 1 - sint * sint)); 
        cosi = abs(cosi); 
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost)); 
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost)); 
        kr = (Rs * Rs + Rp * Rp) / 2; 
    } 
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
	return kr;
}

void main()
{
	// properties
	//vec3 norm = normalize(f_in.normal);
	//vec3 viewDir = normalize(viewPos - f_in.position);

/*
	DirLight dirLight;
	dirLight.direction = vec3(1.0f, 1.0f, 0.0f);
	dirLight.ambient = vec3(1.0f, 1.0f, 1.0f);
	dirLight.diffuse = vec3(1.0f, 1.0f, 1.0f);
	dirLight.specular = vec3(1.0f, 1.0f, 1.0f);
*/

	// phase 1: Directional lighting
	//vec3 result = CalcDirLight(dirLight, norm, viewDir);
	// phase 2: Point lights
	//for(int i = 0; i < NR_POINT_LIGHTS; i++)
	//	  result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
	// phase 3: Spot light
	//result += CalcSpotLight(spotLight, norm, FragPos, viewDir);	 

    if (hasHeightMap == 1) {

	vec3 incident = normalize(f_in.position - cameraPosUniformVersion);
	vec3 normal = normalize(f_in.normal);
	vec3 reflected = reflect(incident, normal);
	vec4 reflected_color = textureCube(skybox, reflected);
	vec3 refracted = refract(incident, normal, 1.0f / 1.3f);
	vec4 refracted_color = textureCube(skybox, refracted);
	float kr = fresnel(incident, normal, 1.3f);
	f_color = reflected_color * kr;
	f_color += refracted_color * (1.0f - kr);
	f_color = reflected_color;
	//f_color = refracted_color;
	//f_color.x = f_color.x * 0.8f;
	//f_color.y = f_color.y * 0.9f;
	//vec3 test = normalize(cameraPosUniformVersion);
	//f_color = vec4(0.0f, 0.0f, refracted.x, 1.0f);
    }
    else {

	if (f_in.texture_coordinate.x >= 0 && f_in.texture_coordinate.x <= 0.5) {

	    f_color = vec4(0.3f, 0.5f, 0.8f, 1.0f);
	}
	else {

	    vec3 color = vec3(texture(u_texture_0, f_in.texture_coordinate));
	    f_color = vec4(color, 1.0f);
	}
	vec3 color = vec3(texture(u_texture_0, f_in.texture_coordinate));
	f_color = vec4(color, 1.0f);
	//f_color = vec4(f_in.texture_coordinate.x, f_in.texture_coordinate.y, 0.0f, 1.0f);
    }

	//f_color = vec4(result, 1.0);
	//vec3 color = vec3(texture(u_texture_1, f_in.texture_coordinate));
	//f_color = vec4(color, 1.0f);
	//f_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

/*
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// combine results
	vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
	return (ambient + diffuse + specular);
}  

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// attenuation
	float distance	  = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + 
				 light.quadratic * (distance * distance));	  
	// combine results
	vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
	ambient  *= attenuation;
	diffuse  *= attenuation;
	specular *= attenuation;
	return (ambient + diffuse + specular);
} 
*/
