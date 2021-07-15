#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D mainRT;
layout(set = 0, binding = 1) uniform sampler2D mainDepth;

layout(push_constant) uniform PushConsts {
	int enabled;
	float timer;
} pushConsts;

float _blend(float val, float val0, float val1, float res0, float res1){
    if( val <= val0 ) return res0;
    if( val >= val1 ) return res1;
    return res0 + (val - val0) * (res1 - res0) / (val1 - val0);
}

float LinearizeDepth(float depth)
{
  float n = 0.1; // camera z near
  float f = 150.0; // camera z far
  return (2.0 * n) / (f + n - depth * (f - n));	
}

void main(){

    vec2 rtSize = textureSize(mainRT, 0).xy;
    vec2 inTex0 = vec2(fragTexCoord) * rtSize;
    inTex0 += 0.5/rtSize;
    vec2 inSPos = fragTexCoord * rtSize;

    vec4 backColor = texture(mainRT, inTex0/rtSize);
    vec4 backDepth = texture(mainDepth, inTex0/rtSize);
    float len = backDepth.r;//length(backDepth);

    bool isGreen =  backColor.g > backColor.r + 0.01f && 
                    backColor.g > backColor.b + 0.01f;

    if(isGreen && bool(pushConsts.enabled))
    {
        vec4 color = vec4(0);
        float xx = inSPos.x / rtSize.x;
        float yy = inSPos.y / rtSize.y;

        float depth = LinearizeDepth(len) * 150;
        float d = _blend(depth, 0, 150, 50, 100);
        //float dClose = _blend(depth, 0, 20, 30, 1);

        //d*=dClose;
        yy+=xx*1000;
        
        yy+=pushConsts.timer * 0.004f;

        float yOffset = fract(yy * d) / d;
        vec2 uvOffset = fragTexCoord - vec2(0, yOffset);
        color = texture(mainRT, uvOffset);

        vec4 poscs2 = texture(mainDepth, uvOffset);
        //if(poscs2.z < backDepth.z) {
        //    outColor = backColor.rrrr;
        //}else{
            outColor = mix(backColor, color, clamp(1-yOffset * d / 3.8, 0, 1));
        //}
    }else{
        outColor = backColor;
    }

    //outColor = texture(mainRT, fragTexCoord);
    //outColor = len.xxxx;
    //outColor = LinearizeDepth(len).xxxx;
}