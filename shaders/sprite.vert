#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in uint inPalette;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint fragPalette;

layout(push_constant) uniform PushConstants
{
    mat4 mvpMatrix;
    vec2 spriteOffset;
    vec2 spriteScale;
}
pc;

void main()
{
    vec2 worldPos = inPosition * pc.spriteScale + pc.spriteOffset;
    gl_Position = pc.mvpMatrix * vec4(worldPos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragPalette = inPalette;
}
