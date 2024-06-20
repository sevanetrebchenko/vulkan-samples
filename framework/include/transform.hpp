
#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>

class Transform {
    public:
        Transform(glm::vec3 p = glm::vec3(0.0f), glm::vec3 s = glm::vec3(1.0f), glm::vec3 r = glm::vec3(0.0f));
        ~Transform();
        
        Transform& operator=(const Transform& other);
        
        void set_position(glm::vec3 p);
        glm::vec3 get_position() const;
        
        void set_scale(glm::vec3 s);
        glm::vec3 get_scale() const;
        
        void set_rotation(glm::vec3 r);
        glm::vec3 get_rotation() const;
        
        glm::mat4 get_matrix();
        
        bool is_dirty() const;

    private:
        void recalculate();
        
        bool dirty;
        
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation; // Stored in degrees
        
        glm::mat4 matrix;
};

#endif // TRANSFORM_HPP
