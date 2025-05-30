#ifndef FALLOUT_GAME_GAME_MATH_H_
#define FALLOUT_GAME_GAME_MATH_H_

#include "graphics/RenderableTypes.h" // For Vec3, Vec4
#include <cmath> // For sin, cos, tan
#include <cstring> // For memcpy

namespace fallout {
namespace game {

const float PI = 3.14159265359f;

struct Mat4 {
    float m[4][4]; // Column-major order like OpenGL: m[col][row]

    Mat4() {
        // Initialize to identity by default
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    static Mat4 Identity() {
        return Mat4();
    }

    // Matrix multiplication (this = this * other)
    Mat4& operator*=(const Mat4& other) {
        Mat4 temp;
        for (int i = 0; i < 4; ++i) { // Row of result
            for (int j = 0; j < 4; ++j) { // Column of result
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) { // Inner product
                    sum += m[k][i] * other.m[j][k]; // Corrected for col-major: this.col[k].row[i] * other.col[j].row[k]
                }
                temp.m[j][i] = sum; // temp.col[j].row[i]
            }
        }
        memcpy(m, temp.m, 16 * sizeof(float));
        return *this;
    }
};

inline Mat4 operator*(Mat4 lhs, const Mat4& rhs) {
    lhs *= rhs;
    return lhs;
}

// --- Matrix Operations ---

inline Mat4 Mat4Translate(const Vec3& v) {
    Mat4 res = Mat4::Identity();
    res.m[3][0] = v.x;
    res.m[3][1] = v.y;
    res.m[3][2] = v.z;
    return res;
}

inline Mat4 Mat4Scale(const Vec3& v) {
    Mat4 res = Mat4::Identity();
    res.m[0][0] = v.x;
    res.m[1][1] = v.y;
    res.m[2][2] = v.z;
    return res;
}

inline Mat4 Mat4RotateY(float angleRad) {
    Mat4 res = Mat4::Identity();
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    res.m[0][0] = c;  res.m[2][0] = s;
    res.m[0][2] = -s; res.m[2][2] = c;
    return res;
}

inline Mat4 Mat4RotateX(float angleRad) {
    Mat4 res = Mat4::Identity();
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    res.m[1][1] = c;  res.m[2][1] = -s;
    res.m[1][2] = s;  res.m[2][2] = c;
    return res;
}


inline Vec3 Normalize(const Vec3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0.0f) return {0,0,0};
    return {v.x / len, v.y / len, v.z / len};
}

inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = Normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f); // Already normalized if f and s are orthonormal

    Mat4 res = Mat4::Identity();
    res.m[0][0] = s.x; res.m[1][0] = s.y; res.m[2][0] = s.z;
    res.m[0][1] = u.x; res.m[1][1] = u.y; res.m[2][1] = u.z;
    res.m[0][2] = -f.x;res.m[1][2] = -f.y;res.m[2][2] = -f.z; // Negative f

    res.m[3][0] = -Dot(s, eye);
    res.m[3][1] = -Dot(u, eye);
    res.m[3][2] = Dot(f, eye); // Positive f.z for right-handed view matrix
    return res;
}

inline Mat4 Ortho(float left, float right, float bottom, float top, float nearZ, float farZ) {
    Mat4 res = Mat4::Identity();
    res.m[0][0] = 2.0f / (right - left);
    res.m[1][1] = 2.0f / (top - bottom);
    res.m[2][2] = -2.0f / (farZ - nearZ); // Vulkan: Z from 0 to 1. OpenGL: -1 to 1
                                        // This is OpenGL style. For Vulkan, adjustments needed.
                                        // Let's stick to OpenGL style for now, Vulkan adaptation is later.
    res.m[3][0] = -(right + left) / (right - left);
    res.m[3][1] = -(top + bottom) / (top - bottom);
    res.m[3][2] = -(farZ + nearZ) / (farZ - nearZ);
    return res;
}
// For Vulkan projection: Z [0,1], Y-inverted compared to OpenGL
inline Mat4 OrthoVulkan(float left, float right, float bottom, float top, float nearZ, float farZ) {
    Mat4 res = Mat4::Identity();
    res.m[0][0] = 2.0f / (right - left);
    res.m[1][1] = 2.0f / (bottom - top); // Flipped Y for Vulkan
    res.m[2][2] = 1.0f / (nearZ - farZ);  // Z range [0, 1] and direction

    res.m[3][0] = -(right + left) / (right - left);
    res.m[3][1] = -(bottom + top) / (bottom - top); // Flipped Y
    res.m[3][2] = nearZ / (nearZ - farZ);
    return res;
}


inline Mat4 Perspective(float fovyRad, float aspect, float nearZ, float farZ) {
    Mat4 res = Mat4::Identity();
    float tanHalfFovy = tanf(fovyRad / 2.0f);
    res.m[0][0] = 1.0f / (aspect * tanHalfFovy);
    res.m[1][1] = 1.0f / (tanHalfFovy);
    res.m[2][2] = -(farZ + nearZ) / (farZ - nearZ); // OpenGL style
    res.m[2][3] = -1.0f;
    res.m[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    res.m[3][3] = 0.0f;
    return res;
}
// For Vulkan projection: Z [0,1], Y-inverted compared to OpenGL
inline Mat4 PerspectiveVulkan(float fovyRad, float aspect, float nearZ, float farZ) {
    Mat4 res = Mat4::Identity();
    float tanHalfFovy = tanf(fovyRad / 2.0f);
    res.m[0][0] = 1.0f / (aspect * tanHalfFovy);
    res.m[1][1] = -1.0f / (tanHalfFovy); // Flipped Y for Vulkan
    res.m[2][2] = farZ / (nearZ - farZ); // Z range [0, 1]
    res.m[2][3] = -1.0f;
    res.m[3][2] = -(farZ * nearZ) / (farZ - nearZ);
    res.m[3][3] = 0.0f;
    return res;
}


} // namespace game
} // namespace fallout
#endif // FALLOUT_GAME_GAME_MATH_H_
