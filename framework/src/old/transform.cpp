
#include "transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

Transform::Transform(glm::vec3 p, glm::vec3 s, glm::vec3 r) : position(p),
                                                              rotation(r),
                                                              scale(s) {
}

Transform::~Transform() {
}

void Transform::set_position(glm::vec3 p) {
    position = p;
    dirty = true;
}

glm::vec3 Transform::get_position() const {
    return position;
}

void Transform::set_scale(glm::vec3 s) {
    scale = s;
    dirty = true;
}

glm::vec3 Transform::get_scale() const {
    return scale;
}

void Transform::set_rotation(glm::vec3 r) {
    rotation = r;
    dirty = true;
}

glm::vec3 Transform::get_rotation() const {
    return rotation;
}

void Transform::recalculate() {
    if (dirty) {
        
        // S * R * T
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::rotate(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 s = glm::scale(scale);
        
        matrix = t * r * s;
        dirty = false;
    }
}

glm::mat4 Transform::get_matrix() {
    recalculate();
    return matrix;
}

Transform& Transform::operator=(const Transform& other) {
    if (this == &other) {
        return *this;
    }
    
    position = other.position;
    scale = other.scale;
    rotation = other.rotation;
    dirty = true;
    
    return *this;
}

bool Transform::is_dirty() const {
    return dirty;
}
