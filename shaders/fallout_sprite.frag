#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in flat uint fragTextureIndex;
layout(location = 3) in vec2 fragScreenPos;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 viewMatrix;
    mat4 projMatrix;
    vec2 screenSize;
    vec2 cameraPos;
    float time;
} camera;

layout(set = 0, binding = 1) uniform RenderSettings {
    vec3 ambientColor;
    float ambientIntensity;
    vec3 globalTint;
    float gamma;
    float contrast;
    float brightness;
    float saturation;
    float pixelPerfect;
} settings;

layout(set = 0, binding = 2) uniform sampler2D textures[32];
layout(set = 0, binding = 3) uniform sampler smoothSampler;

layout(location = 0) out vec4 outColor;

// Fallout-style color grading
vec3 falloutColorGrade(vec3 color) {
    // Apply brightness and contrast
    color = ((color - 0.5) * settings.contrast + 0.5) * settings.brightness;

    // Desaturate slightly for that classic Fallout look
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, settings.saturation);

    // Apply global color tint (wasteland brown/yellow)
    color *= settings.globalTint;

    // Gamma correction
    color = pow(color, vec3(1.0 / settings.gamma));

    return color;
}

// Pixel perfect sampling for retro feel
vec4 samplePixelPerfect(sampler2D tex, vec2 uv) {
    if (settings.pixelPerfect > 0.5) {
        // Get texture size
        vec2 texSize = textureSize(tex, 0);

        // Snap to nearest pixel
        vec2 pixelUV = floor(uv * texSize + 0.5) / texSize;
        return texture(tex, pixelUV);
    } else {
        // Use smooth sampling
        return texture(sampler2D(tex, smoothSampler), uv);
    }
}

void main() {
    // Sample the appropriate texture
    vec4 texColor = samplePixelPerfect(textures[fragTextureIndex], fragTexCoord);

    // Discard transparent pixels
    if (texColor.a < 0.01) {
        discard;
    }

    // Apply instance color tint
    vec3 finalColor = texColor.rgb * fragColor.rgb;

    // Apply ambient lighting
    finalColor += settings.ambientColor * settings.ambientIntensity;

    // Apply Fallout-style color grading
    finalColor = falloutColorGrade(finalColor);

    // Preserve original alpha
    float finalAlpha = texColor.a * fragColor.a;

    outColor = vec4(finalColor, finalAlpha);
}
