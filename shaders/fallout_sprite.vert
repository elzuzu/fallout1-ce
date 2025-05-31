#version 450

// Quad vertex data
layout(location = 0) in vec2 inPosition;    // Quad corner (-0.5 to 0.5)
layout(location = 1) in vec2 inTexCoord;    // Quad UV (0.0 to 1.0)

// Per-instance data
layout(location = 2) in vec2 inSpritePos;      // World position
layout(location = 3) in vec2 inSpriteSize;     // Sprite size
layout(location = 4) in vec4 inUVRect;         // UV rect (x, y, w, h)
layout(location = 5) in vec4 inColor;          // Color tint
layout(location = 6) in vec2 inRotationDepth;  // rotation, depth
layout(location = 7) in uint inTextureIndex;   // Texture array index

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 viewMatrix;
    mat4 projMatrix;
    vec2 screenSize;
    vec2 cameraPos;
    float time;
} camera;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out flat uint fragTextureIndex;
layout(location = 3) out vec2 fragScreenPos;

void main() {
    float rotation = inRotationDepth.x;
    float depth = inRotationDepth.y;

    // Apply rotation to quad
    vec2 rotatedPos = inPosition;
    if (rotation != 0.0) {
        float cosR = cos(rotation);
        float sinR = sin(rotation);
        rotatedPos = vec2(
            inPosition.x * cosR - inPosition.y * sinR,
            inPosition.x * sinR + inPosition.y * cosR
        );
    }

    // Scale and position the quad
    vec2 worldPos = inSpritePos + (rotatedPos * inSpriteSize);

    // Apply camera transform
    vec4 viewPos = camera.viewMatrix * vec4(worldPos, depth, 1.0);
    gl_Position = camera.projMatrix * viewPos;

    // Calculate texture coordinates using UV rect
    fragTexCoord = inUVRect.xy + (inTexCoord * inUVRect.zw);

    fragColor = inColor;
    fragTextureIndex = inTextureIndex;

    // Screen position for effects
    fragScreenPos = gl_Position.xy / gl_Position.w;
}
