
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

// Camera assumes 16:9 aspect ratio, 75 degree FOV
class Camera {
    public:
        Camera(glm::vec3 position = glm::vec3(0.0f, 2.0f, 3.0f));
        ~Camera();
        
        void set_position(glm::vec3 position);
        glm::vec3 get_position() const;
        
        void set_look_direction(glm::vec3 direction);
        glm::vec3 get_look_direction() const;
        
        void set_up_vector(glm::vec3 direction);
        glm::vec3 get_up_vector() const;

        // Camera matrix is projection * view
        glm::mat4 get_view_matrix();
        glm::mat4 get_projection_matrix();
        
    private:
        void recalculate();
        
        bool dirty;
        
        glm::vec3 eye; // Eye position
        glm::vec3 look_at;
        glm::vec3 up;
        
        glm::mat4 view;
        glm::mat4 projection;
};

#endif // CAMERA_HPP
