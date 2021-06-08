#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D tex1;

void main() {
    vec4 color = texture(tex1, fragTexCoord);
    outColor = color;
    //outColor = vec4(fragTexCoord,0,1);
}
