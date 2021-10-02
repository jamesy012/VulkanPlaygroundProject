#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 fragTexCoord;

void main(){
    gl_Position = vec4(inPosition,1.0,1.0);
    fragTexCoord = (inPosition+1) /2;
}