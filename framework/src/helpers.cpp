
#include "helpers.hpp"

#define GLM_FORCE_RADIANS 1
#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"
#include <tiny_obj_loader.h>
#include <stdexcept> // std::runtime_error
#include <unordered_map> // std::unordered_map

unsigned get_memory_type_index(VkPhysicalDevice physical_device, VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags desired_memory_properties) {
    // Returns the index of the memory
    // TODO: physical device memory properties can be cached to prevent repeat calls to vkGetPhysicalDeviceMemoryProperties
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    unsigned selected_memory_type = physical_device_memory_properties.memoryTypeCount;
    
    for (unsigned i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if (memory_requirements.memoryTypeBits & (1 << i)) {
            // Memory type was requested, check to see if it is supported by the selected physical device
            if ((physical_device_memory_properties.memoryTypes[i].propertyFlags & desired_memory_properties) == desired_memory_properties) {
                // Memory supports all desired features
                selected_memory_type = i;
                break;
            }
        }
    }
    
    if (selected_memory_type == physical_device_memory_properties.memoryTypeCount) {
        throw std::runtime_error("failed to find suitable memory type for resource!");
    }
    
    return selected_memory_type;
}

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view) {
    VkImageViewCreateInfo image_view_ci { };
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = image;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.format = format;
    
    image_view_ci.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    
    image_view_ci.subresourceRange.aspectMask = aspect;
    
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = mip_levels;
    
    // Only one image layer
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &image_view_ci, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo image_ci { };
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.extent.width = image_width;
    image_ci.extent.height = image_height;
    image_ci.extent.depth = 1; // 2D image must have a depth of 1
    image_ci.mipLevels = mip_levels;
    image_ci.arrayLayers = 1; // No image array
    
    image_ci.format = format; // TODO: format must be supported by the graphics hardware (VkSurfaceFormatKHR) (check?)
    
    // Tiling mode specifies the tiling arrangement of texels in an image
    image_ci.tiling = tiling; // VK_IMAGE_TILING_LINEAR: texels are laid out in row-major order to support direct access of texels in the image memory
                                             // VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation-dependent order for optimal access from shaders
    
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#resources-image-layouts
    // The initial layout of an image specifies the layout the image is in before the start of the renderpass it is used in. Applications can transform image resources from one layout to another based on the specific usage of the image memory
    //   - VK_IMAGE_LAYOUT_PREINITIALIZED: image data ca be pre-initialized by the host, and transitioning to another layout will preserve that data
    //   - VK_IMAGE_LAYOUT_UNDEFINED: image data is undefined (don't care) and preservation of the data when transitioning to another layout is not guaranteed
    // Note: All image resources are required to be transitioned to another layout (than the ones above) before accessed by the GPU
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.usage = usage;
    
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.samples = samples;
    image_ci.flags = 0; // TODO: look into sparse 3D textures for something like voxel terrain (allows avoiding allocations for large chunks of 'air' voxels)
    
    if (vkCreateImage(device, &image_ci, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    
    // Query image memory requirements
    VkMemoryRequirements image_memory_requirements { };
    vkGetImageMemoryRequirements(device, image, &image_memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info { };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = image_memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = get_memory_type_index(physical_device, image_memory_requirements, desired_memory_properties);
    
    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for image!");
    }
}

void create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize allocation_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags desired_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo buffer_create_info { };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = allocation_size;
    buffer_create_info.usage = buffer_usage; // Indicates the purpose(s) for which the data in the buffer will be used
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer object!");
    }

    // Query buffer memory requirements
    VkMemoryRequirements memory_requirements { };
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    // Graphics cards have different types of memory (such as VRAM and RAM swap space) that varies in terms of permitted operations and performance
    // Buffer usage requirements must be considered when finding the right type of memory to use
    VkMemoryAllocateInfo memory_allocate_info { };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = get_memory_type_index(physical_device, memory_requirements, desired_properties);

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for buffer!");
    }
    
    // Associate the memory with the buffer
    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

Model load_model(const char* filepath) {
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

                if (has_texture_coordinates) {
                    model.vertices.emplace_back(vertex);
                }
                else {
                    model.vertices.emplace_back(vertex);
                }
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

Model::Vertex::Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 uv) : position(position),
                                                                            normal(normal) {
}

Model::Vertex::Vertex(glm::vec3 position, glm::vec2 uv) : position(position),
                                                          normal(glm::vec3(0.0f)) {
}

Model::Vertex::Vertex(glm::vec3 position) : position(position),
                                            normal(glm::vec3(0.0f)) {
}

VkDescriptorSetLayoutBinding create_descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stages, unsigned binding) {
    VkDescriptorSetLayoutBinding set_binding { };
    
    // Should match with the 'binding' indicator on the buffer
    // layout (binding = 0) uniform ... { ... };
    set_binding.binding = binding;
    set_binding.descriptorType = type;
    
    // Allows for an array of uniform buffer objects
    set_binding.descriptorCount = 1;
    
    // Describe what stages this buffer is accessed in (can be a bitwise-or of VkShaderStageFlagBits, or VK_SHADER_STAGE_ALL_GRAPHICS)
    set_binding.stageFlags = stages;
    
    set_binding.pImmutableSamplers = nullptr;
    return set_binding;
}

std::size_t align_to_device_boundary(VkPhysicalDevice physical_device, std::size_t size) {
    VkPhysicalDeviceProperties properties { };
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    
	size_t min_alignment = properties.limits.minUniformBufferOffsetAlignment;
	size_t aligned = size;
	if (min_alignment > 0) {
		aligned = (aligned + min_alignment - 1) & ~(min_alignment - 1);
	}
	return aligned;
    
}

void copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkDeviceSize src_offset, VkBuffer dst,  VkDeviceSize dst_offset, VkDeviceSize size) {
    // Copy 'size' bytes from the source to the destination buffer.
    VkBufferCopy copy_region { };
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);
}