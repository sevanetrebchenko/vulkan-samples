
#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/glm.hpp>
#include <vector> // std::vector

struct Model {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };
    
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
};

// Primitives
Model load_cube();
Model load_sphere();

#endif // MODEL_HPP
