
#include "loaders/gltf.hpp"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <vulkan/vulkan.h>

#include <mikktspace.h>

#include <iostream> // std::cout, std::endl
#include <queue> // std::queue

// Needs to provide definitions for MikkTSpace interface defined in mikktspace.h
// Reference: https://www.turais.de/using-mikktspace-in-your-project/

int get_vertex_index(const SMikkTSpaceContext* context, int face_index, int vertex_index);
int get_num_faces(const SMikkTSpaceContext* context);
int get_num_face_vertices(const SMikkTSpaceContext* context, int face_index);
void get_position(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index);
void get_normal(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index);
void get_uv(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index);
void set_tangent(const SMikkTSpaceContext *context, const float* tangent, float sign, int face_index, int vertex_index);

int get_vertex_index(const SMikkTSpaceContext* context, int face_index, int vertex_index) {
    Model* model = static_cast<Model*>(context->m_pUserData);
    return model->indices[face_index * get_num_face_vertices(context, face_index) + vertex_index];
}

// Returns the total number of faces on the mesh to be processed
int get_num_faces(const SMikkTSpaceContext* context) {
    Model* model = static_cast<Model*>(context->m_pUserData);
    return model->indices.size() / 3;
}

// Returns the number of vertices, for the given face
// face_index is guaranteed to be in the range [0, get_num_faces(...) - 1]
int get_num_face_vertices(const SMikkTSpaceContext* context, int face_index) {
    // Hardcoded for triangle meshes (for now)
    return 3;
}

// Returns the vertex position / normal / UV coordinate for the given vertex, for the given face
// face_index is guaranteed to be in the range [0, get_num_faces(...) - 1]
// vertex_index is in the range [0, 2] for triangles and [0, 3] for quads
void get_position(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index) {
    Model* model = static_cast<Model*>(context->m_pUserData);
    const glm::vec3& position = model->vertices[get_vertex_index(context, face_index, vertex_index)].position;
    out[0] = position.x;
    out[1] = position.y;
    out[2] = position.z;
}

void get_normal(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index) {
    Model* model = static_cast<Model*>(context->m_pUserData);
    const glm::vec3& normal = model->vertices[get_vertex_index(context, face_index, vertex_index)].normal;
    out[0] = normal.x;
    out[1] = normal.y;
    out[2] = normal.z;
}

void get_uv(const SMikkTSpaceContext *context, float* out, int face_index, int vertex_index) {
    Model* model = static_cast<Model*>(context->m_pUserData);
    const glm::vec2& uv = model->vertices[get_vertex_index(context, face_index, vertex_index)].uv;
    out[0] = uv.s;
    out[1] = uv.t;
}

// Returns the tangent and orientation (sign)
// Note: results are not indexed // TODO: is this a problem?
void set_tangent(const SMikkTSpaceContext *context, const float* in, float sign, int face_index, int vertex_index) {
    // Assumes tangent array is preallocated
    Model* model = static_cast<Model*>(context->m_pUserData);
    glm::vec4& tangent = model->vertices[get_vertex_index(context, face_index, vertex_index)].tangent;
    tangent = glm::vec4(glm::normalize(glm::vec3(in[0], in[1], in[2])), sign);
}

// Loads model tangents using MikkTSpace
void load_tangents(Model& model) {
    SMikkTSpaceInterface interface { };
    interface.m_getNumFaces = get_num_faces;
    interface.m_getNumVerticesOfFace = get_num_face_vertices;
    interface.m_getPosition = get_position;
    interface.m_getNormal = get_normal;
    interface.m_getTexCoord = get_uv;
    interface.m_setTSpaceBasic = set_tangent;
    
    SMikkTSpaceContext context { };
    context.m_pInterface = &interface;
    context.m_pUserData = &model;
    
    genTangSpaceDefault(&context);
}

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
    
    std::size_t num_vertices = positions.size();
    bool has_normals = !normals.empty();
    bool has_uvs = !uvs.empty();
    bool has_tangents = !tangents.empty();
    
    if ((has_normals && num_vertices != normals.size()) || (has_uvs && num_vertices != uvs.size()) || (has_tangents && num_vertices != tangents.size())) {
        // Assume gLTF models are indexed
        throw std::runtime_error("error mapping model");
    }
    
    if (!has_normals) {
        // TODO: compute vertex normals
    }
    
    Model model { };
    model.indices = std::move(indices);
    
    for (std::size_t i = 0u; i < positions.size(); ++i) {
        Model::Vertex& vertex = model.vertices.emplace_back();
        vertex.position = positions[i];
        
        if (has_normals) {
            vertex.normal = normals[i];
        }
        
        if (has_uvs) {
            vertex.uv = uvs[i];
        }
    }
    
    if (has_uvs && !has_tangents) {
        load_tangents(model);
        has_tangents = true;
        
//        std::vector<glm::vec3> temp(num_vertices, glm::vec3(0.0f));
//
//        for (std::size_t i = 0u; i < indices.size() / 3; i += 3) {
//            // Derivation: https://web.archive.org/web/20110708081637/http://www.terathon.com/code/tangent.html
//
//            const glm::vec3& p0 = positions[indices[i + 0]];
//            const glm::vec3& p1 = positions[indices[i + 1]];
//            const glm::vec3& p2 = positions[indices[i + 2]];
//
//            const glm::vec2& uv0 = uvs[indices[i + 0]];
//            const glm::vec2& uv1 = uvs[indices[i + 1]];
//            const glm::vec2& uv2 = uvs[indices[i + 2]];
//
//            glm::vec3 q1 = p1 - p0;
//            glm::vec3 q2 = p2 - p0;
//
//            glm::vec2 st1 = uv1 - uv0;
//            glm::vec2 st2 = uv2 - uv0;
//
//            float f = 1.0f / (st1.s * st2.t - st2.s * st1.t);
//
//            glm::vec3 tangent;
//            tangent.x = f * (st2.t * q1.x - st1.t * q2.x);
//            tangent.y = f * (st2.t * q1.y - st1.t * q2.y);
//            tangent.z = f * (st2.t * q1.z - st1.t * q2.z);
//
//            temp[indices[i + 0]] += tangent;
//            temp[indices[i + 1]] += tangent;
//            temp[indices[i + 2]] += tangent;
//        }
//
//        tangents.resize(num_vertices);
//
//        // Normalize tangents, similar to how normalizing normals works
//        for (std::size_t i = 0u; i < num_vertices; ++i) {
//            tangents[i] = glm::vec4(glm::normalize(temp[i]), 1.0f);
//        }
//
//        has_tangents = true;
    }
    
    return model;
}