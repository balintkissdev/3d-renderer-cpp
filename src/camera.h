#ifndef CAMERA_H_
#define CAMERA_H_

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

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
    void look(const float xOffset, const float yOffset);

    [[nodiscard]] glm::mat4 makeViewMatrix() const;

    [[nodiscard]] inline glm::vec3 position() const;
    [[nodiscard]] inline glm::vec2 rotation() const;

private:
    static const glm::vec3 UP_VECTOR;

    glm::vec3 position_;
    glm::vec2 rotation_;  // rotation_.x is also known as yaw, while rotation_.y
                          // is pitch
    glm::vec3 front_;

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
