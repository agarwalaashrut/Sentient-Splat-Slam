#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace sss {

Camera::Camera() 
    : position(0.f, 0.5f, 4.f),
      pitch(0.f),
      yaw(-90.f),
      fov(60.f),
      znear(0.1f),
      zfar(100.f),
      speed(2.5f),
      sensitivity(0.1f) {
}

glm::vec3 Camera::forward() const {
    const float yaw_rad = glm::radians(yaw);
    const float pitch_rad = glm::radians(pitch);
    return glm::normalize(glm::vec3{
        cos(yaw_rad) * cos(pitch_rad),
        sin(pitch_rad),
        sin(yaw_rad) * cos(pitch_rad)
    });
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0.f, 1.f, 0.f)));
}

glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position, position + forward(), up());
}

glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

void Camera::on_mouse_move(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch -= dy * sensitivity;
    pitch = glm::clamp(pitch, -89.f, 89.f);
}

void Camera::on_keyboard(bool W, bool A, bool S, bool D, bool Q, bool E, float dt) {
    float velocity = speed * dt;
    if (W) position += forward() * velocity;
    if (S) position -= forward() * velocity;
    if (A) position -= right() * velocity;
    if (D) position += right() * velocity;
    if (Q) position -= up() * velocity;
    if (E) position += up() * velocity;
}

}  // namespace sss
