#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

/// Decoupling of camera view position and rotation manipulation.
///
/// Application-side logic accepts user input and updates viewing properties
/// through movement and look operations while renderer accesses the resulting
/// view matrix to use for applying Model-View-Projection transformation.
class Camera
{
public:
    Camera(const glm::vec3& position, const glm::vec2& rotation);

    void moveForward(const float deltaTime);
    void moveBackward(const float deltaTime);
    void strafeLeft(const float deltaTime);
    void strafeRight(const float deltaTime);
    void ascend(const float deltaTime);
    void descend(const float deltaTime);

    /// Apply mouse input changes to change camera direction. Offsets are mouse
    /// cursor distances from the center of the view.
    void look(const float xOffset, const float yOffset);

    [[nodiscard]] glm::mat4 calculateViewMatrix() const;

    [[nodiscard]] inline glm::vec3 position() const;
    [[nodiscard]] inline glm::vec2 rotation() const;

private:
    /// Camera location in world coordinate space. Also known as "eye
    /// position".
    glm::vec3 position_;
    /// Rotation elements are stored as Euler angles, then applied to direction
    /// transformation. Looking along X axis (left/right, snapped around Y axis)
    /// is known as "yaw". Looking along Y axis (up/down, snapped around X axis)
    /// us known as "pitch".
    ///
    /// Rolling around Z axis (like an aeroplane or spaceship) is omitted.
    glm::vec2 rotation_;
    /// Direction vector storing the rotations computed from mouse movements.
    /// Determines where the camera should point at. Always normalized and has
    /// length of 1.
    glm::vec3 direction_;

    mutable glm::vec3 cachedPosition_;
    glm::vec2 cachedRotation_;
    mutable glm::vec3 cachedDirection_;
    mutable glm::mat4 cachedView_;

    void updateDirection();
};

inline glm::vec3 Camera::position() const
{
    return position_;
}

inline glm::vec2 Camera::rotation() const
{
    return rotation_;
}

#endif
