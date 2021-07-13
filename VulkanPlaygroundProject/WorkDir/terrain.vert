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
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragViewPos;
layout(location = 4) out float fragHeight;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
} sceneData;

layout(set = 1, binding = 0) uniform ObjectBuffer{   
	mat4 modelMatrix; 
} objectData;

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main() {
    float offset = 1.0f / 256.0f; 
    float height = texture(tex1, inTexCoord).r;
    height += texture(tex1, inTexCoord + vec2(offset,0)).r;
    height += texture(tex1, inTexCoord + vec2(0,offset)).r;
    height += texture(tex1, inTexCoord + vec2(offset,offset)).r;
    height = height / 4.0;
    vec3 position = inPosition;
    position.y = height * 50;


    mat4 modelScene = objectData.modelMatrix;
    mat3 modelSceneMat3 = mat3(modelScene);
    vec4 pos = modelScene * vec4(position, 1.0);
    gl_Position = sceneData.viewProj * pos;

    fragPos = pos.xyz;
    
    fragHeight = height;
    fragColor = vec4(inTexCoord,height,1);
    fragTexCoord = inTexCoord;
	fragViewPos = (sceneData.viewProj * vec4(pos.xyz, 1.0)).xyz;
}