#version 460
#extension GL_EXT_buffer_reference : require
layout(buffer_reference, buffer_reference_align = 16) readonly buffer FrameBlock { mat4 viewProj; vec4 tint; };
layout(push_constant) uniform PC { uint64_t addr; } pc;
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
layout(location=0) out vec2 fragUV;
void main(){
    FrameBlock f = FrameBlock(pc.addr);
    fragUV = uv;
    gl_Position = f.viewProj * vec4(pos,0.0,1.0);
}
