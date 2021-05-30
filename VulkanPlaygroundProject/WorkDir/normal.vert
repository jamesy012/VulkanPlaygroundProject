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

void main() {


    mat4 modelScene = mat4(1.0);
    mat3 modelSceneMat3 = mat3(modelScene);
    vec4 pos = modelScene * vec4(inPosition, 1.0);
    gl_Position = modelScene * pos;

    fragPos = pos.xyz;
    fragNormal = normalize(modelSceneMat3 * inNorm);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    fragTangent = vec3(modelScene * vec4(inTangent,0));
}