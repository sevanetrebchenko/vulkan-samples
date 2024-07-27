
#ifndef CAMERA_HPP
#define CAMERA_HPP

#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan requires depth values to range [0.0, 1.0], not the default [-1.0, 1.0] that OpenGL uses
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

        float get_near_plane_distance() const;
        float get_far_plane_distance() const;
        
        // Camera matrix is projection * view
        glm::mat4 get_view_matrix();
        glm::mat4 get_projection_matrix();
        
        bool is_dirty() const;
        
    private:
        void recalculate();
        
        bool dirty;
        
        float near;
        float far;
        
        glm::vec3 eye; // Eye position
        glm::vec3 look_at;
        glm::vec3 up;
        
        glm::mat4 view;
        glm::mat4 projection;
};

class OrbitCamera {
    public:
        OrbitCamera();
        ~OrbitCamera();
        
        void rotate_left(float d);
        void rotate_right(float d);
        void rotate_up(float d);
        void rotate_down(float d);
        void zoom_in(float d);
        void zoom_out(float d);
        
        glm::mat4 get_view_matrix();
        glm::mat4 get_projection_matrix();
        glm::vec3 get_position();
        
    private:
        void recalculate();
        
        glm::vec3 target;
        glm::vec3 up;
        glm::vec3 eye;
        
        glm::mat4 view;
        glm::mat4 projection;
        
        bool dirty;
        
        float distance; // Distance from target point
        float near;
        float far;

        float azimuth; // For rotating to the side
        float polar; // For rotating up / down
};

#endif // CAMERA_HPP
