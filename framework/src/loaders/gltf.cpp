
#include "loaders/gltf.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h> // Postprocessing loaded aiScene data
#include <assimp/pbrmaterial.h>

#include <vulkan/vulkan.h>

#include <iostream> // std::cout, std::endl
#include <queue> // std::queue

Model load_gltf(const char* filename) {
    Assimp::Importer loader { };
    
    const aiScene* scene = loader.ReadFile(filename, aiProcess_FlipUVs);
    if (!scene) {
        throw std::runtime_error("failed to load glTF model!");
    }
    
    Model model { };
    model.meshes.resize(scene->mNumMeshes);
    
    for (std::size_t m = 0u; m < scene->mNumMeshes; ++m) {
        const aiMesh& assimp_mesh = *scene->mMeshes[m];
        Mesh& mesh = model.meshes[m];

        for (std::size_t f = 0; f < assimp_mesh.mNumFaces; ++f) {
            const aiFace& face = assimp_mesh.mFaces[f];
            
            // Load geometry data
            for (std::size_t i = 0u; i < face.mNumIndices; ++i) {
                std::size_t index = face.mIndices[i];
                
                Vertex& vertex = mesh.vertices.emplace_back();
                
                if (assimp_mesh.HasPositions()) {
                    vertex.position = glm::vec3(assimp_mesh.mVertices[index].x, assimp_mesh.mVertices[index].y, assimp_mesh.mVertices[index].z);
                }
                
                if (assimp_mesh.HasNormals()) {
                    vertex.normal = glm::vec3(assimp_mesh.mNormals[index].x, assimp_mesh.mNormals[index].y, assimp_mesh.mNormals[index].z);
                }
                
                if (assimp_mesh.HasTangentsAndBitangents()) {
                    vertex.tangent = glm::vec3(assimp_mesh.mTangents[index].x, assimp_mesh.mTangents[index].y, assimp_mesh.mTangents[index].z);
                }
                
                // Assimp supports loading bitangents
                // glm::vec3(mesh->mBitangents[index].x, mesh->mBitangents[index].y, mesh->mBitangents[index].z);
                
                if (assimp_mesh.HasTextureCoords(0)) {
                    vertex.uv = glm::vec2(assimp_mesh.mTextureCoords[0][index].x, assimp_mesh.mTextureCoords[0][index].y);
                }

                // TODO: indexed rendering (+ effects on normal / tangent vectors)
                mesh.indices.emplace_back(mesh.indices.size());
            }
            
//            // Load material data
//            if (assimp_mesh.mMaterialIndex >= 0) {
//                const aiMaterial& assimp_material = *scene->mMaterials[assimp_mesh.mMaterialIndex];
//
//                static const auto load = [](const aiMaterial& assimp_material, aiTextureType type) -> Texture* {
//                    if (assimp_material.GetTextureCount(type) > 0) {
//                        aiString texture_path { };
//                        assimp_material.GetTexture(type, 0, &texture_path);
//                        return load_texture(texture_path.C_Str());
//                    }
//                    return nullptr;
//                };
//
//                PBRMaterial* material = new PBRMaterial();
//
//                material->albedo = load(assimp_material, aiTextureType_DIFFUSE);
//                material->normals = load(assimp_material, aiTextureType_NORMALS);
//
//                aiString metallic_roughness_texture_path { };
//                if (assimp_material.GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &metallic_roughness_texture_path) == aiReturn_SUCCESS) {
//                    // Combined metallic / roughness texture (metallic stored in g channel, roughness stored in b channel)
//                    // Combined texture is stored in material->metallic
//                    // TODO: is it worth splitting the texture into two and assign to the proper member variable?
//                    material->metallic = load_texture(metallic_roughness_texture_path.C_Str());
//                }
//                else {
//                    // Load metallic and roughness textures individually
//                    material->metallic = load(assimp_material, aiTextureType_METALNESS);
//                    material->roughness = load(assimp_material, aiTextureType_DIFFUSE_ROUGHNESS);
//                }
//
//                material->ambient_occlusion = load(assimp_material, aiTextureType_AMBIENT_OCCLUSION);
//                material->emissive = load(assimp_material, aiTextureType_EMISSION_COLOR);
//
//                assimp_material.Get(AI_MATKEY_METALLIC_FACTOR, material->metallic_scale);
//                assimp_material.Get(AI_MATKEY_ROUGHNESS_FACTOR, material->roughness_scale);
//
//                model.meshes.emplace_back(material);
//            }
        }
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

    // Normalize UV coordinates

    
//    for (Mesh& mesh : model.meshes) {
//        for (int i = 0; i < 2; ++i) {
//            // 0 = x, 1 = y
//            float min = std::numeric_limits<float>::max();
//            float max = std::numeric_limits<float>::lowest();
//
//            for (Vertex& vertex : mesh.vertices) {
//                if (vertex.uv[0] > max) {
//                    max = vertex.uv[0];
//                }
//
//                if (vertex.uv[0] < min) {
//                    min = vertex.uv[0];
//                }
//            }
//
//            float scale = (max - min);
//            float offset = min;
//
//            for (Vertex& vertex : mesh.vertices) {
//                vertex.uv[0] = (vertex.uv[0] - offset) / scale;
//            }
//        }
//    }
    
    return model;
}