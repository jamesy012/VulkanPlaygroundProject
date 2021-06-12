#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewproj; 
    vec4 viewPos;
    vec4 lightPos;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main() {
    vec4 baseTextureVec4 = texture(tex1, fragTexCoord);
    vec3 baseTexture = baseTextureVec4.xyz;

    vec3 normal = fragNormal;
    //outColor = vec4(fragTexCoord,0,1);

    float ambientStrength = 0.1;
    vec3 lightColor = vec3(1,1,1);
    vec3 ambient = ambientStrength * lightColor * baseTexture;

    vec3 lightPos = sceneData.lightPos.xyz;//vec3(0,10,10);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normal,lightDir), 0.0f);
    vec3 diffuse = diff * lightColor* baseTexture;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(sceneData.viewPos.xyz - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor/* * roughness*/; 
    
    outColor = vec4(ambient + (diffuse +specular), baseTextureVec4.a);

}
