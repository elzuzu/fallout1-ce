#version 450
layout(location = 0) in vec2 vUV;
layout(location = 1) flat in uint vPal;

layout(set=0,binding=0) uniform sampler2D texIndexed;
layout(set=0,binding=1) uniform sampler1D texPalette[32];

layout(push_constant) uniform Push {
    uint uPaletteIndex;
} uPC;

layout(location = 0) out vec4 outColor;

void main() {
    float idx = texture(texIndexed, vUV).r;
    uint pal = uPC.uPaletteIndex != 0u ? uPC.uPaletteIndex : vPal;
    outColor = texelFetch(texPalette[pal], int(idx * 255.0), 0);
}
