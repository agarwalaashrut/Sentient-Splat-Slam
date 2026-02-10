#pragma once

#include <glm/glm.hpp>

namespace sss {

class Camera {
public:
    Camera();
    ~Camera() = default;

    // Properties
    glm::vec3 position;
    float pitch;
    float yaw;
    float fov;
    float znear;
    float zfar;
    float speed;
    float sensitivity;

    // Getters
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;

    // Input handling
    void on_mouse_move(float dx, float dy);
    void on_keyboard(bool W, bool A, bool S, bool D, bool Q, bool E, float dt);
};

}  // namespace sss
