#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragViewPos;
layout(location = 4) in float fragHeight;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
    vec4 viewPos;
    vec4 lightPos;
    vec4 shadowCascadeSplits;
    mat4 shadowCascadeProj[4];
} sceneData;

layout(set = 2, binding = 1) uniform sampler2DArray texCascadeShadow;
layout(set = 2, binding = 2) uniform sampler2DArray texTerrainTextures;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadowCoord, vec2 off, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005f;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( texCascadeShadow, vec3(shadowCoord.st + off, cascadeIndex) ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias ) 
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

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

vec2 rotateUV(vec2 uv, float rotation, vec2 mid)
{
    float cosAngle = cos(rotation);
    float sinAngle = sin(rotation);
    return vec2(
        cosAngle * (uv.x - mid.x) + sinAngle * (uv.y - mid.y) + mid.x,
        cosAngle * (uv.y - mid.y) - sinAngle * (uv.x - mid.x) + mid.y
    );
}

void main() {
    //outColor = fragColor;
outColor = vec4(0,0,0,1);
	vec3 heights[3] = {{0,0.2, 0.2},{0,0.1, 0.3},{0.6,0.2,0.3}};
	float uvScales[3] = {16, 6,8};
	float uvRotates[3] = {0, 0.5, 2};
	for(uint i = 0;i<3;i++) {
	float center = heights[i].x;
    float width = heights[i].y;
    float fade = heights[i].z;
		float widthFade = width + fade;
		//float maskCenter;
    	//maskCenter = step(fragHeight, center + width); 
    	//maskCenter *= step(center - width, fragHeight);

    	//float val = (1.0-abs(fragHeight+center+width))/fade;
    	//float val2 = (1.0-abs((1.0-fragHeight)+center+width))/fade;
		//
    	//val = clamp(val, 0.0, 1.0);
    	//val2 = clamp(val2, 0.0, 1.0);
		//float value = 1.0-(val + val2) + maskCenter;

    float val = map(fragHeight, center-widthFade,center, 0.0, 1.0); 
    float val2 = map(fragHeight, center,center+widthFade, 1.0, 0.0); 
float value = val * val2;
	value = clamp(value, 0, 1);
		//float value = clamp(fragHeight - heights[i].x,0,heights[i].y) * 1/heights[i].y;
		//float value = clamp(fragHeight, 0, heights[i].y);
		outColor += value * texture(texTerrainTextures, vec3(rotateUV(fragTexCoord,uvRotates[i], vec2(0.5))*uvScales[i], i));
		//outColor = value.rrrr;
	}

    outColor *= 1-clamp(distance(sceneData.viewPos.xyz, fragPos) / 150.0f, 0, 1);

	//// Get cascade index for the current fragment's view position
	//uint cascadeIndex = 0;
	//for(uint i = 0; i < 4 - 1; ++i) {
	//	if(-fragViewPos.z < sceneData.shadowCascadeSplits[i]) {	
	//		cascadeIndex = i + 1;
	//	}
	//}

	////vec4 shadowCoord = (biasMat * ubo.cascadeViewProjMat[cascadeIndex]) * vec4(inPos, 1.0);	
	//vec4 shadowCoord = (biasMat * sceneData.shadowCascadeProj[cascadeIndex]) * vec4(fragPos, 1.0);	
    //float shadowRes = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
	//outColor *= clamp(shadowRes,0.5f,1.0f);
}
