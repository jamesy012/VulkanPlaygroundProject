#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec4 inJointIndex;
layout(location = 6) in vec4 inJointWeights;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec4 fragShadowCoord;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
    vec4 viewPos;
    vec4 lightPos;
    mat4 lightProj;
} sceneData;

layout(set = 1, binding = 0) uniform ObjectBuffer{   
	mat4 modelMatrix; 
} objectData;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    mat4 modelScene = objectData.modelMatrix;
    mat3 modelSceneMat3 = mat3(modelScene);
    vec4 pos = modelScene * vec4(inPosition, 1.0);
    gl_Position = sceneData.viewProj * pos;

    fragPos = pos.xyz;
    fragNormal = normalize(modelSceneMat3 * inNorm);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    fragTangent = vec3(modelScene * vec4(inTangent,0));

    fragShadowCoord = ( biasMat * sceneData.lightProj * modelScene) * vec4(inPosition, 1.0);	
}