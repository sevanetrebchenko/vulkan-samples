
#ifndef GLTF_HPP
#define GLTF_HPP

//#include "model.hpp"

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    
    std::size_t vertex_offset;
    std::size_t index_offset;
};

struct Texture {
};

Texture* load_texture(const char* path);

struct Material {
};

struct PBRMaterial : Material {
    Texture* albedo;
    Texture* normals;
    Texture* ambient_occlusion;
    
    Texture* metallic;
    float metallic_scale;
    
    Texture* roughness;
    float roughness_scale;
    
    Texture* emissive;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Material*> materials;
};

Model load_gltf(const char* filepath);

#endif // GLTF_HPP
