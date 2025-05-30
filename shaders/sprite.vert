#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) flat in uint inPalette;

layout(location = 0) out vec2 vUV;
layout(location = 1) flat out uint vPal;

void main() {
    vUV = inUV;
    vPal = inPalette;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
