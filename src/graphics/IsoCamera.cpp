#include "IsoCamera.hpp"

void IsoCamera::update()
{
    float azim = glm::radians(azimuthDeg);
    float elev = glm::radians(elevationDeg);
    elev = glm::clamp(elev, glm::radians(1.f), glm::radians(89.f));

    glm::vec3 eye;
    eye.x = target.x + distance * cos(elev) * sin(azim);
    eye.y = target.y + distance * sin(elev);
    eye.z = target.z + distance * cos(elev) * cos(azim);

    view = glm::lookAtRH(eye, target, glm::vec3(0.f, 1.f, 0.f));
    proj = glm::perspectiveRH_ZO(glm::radians(60.f), aspectRatio, 0.1f, 1000.f);
    proj[1][1] *= -1.f;
}
