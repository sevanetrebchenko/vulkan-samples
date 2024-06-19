
#include "camera.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

Camera::Camera(glm::vec3 position) : dirty(true),
                                     eye(position),
                                     look_at(glm::normalize(glm::vec3(0.0f) - eye)),
                                     up(glm::vec3(0.0f, 1.0f, 0.0f)),
                                     view(),
                                     projection() {
}

Camera::~Camera() {
}

void Camera::set_position(glm::vec3 position) {
    eye = position;
    dirty = true;
}

glm::vec3 Camera::get_position() const {
    return eye;
}

void Camera::set_look_direction(glm::vec3 direction) {
    look_at = glm::normalize(direction);
    dirty = true;
}

glm::vec3 Camera::get_look_direction() const {
    return look_at;
}

glm::mat4 Camera::get_view_matrix() {
    if (dirty) {
        recalculate();
    }
    return view;
}

glm::mat4 Camera::get_projection_matrix() {
    if (dirty) {
        recalculate();
    }
    return projection;
}

void Camera::recalculate() {
    view = glm::lookAt(eye, look_at, up);
    
    double fov = 45.0;
    double aspect = 16.0 / 9.0;
    double near = 0.01; // Near plane distance
    double far = 1000.0; // Far plane distance
    
    projection = glm::perspective(glm::radians(fov), aspect, near, far);
    projection[1][1] *= -1; // Account for Vulkan clip coordinate origin starting at the top left
    
    dirty = false;
}

void Camera::set_up_vector(glm::vec3 direction) {
    up = glm::normalize(direction);
    dirty = true;
}

glm::vec3 Camera::get_up_vector() const {
    return up;
}
