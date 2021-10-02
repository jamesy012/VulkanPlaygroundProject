#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec3 fragViewPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
    vec4 viewPos;
    vec4 lightPos;
    vec4 shadowCascadeSplits;
    mat4 shadowCascadeProj[4];
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D tex1;
layout(set = 2, binding = 1) uniform sampler2DArray texCascadeShadow;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadowCoord, vec2 off, uint cascadeIndex)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( texCascadeShadow, vec3(shadowCoord.st + off, cascadeIndex) ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
	ivec2 texDim = textureSize(texCascadeShadow, 0).xy;
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
        {
    		shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	
	}
	return shadowFactor / count;
}

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

	// Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
	for(uint i = 0; i < 4 - 1; ++i) {
		if(-fragViewPos.z < sceneData.shadowCascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}

	//vec4 shadowCoord = (biasMat * ubo.cascadeViewProjMat[cascadeIndex]) * vec4(inPos, 1.0);	
	vec4 shadowCoord = (biasMat * sceneData.shadowCascadeProj[cascadeIndex]) * vec4(fragPos, 1.0);	
    float shadowRes = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
    
    outColor = vec4(ambient + (diffuse +specular) * shadowRes, baseTextureVec4.a);
	//if(fragViewPos.z>0){
    //outColor.rgb = mod(fragViewPos.zzz,1.0);
	//}


	//switch(cascadeIndex) {
	//	case 0 : 
	//		outColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
	//		break;
	//	case 1 : 
	//		outColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
	//		break;
	//	case 2 : 
	//		outColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
	//		break;
	//	case 3 : 
	//		outColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
	//		break;
	//}
}
