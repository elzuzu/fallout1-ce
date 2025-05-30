#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct IsoCamera {
    glm::vec3 target   = {0.f, 0.f, 0.f};
    float     distance = 10.f;
    float     azimuthDeg   = 45.f;
    float     elevationDeg = 35.f;
    float     aspectRatio  = 16.f/9.f;
    glm::mat4 view{1.f};
    glm::mat4 proj{1.f};
    void update();
    [[nodiscard]] glm::mat4 getViewProj() const { return proj * view; }
};
