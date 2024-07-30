
#include "loaders/gltf.hpp"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <vulkan/vulkan.h>

#include <iostream> // std::cout, std::endl
#include <queue> // std::queue

Model load_gltf(const char* filename) {
    tinygltf::Model glftmodel;
    tinygltf::TinyGLTF loader;
    std::string error;
    std::string warning;
    
    bool result = loader.LoadASCIIFromFile(&glftmodel, &error, &warning, filename);
    
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
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned> indices;
    
    // Bounding box dimensions
    glm::vec3 min;
    glm::vec3 max;
    
    tinygltf::Scene& scene = glftmodel.scenes[glftmodel.defaultScene > -1 ? glftmodel.defaultScene : 0];
    std::queue<int> nodes;
    
    // Push top-level scene nodes
    for (std::size_t i = 0u; i < scene.nodes.size(); ++i) {
        nodes.push(scene.nodes[i]);
    }
    
    while (!nodes.empty()) {
        const tinygltf::Node& node = glftmodel.nodes[nodes.front()];
        
        if (node.mesh > -1) {
            const tinygltf::Mesh& mesh = glftmodel.meshes[node.mesh];
            for (const tinygltf::Primitive& primitive : mesh.primitives) {
                float* data;
                std::size_t stride;
                int byte_stride;
                
                // Accessors and their types (number of components) are defined in this table:
                // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes
                
                // Positions
                auto attribute = primitive.attributes.find("POSITION");
                assert(attribute != primitive.attributes.end()); // Positions are expected to present
                tinygltf::Accessor& accessor = glftmodel.accessors[attribute->second];
                tinygltf::BufferView& view = glftmodel.bufferViews[accessor.bufferView];
                tinygltf::Buffer& buffer = glftmodel.buffers[view.buffer];
                
                min = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
                max = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
                
                data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                
                byte_stride = accessor.ByteStride(view);
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
                
                // Normals
                attribute = primitive.attributes.find("NORMAL");
                if (attribute != primitive.attributes.end()) {
                    // Normals are not guaranteed to be present, but can be calculated from the vertex positions
                    accessor = glftmodel.accessors[attribute->second];
                    view = glftmodel.bufferViews[accessor.bufferView];
                    buffer = glftmodel.buffers[view.buffer];
                    data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    byte_stride = accessor.ByteStride(view);
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
                else {
                    // TODO: manual calculation of vertex normals
                }
                
                // UV coordinates (only TEXCOORD_0 for now)
                attribute = primitive.attributes.find("TEXCOORD_0");
                if (attribute != primitive.attributes.end()) {
                    // Texture coordinates are not guaranteed to be present
                    accessor = glftmodel.accessors[attribute->second];
                    view = glftmodel.bufferViews[accessor.bufferView];
                    buffer = glftmodel.buffers[view.buffer];
                    data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    byte_stride = accessor.ByteStride(view);
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
                
                // Tangent vector (part of TBN matrix)
                attribute = primitive.attributes.find("TANGENT");
                if (attribute != primitive.attributes.end()) {
                    accessor = glftmodel.accessors[attribute->second];
                    view = glftmodel.bufferViews[accessor.bufferView];
                    buffer = glftmodel.buffers[view.buffer];
                    data = (float*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                    
                    byte_stride = accessor.ByteStride(view);
                    if (!byte_stride) {
                        // Returns -1 on invalid gLTF value
                         stride = tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4); // Tangents are stored as a normalized vec4 with w being a signed value representing handedness
                    }
                    else {
                        stride = byte_stride / sizeof(float);
                    }
                    
                    for (std::size_t i = 0u; i < accessor.count; ++i) {
                        glm::vec4 tangent = glm::vec4(data[i * stride + 0], data[i * stride + 1], data[i * stride + 2], data[i * stride + 3]);
                        tangents.emplace_back(tangent);
                    }
                }
                
                // Indices
                accessor = glftmodel.accessors[primitive.indices > -1 ? primitive.indices : 0];
                view = glftmodel.bufferViews[accessor.bufferView];
                buffer = glftmodel.buffers[view.buffer];
                void* index_data = (void*)(&buffer.data[accessor.byteOffset + view.byteOffset]);
                
                switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                        for (std::size_t i = 0u; i < accessor.count; ++i) {
                            indices.emplace_back(((unsigned int*)(index_data))[i]);
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        for (std::size_t i = 0u; i < accessor.count; ++i) {
                            indices.emplace_back(((unsigned short*)(index_data))[i]);
                        }
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        for (std::size_t i = 0u; i < accessor.count; ++i) {
                            indices.emplace_back(((unsigned char*)(index_data))[i]);
                        }
                        break;
                    }
                }
            }
        }
        nodes.pop();
    }
    
    
    
    // Center model at (0, 0, 0)
    glm::vec3 center = glm::vec3((min + max) / 2.0f);
    for (glm::vec3& position : positions) {
        position -= center;
    }

    // Scale the mesh to range [-1 1] on all axes
    glm::vec3 extents = max - min;
    float max_extent = std::max({ extents.x, extents.y, extents.z });
    float scale = 2.0f / max_extent;
    for (glm::vec3& position : positions) {
        position *= scale;
    }
    
    assert(indices.size() % 3 == 0);
    

    
    std::size_t size = positions.size();
    bool has_normals = !normals.empty();
    bool has_uvs = !uvs.empty();
    bool has_tangents = !tangents.empty();
    
    if ((has_normals && size != normals.size()) || (has_uvs && size != uvs.size()) || (has_tangents && size != tangents.size())) {
        // Assume gLTF models are indexed
        throw std::runtime_error("error mapping model");
    }
    
    if (!has_normals) {
        // TODO: compute vertex normals
    }
    
    if (has_uvs && !has_tangents) {
        for (std::size_t i = 0u; i < indices.size() / 3; i += 3) {
            const glm::vec3& p0 = positions[indices[i + 0]];
            const glm::vec3& p1 = positions[indices[i + 1]];
            const glm::vec3& p2 = positions[indices[i + 2]];
            
            const glm::vec2& uv0 = uvs[indices[i + 0]];
            const glm::vec2& uv1 = uvs[indices[i + 1]];
            const glm::vec2& uv2 = uvs[indices[i + 2]];
            
            glm::vec3 e1 = p1 - p0;
            glm::vec3 e2 = p2 - p0;
            
            glm::vec2 duv1 = uv1 - uv0;
            glm::vec2 duv2 = uv2 - uv0;
            
            float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);
        
            glm::vec3 tangent;
            tangent.x = f * (duv2.y * e1.x - duv1.y * e2.x);
            tangent.y = f * (duv2.y * e1.y - duv1.y * e2.y);
            tangent.z = f * (duv2.y * e1.z - duv1.y * e2.z);
            
            // TODO: average all tangents per vertex
            tangents.emplace_back(tangent, 1.0f);
            tangents.emplace_back(tangent, 1.0f);
            tangents.emplace_back(tangent, 1.0f);
        }
        
        has_tangents = true;
    }
    
    Model model { };
    model.indices = std::move(indices);
    
    for (std::size_t i = 0u; i < positions.size(); ++i) {
        Model::Vertex& vertex = model.vertices.emplace_back();
        vertex.position = positions[i];
        
        if (has_normals) {
            vertex.normal = normals[i];
        }
        
        if (has_tangents) {
            vertex.tangent = tangents[i];
        }
        
        if (has_uvs) {
            vertex.uv = uvs[i];
        }
    }
    
    return model;
}