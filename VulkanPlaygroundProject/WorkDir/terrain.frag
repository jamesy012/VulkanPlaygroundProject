#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
    vec4 viewPos;
} sceneData;

void main() {
    outColor = fragColor;
    outColor *= 1-clamp(distance(sceneData.viewPos.xyz, fragPos) / 100.0f, 0, 1);
}
