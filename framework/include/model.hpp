
#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/glm.hpp>
#include <vector> // std::vector

struct Model {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };
    
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    
    std::size_t vertex_offset;
    std::size_t index_offset;
};

// Primitives
Model load_cube();
Model load_plane();

#endif // MODEL_HPP
