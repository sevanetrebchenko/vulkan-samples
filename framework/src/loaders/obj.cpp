
#include "loaders/obj.hpp"
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>

#include <unordered_map> // std::unordered_map
#include <stdexcept> // std::runtime_error

Model load_obj(const char* filepath) {
    std::unordered_map<glm::vec3, std::size_t> unique_vertex_list { };
    
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    
    tinyobj::attrib_t attributes { }; // Holds all positions, normals, and texture coordinates.
    std::vector<tinyobj::shape_t> shape_data { }; // Holds all separate objects and their faces.
    std::vector<tinyobj::material_t> material_data { };
    std::string warning;
    std::string error;
    
    if (!tinyobj::LoadObj(&attributes, &shape_data, &material_data, &warning, &error, filepath)) {
        throw std::runtime_error("failed to load .obj model file: " + error);
    }
    
    Model model { };
    
    bool has_vertex_normals = !attributes.normals.empty();
    bool has_texture_coordinates = !attributes.texcoords.empty();
    bool has_vertex_colors = !attributes.colors.empty();
    
    for (const tinyobj::shape_t& shape : shape_data) {
        for (const tinyobj::index_t& index : shape.mesh.indices) {
            int base = 3 * index.vertex_index;
            glm::vec3 vertex(attributes.vertices[base + 0], attributes.vertices[base + 1], attributes.vertices[base + 2]);
            
            int uv_base = 2 * index.texcoord_index;
            glm::vec2 uv { };
            if (has_texture_coordinates) {
                uv = glm::vec2(attributes.texcoords[uv_base + 0], 1.0f - attributes.texcoords[uv_base + 1]);
            }

            if (unique_vertex_list.count(vertex) == 0) {
                unique_vertex_list[vertex] = unique_vertex_list.size();

                // Find mesh extrema.
                if (vertex.x < minimum.x) {
                    minimum.x = vertex.x;
                }
                else if (vertex.x > maximum.x) {
                    maximum.x = vertex.x;
                }

                if (vertex.y < minimum.y) {
                    minimum.y = vertex.y;
                }
                else if (vertex.y > maximum.y) {
                    maximum.y = vertex.y;
                }

                if (vertex.z < minimum.z) {
                    minimum.z = vertex.z;
                }
                else if (vertex.z > maximum.z) {
                    maximum.z = vertex.z;
                }

                model.vertices.emplace_back().position = vertex;
            }

            // In case of duplicate vertex, push back index instead
            model.indices.emplace_back(unique_vertex_list[vertex]);
        }
    }
    
    // TODO: Compute vertex normals
//    std::vector<std::vector<glm::vec3>> adjacent_face_normals(vertex_positions.size());
//
//    for (u32 i = 0u; i < indices.size(); ++i) {
//        glm::vec3 v1 = vertex_positions[indices[i + 0]];
//        glm::vec3 v2 = vertex_positions[indices[i + 1]];
//        glm::vec3 v3 = vertex_positions[indices[i + 2]];
//
//        glm::vec3 face_normal = glm::cross(v3 - v1, v2 - v1);
//
//        bool duplicate = false;
//
//        for (u32 j = 0u; j < 3; ++j) {
//            for (glm::vec3 normal : adjacent_face_normals[indices[i + j]]) {
//                if (glm::dot(face_normal, normal) - glm::dot(face_normal, face_normal) > std::numeric_limits<float>::epsilon()) {
//                    duplicate = true;
//                    break;
//                }
//            }
//
//            if (!duplicate) {
//                adjacent_face_normals[indices[i + j]].emplace_back(face_normal);
//            }
//        }
//    }

// Face normals
//    for (u32 i = 0u; i < vertex_positions.size() / 3; ++i) {
//        glm::vec3 v1 = vertex_positions[i + 0];
//        glm::vec3 v2 = vertex_positions[i + 1];
//        glm::vec3 v3 = vertex_positions[i + 2];
//
//        glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
//        vertex_normals.emplace_back(normal);
//        vertex_normals.emplace_back(normal);
//        vertex_normals.emplace_back(normal);
//    }

//    for (u32 i = 0u; i < vertex_positions.size(); ++i) {
//        vertex_normals.emplace_back(1.0f);
//        glm::vec3 normal { };
//
//        for (glm::vec3 current : adjacent_face_normals[i]) {
//            normal += current;
//        }
//
//        vertex_normals.emplace_back(glm::normalize(normal));
//    }
    
    // Center model at (0, 0, 0)
    glm::vec3 center = glm::vec3((minimum + maximum) / 2.0f);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -center);

    for (std::size_t i = 0u; i < model.vertices.size(); ++i) {
        model.vertices[i].position = glm::vec3(translation * glm::vec4(model.vertices[i].position, 1.0f));
    }

    // Scale model to [1, 1, 1]
    glm::vec3 bounding_box = maximum - minimum;

    // Scale the mesh to range [-1 1] on all axes.
    float max_dimension = std::max(bounding_box.x, std::max(bounding_box.y, bounding_box.z));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / max_dimension));

    for (std::size_t i = 0u; i < model.vertices.size(); ++i) {
        model.vertices[i].position = glm::vec3(scale * glm::vec4(model.vertices[i].position, 1.0f));
    }
    
    // Recalculate vertex normals
    assert(model.indices.size() % 3 == 0);
    std::vector<std::vector<glm::vec3>> adjacent_face_normals(model.vertices.size());

    // Traverse all mesh faces and attempt to add the face normals of all adjacent faces, taking care to make sure normals are not accounted for more than once
    // Attempt to add each normal to the involved vertex if it hasn't been already.
    for (int i = 0; i < model.indices.size(); i += 3) {
        unsigned index1 = model.indices[i + 0];
        unsigned index2 = model.indices[i + 1];
        unsigned index3 = model.indices[i + 2];
        const glm::vec3& vertex1 = model.vertices[index1].position;
        const glm::vec3& vertex2 = model.vertices[index2].position;
        const glm::vec3& vertex3 = model.vertices[index3].position;

        // Calculate face normal.
        glm::vec3 face_normal = glm::cross(vertex3 - vertex2, vertex1 - vertex2);

        bool duplicate = false;
        // Attempt to add each normal to the involved vertices.
        for (unsigned j = 0; j < 3; ++j) {
            const unsigned& index = model.indices[i + j];
            // Check if normal was already added to this face's vertices.
            for (const auto &normal : adjacent_face_normals[index]) {
                if ((glm::dot(face_normal, normal) - glm::dot(face_normal, face_normal)) > std::numeric_limits<float>::epsilon()) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate) {
                adjacent_face_normals[index].emplace_back(face_normal);
            }
        }
    }

    // Compute normals from precomputed adjacent normal list
    for (std::size_t i = 0u; i < adjacent_face_normals.size(); ++i) {
        glm::vec3 normal = glm::vec3(0.0f);

        // Sum all adjacent face normals (this needs to happen without duplicates)
        for (const glm::vec3& n : adjacent_face_normals[i]) {
            normal += n;
        }
        
        model.vertices[i].normal = glm::normalize(normal);
    }
    
    return std::move(model);
}
