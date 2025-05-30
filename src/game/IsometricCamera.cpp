#include "IsometricCamera.h"
#include <cmath> // For sin, cos

namespace fallout {
namespace game {

IsometricCamera::IsometricCamera() {
    // Sensible defaults
    upVector_ = {0.0f, 1.0f, 0.0f}; // Assuming Y is up for the world coordinate system for camera math
                                   // If Z is up in your world, this should be {0,0,1} and rotations adjusted.

    // Typical isometric view angles:
    // Rotate around world Y axis by 45 degrees.
    // Pitch down from horizontal by ~30 to ~35.26 degrees (atan(1/sqrt(2))) for "true" isometric if desired.
    // Let's use PI/4 for Y rotation and PI/6 for X pitch for a common setup.
    rotationAngleY_ = PI / 4.0f;
    pitchAngleX_ = PI / 6.0f;

    zoomLevel_ = 15.0f; // Default distance or ortho view extent
    aspectRatio_ = 16.0f / 9.0f;
    projectionType_ = ProjectionType::ORTHOGRAPHIC;
    orthoHeight_ = 10.0f; // This is half-height for ortho projection
    fovYRadians_ = PI / 4.0f; // 45 degrees FOV for perspective

    nearPlane_ = 0.1f;
    farPlane_ = 200.0f; // Increased far plane for typical game scenes

    RecalculateViewMatrix();
    RecalculateProjectionMatrix();
}

void IsometricCamera::Update(float deltaTime) {
    // For now, updates are driven by explicit calls.
    // This could be used for smoothing, damped movement, etc.
    if (viewDirty_) {
        RecalculateViewMatrix();
        viewDirty_ = false;
    }
    if (projectionDirty_) {
        RecalculateProjectionMatrix();
        projectionDirty_ = false;
    }
}

void IsometricCamera::SetTarget(const Vec3& target) {
    targetPosition_ = target;
    viewDirty_ = true;
}

void IsometricCamera::SetRotationAngle(float angleRad) {
    rotationAngleY_ = angleRad;
    // Normalize angle to 0-2PI range if desired
    // rotationAngleY_ = fmodf(angleRad, 2.0f * PI);
    viewDirty_ = true;
}

void IsometricCamera::SetPitchAngle(float angleRad) {
    pitchAngleX_ = angleRad;
    // Clamp pitch to avoid flipping, e.g., 5 to 85 degrees
    // pitchAngleX_ = std::max(5.0f * PI / 180.0f, std::min(85.0f * PI / 180.0f, angleRad));
    viewDirty_ = true;
}

void IsometricCamera::SetZoom(float zoom) {
    zoomLevel_ = zoom;
    if (zoomLevel_ < 0.1f) zoomLevel_ = 0.1f; // Prevent zoom from becoming zero or negative
    viewDirty_ = true; // If perspective, view matrix changes.
    projectionDirty_ = true; // If orthographic, projection matrix changes.
}

void IsometricCamera::SetAspectRatio(float aspect) {
    aspectRatio_ = aspect;
    projectionDirty_ = true;
}

void IsometricCamera::SetProjectionType(ProjectionType type) {
    projectionType_ = type;
    projectionDirty_ = true;
}

void IsometricCamera::SetOrthographicSize(float width, float height) {
    // This is an alternative to SetZoom for Ortho.
    // For now, let orthoHeight_ be primary control via zoom.
    orthoHeight_ = height / 2.0f; // Assuming 'height' is full view height
    // aspectRatio_ should be width/height
    projectionDirty_ = true;
}

void IsometricCamera::SetPerspectiveFOV(float fovyRad) {
    fovYRadians_ = fovyRad;
    projectionDirty_ = true;
}

void IsometricCamera::Rotate(float angleDeltaRad) {
    rotationAngleY_ += angleDeltaRad;
    viewDirty_ = true;
}

void IsometricCamera::AdjustZoom(float delta) {
    zoomLevel_ += delta;
    if (zoomLevel_ < 0.1f) zoomLevel_ = 0.1f;
    viewDirty_ = (projectionType_ == ProjectionType::PERSPECTIVE); // Zoom affects position for perspective
    projectionDirty_ = (projectionType_ == ProjectionType::ORTHOGRAPHIC); // Zoom affects ortho bounds
}


void IsometricCamera::RecalculateViewMatrix() {
    // Calculate camera position based on target, rotation, pitch, and zoom
    // 1. Start with a vector pointing along the -Z axis (if camera looks down Z)
    //    or +Z if using a right-handed view matrix where -Z is view direction.
    //    Let's assume forward is -Z in view space.
    //    Offset from target:
    float x = 0.0f;
    float y = 0.0f;
    float z = zoomLevel_; // Distance from target

    // Apply pitch (rotation around X-axis, relative to camera's horizontal plane)
    float cosPitch = cosf(pitchAngleX_);
    float sinPitch = sinf(pitchAngleX_);
    float pitchedY = y * cosPitch - z * sinPitch;
    float pitchedZ = y * sinPitch + z * cosPitch;

    y = pitchedY;
    z = pitchedZ;

    // Apply yaw (rotation around world Y-axis)
    float cosYaw = cosf(rotationAngleY_);
    float sinYaw = sinf(rotationAngleY_);
    float yawedX = x * cosYaw + z * sinYaw;
    // y remains 'pitchedY'
    float yawedZ = -x * sinYaw + z * cosYaw;

    position_.x = targetPosition_.x + yawedX;
    position_.y = targetPosition_.y + y; // y is the pitched height component
    position_.z = targetPosition_.z + yawedZ;

    // World Up an_ = {0.0f, 1.0f, 0.0f}; // Or {0,0,1} if Z is up in the world
    // The up vector for LookAt should be the world's up direction.
    // If pitchAngleX_ approaches +/- 90 degrees, this can cause issues.
    // For typical isometric, pitch is constrained so this is fine.
    viewMatrix_ = LookAt(position_, targetPosition_, upVector_);
    viewDirty_ = false;
}

void IsometricCamera::RecalculateProjectionMatrix() {
    if (projectionType_ == ProjectionType::ORTHOGRAPHIC) {
        float orthoViewHeight = orthoHeight_ * zoomLevel_; // Zoom scales the view volume size
        float orthoViewWidth = orthoViewHeight * aspectRatio_;
        projectionMatrix_ = OrthoVulkan(-orthoViewWidth / 2.0f, orthoViewWidth / 2.0f,
                                   -orthoViewHeight / 2.0f, orthoViewHeight / 2.0f,
                                   nearPlane_, farPlane_);
    } else { // PERSPECTIVE
        // For perspective, zoomLevel_ effectively IS the distance, which is handled in RecalculateViewMatrix.
        // FOV remains constant.
        projectionMatrix_ = PerspectiveVulkan(fovYRadians_, aspectRatio_, nearPlane_, farPlane_);
    }
    projectionDirty_ = false;
}

} // namespace game
} // namespace fallout
