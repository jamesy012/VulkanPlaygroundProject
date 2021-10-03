#version 450

//Vertex
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNorm;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec2 vertexTexCoord;
//layout(location = 4) in vec3 inTangent;
//layout(location = 5) in vec4 inJointIndex;
//layout(location = 6) in vec4 inJointWeights;

//Instance
layout(location = 4) in vec3 instancePos;

layout(location = 0) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
} sceneData;

void main(){
    //mat4 modelScene = instanceMatrix;
    //mat3 modelSceneMat3 = mat3(modelScene);

    //vec4 pos = modelScene * vec4(vertexPosition, 1.0);
    vec4 pos = vec4(instancePos + (vertexPosition * 0.05f), 1.0);

    gl_Position = sceneData.viewProj * pos;
    fragTexCoord = vertexTexCoord;
}