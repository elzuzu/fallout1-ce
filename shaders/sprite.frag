#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragPalette;

layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler1D paletteSampler[32];

layout(push_constant) uniform Push
{
    uint paletteOverride;
}
pc;

layout(location = 0) out vec4 outColor;

void main()
{
    float idx = texture(texSampler, fragTexCoord).r * 255.0;
    uint pal = pc.paletteOverride != 0u ? pc.paletteOverride : fragPalette;
    outColor = texelFetch(paletteSampler[pal], int(idx), 0);
}
