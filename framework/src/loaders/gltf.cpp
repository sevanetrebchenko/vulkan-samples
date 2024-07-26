
#include "loaders/gltf.hpp"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <vulkan/vulkan.h>

#include <iostream> // std::cout, std::endl
#include <queue> // std::queue

Model load_gltf(const char* filename) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string error;
    std::string warning;
    
    bool result = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
    
    if (!error.empty()) {
        std::cout << "error while loading gltf model: " << error << std::endl;
    }
    
    if (!warning.empty()) {
        std::cout << "warning while loading gltf model: " << warning << std::endl;
    }

    if (!result) {
        throw std::runtime_error("failed to load gltf!");
    }
    
    // TODO: extensions
    
//    // Load texture samplers
//    struct SamplerData {
//        int min_filter;
//        int mag_filter;
//
//        // No w sampling
//        int address_mode_u;
//        int address_mode_v;
//    };
//
//    std::vector<SamplerData> samplers;
//    for (const tinygltf::Sampler& sampler : model.samplers) {
//        SamplerData& data = samplers.emplace_back();
//        data.min_filter = sampler.minFilter;
//        data.mag_filter = sampler.magFilter;
//        data.address_mode_u = sampler.wrapS;
//        data.address_mode_v = sampler.wrapT;
//    }
    
    // Load vertex / index data
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned> indices;
    
    // Bounding box dimensions
    glm::vec3 min;
    glm::vec3 max;
    
    tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    std::queue<int> nodes;
    
    // Push top-level scene nodes
    for (std::size_t i = 0u; i < scene.nodes.size(); ++i) {
        nodes.push(scene.nodes[i]);
    }
    
    while (!nodes.empty()) {
        const tinygltf::Node& node = model.nodes[nodes.front()];
        
        if (node.mesh > -1) {
            const tinygltf::Mesh& mesh = model.meshes[node.mesh];
            for (const tinygltf::Primitive& primitive : mesh.primitives) {
                // Positions
                {
                    auto attribute = primitive.attributes.find("POSITION");
                    const tinygltf::Accessor& accessor = model.accessors[attribute->second];
                    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[view.buffer];
                    
                    min = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
                    max = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
                    
                    float* data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    std::size_t stride;
                    int byte_stride = accessor.ByteStride(view);
                    if (!byte_stride) {
                        // Returns -1 on invalid gLTF value
                         stride = tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                    }
                    else {
                        stride = byte_stride / sizeof(float);
                    }
                    
                    for (std::size_t i = 0u; i < accessor.count; ++i) {
                        positions.emplace_back(data[i * stride + 0], data[i * stride + 1], data[i * stride + 2]);
                    }
                }
                // Normals
                {
                    auto attribute = primitive.attributes.find("NORMAL");
                    const tinygltf::Accessor& accessor = model.accessors[attribute->second];
                    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[view.buffer];
                    float* data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    std::size_t stride;
                    int byte_stride = accessor.ByteStride(view);
                    if (!byte_stride) {
                        // Returns -1 on invalid gLTF value
                         stride = tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                    }
                    else {
                        stride = byte_stride / sizeof(float);
                    }
                    
                    for (std::size_t i = 0u; i < accessor.count; ++i) {
                        glm::vec3 normal = glm::vec3(data[i * stride + 0], data[i * stride + 1], data[i * stride + 2]);
                        normals.emplace_back(glm::normalize(normal));
                    }
                }
                
                // UV coordinates (only TEXCOORD_0 for now)
                {
                    auto attribute = primitive.attributes.find("TEXCOORD_0");
                    const tinygltf::Accessor& accessor = model.accessors[attribute->second];
                    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[view.buffer];
                    float* data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    std::size_t stride;
                    int byte_stride = accessor.ByteStride(view);
                    if (!byte_stride) {
                        // Returns -1 on invalid gLTF value
                         stride = tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
                    }
                    else {
                        stride = byte_stride / sizeof(float);
                    }
                    
                    for (std::size_t i = 0u; i < accessor.count; ++i) {
                        uvs.emplace_back(data[i * stride + 0], data[i * stride + 1]);
                    }
                }
                // Indices
                {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer &buffer = model.buffers[view.buffer];
                    void* data = (void*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    switch (accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                            for (std::size_t i = 0u; i < accessor.count; ++i) {
                                indices.emplace_back(((unsigned int*)(data))[i]);
                            }
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                            for (std::size_t i = 0u; i < accessor.count; ++i) {
                                indices.emplace_back(((unsigned short*)(data))[i]);
                            }
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                            for (std::size_t i = 0u; i < accessor.count; ++i) {
                                indices.emplace_back(((unsigned char*)(data))[i]);
                            }
                            break;
                        }
                    }
                }
            }
        }
        nodes.pop();
    }
    
//    // Center model at (0, 0, 0)
//    glm::vec3 center = glm::vec3((min + max) / 2.0f);
//    for (glm::vec3& position : positions) {
//        position -= center;
//    }
//
//    // Scale the mesh to range [-1 1] on all axes
//    glm::vec3 extents = max - min;
//    float max_extent = std::max({ extents.x, extents.y, extents.z });
//    float scale = 2.0f / max_extent;
//    for (glm::vec3& position : positions) {
//        position *= scale;
//    }
    
    Model m { };
    m.indices = std::move(indices);
    
    if (positions.size() != normals.size() || positions.size() != uvs.size()) {
        // Temporary hack solution to get gltf model loading working
        throw std::runtime_error("error mapping model");
    }
    
    for (std::size_t i = 0u; i < positions.size(); ++i) {
        Model::Vertex& vertex = m.vertices.emplace_back();
        vertex.position = positions[i];
        vertex.normal = normals[i];
        vertex.uv = uvs[i];
    }
    
    return m;
}