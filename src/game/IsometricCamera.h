#ifndef FALLOUT_GAME_ISOMETRIC_CAMERA_H_
#define FALLOUT_GAME_ISOMETRIC_CAMERA_H_

#include "GameMath.h" // For Mat4, Vec3

namespace fallout {
namespace game {

enum class ProjectionType {
    ORTHOGRAPHIC,
    PERSPECTIVE
};

class IsometricCamera {
public:
    IsometricCamera();

    void Update(float deltaTime); // For smooth transitions or damped movement if needed

    // --- Configuration ---
    void SetTarget(const Vec3& target);
    void SetRotationAngle(float angleRad); // Angle around Y axis (or engine's 'up')
    void SetPitchAngle(float angleRad);    // Angle for isometric view (e.g. 30-45 deg from horizontal)
    void SetZoom(float zoom);              // Distance from target or ortho view size
    void SetAspectRatio(float aspect);
    void SetProjectionType(ProjectionType type);
    void SetOrthographicSize(float width, float height); // Or a single 'viewSize' for ortho
    void SetPerspectiveFOV(float fovyRad); // Field of View Y for perspective

    // --- Actions ---
    void Rotate(float angleDeltaRad);
    void AdjustZoom(float delta);

    // --- Accessors ---
    const Mat4& GetViewMatrix() const { return viewMatrix_; }
    const Mat4& GetProjectionMatrix() const { return projectionMatrix_; }
    const Vec3& GetPosition() const { return position_; }
    float GetRotationAngle() const { return rotationAngleY_; }

private:
    void RecalculateViewMatrix();
    void RecalculateProjectionMatrix();

    Vec3 targetPosition_ = {0.0f, 0.0f, 0.0f};
    Vec3 position_ = {0.0f, 0.0f, 0.0f}; // Actual camera position in world space
    Vec3 upVector_ = {0.0f, 1.0f, 0.0f}; // Assuming Y is up for camera math first

    // Angles defining the isometric view relative to the target
    float rotationAngleY_ = PI / 4.0f; // Rotation around the world's Up-axis (e.g. Y or Z)
                                       // Initial angle often 45 degrees for isometric.
    float pitchAngleX_ = PI / 6.0f;    // Angle to look down, e.g. 30 degrees from horizontal.
                                       // A fixed value for classic isometric.

    float zoomLevel_ = 10.0f;          // For ortho: half-height of view volume. For perspective: distance.

    ProjectionType projectionType_ = ProjectionType::ORTHOGRAPHIC;
    float aspectRatio_ = 16.0f / 9.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 100.0f;

    // Orthographic projection params
    float orthoHeight_ = 10.0f; // When zoomLevel_ is 1.0, this is the half-height. Scaled by zoom.

    // Perspective projection params
    float fovYRadians_ = PI / 4.0f; // e.g. 45 degrees FOV

    Mat4 viewMatrix_;
    Mat4 projectionMatrix_;

    bool viewDirty_ = true;
    bool projectionDirty_ = true;
};

} // namespace game
} // namespace fallout

#endif // FALLOUT_GAME_ISOMETRIC_CAMERA_H_
