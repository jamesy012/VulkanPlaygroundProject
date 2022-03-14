#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


layout (set = 2, binding = 0) uniform sampler2D samplerColorMap;

void main(){
    vec4 color = texture(samplerColorMap, fragTexCoord);
    outColor = color;
}