#version 450
layout(binding = 0) uniform sampler2D indexTex;
layout(binding = 1) uniform sampler1D paletteLut;
layout(push_constant) uniform PC { float gamma; } pc;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main(){
    float idx = texture(indexTex, uv).r * 255.0;
    vec4 c = texelFetch(paletteLut, int(idx), 0);
    c.rgb = pow(c.rgb, vec3(pc.gamma));
    color = c;
}
