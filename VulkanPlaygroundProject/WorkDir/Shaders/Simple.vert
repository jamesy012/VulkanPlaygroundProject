#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec4 inJointIndex;
layout(location = 6) in vec4 inJointWeights;

layout(location = 0) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
} sceneData;

layout(set = 1, binding = 0) uniform ObjectBuffer{   
	mat4 modelMatrix; 
} objectData;

void main(){
    mat4 modelScene = objectData.modelMatrix;
    mat3 modelSceneMat3 = mat3(modelScene);
    vec4 pos = modelScene * vec4(inPosition, 1.0);
    gl_Position = sceneData.viewProj * pos;
    fragTexCoord = inTexCoord;
}