
#ifndef VULKAN_INITIALIZERS_HPP
#define VULKAN_INITIALIZERS_HPP

#include <vulkan/vulkan.h>
#include <initializer_list> // std::initializer_list
#include <utility> // std::pair
#include <string> // std::string

// Stage gets determined from the shader extension
//   .vert - VK_SHADER_STAGE_VERTEX_BIT
//   .frag - VK_SHADER_STAGE_FRAGMENT_BIT
//   .geom - VK_SHADER_STAGE_GEOMETRY_BIT
VkShaderModule create_shader_module(VkDevice device, const char* filepath, std::initializer_list<std::pair<std::string, std::string>> preprocessor_definitions = { });
VkPipelineShaderStageCreateInfo create_shader_stage(VkShaderModule module, VkShaderStageFlagBits stage, VkSpecializationInfo* specialization_info = nullptr, const char* entry = "main");

VkVertexInputBindingDescription create_vertex_binding_description(unsigned binding, unsigned stride, VkVertexInputRate input_rate);
VkVertexInputAttributeDescription create_vertex_attribute_description(unsigned binding, unsigned location, VkFormat format, unsigned offset);

// Describes the topology of the geometry being rendered
VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(VkPrimitiveTopology topology, bool enable_primitive_restart = false);

VkViewport create_viewport(float x, float y, float width, float height, float min_depth, float max_depth);

VkRect2D create_region(int x, int y, unsigned width, unsigned height);

VkAttachmentDescription create_attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp load, VkAttachmentStoreOp store, VkAttachmentLoadOp stencil_load, VkAttachmentStoreOp stencil_store, VkImageLayout initial, VkImageLayout final);
VkAttachmentReference create_attachment_reference(unsigned attachment, VkImageLayout layout);

VkSubpassDependency create_subpass_dependency(unsigned src_subpass, VkPipelineStageFlags src_stages, VkAccessFlags src_memory_access, unsigned dst_subpass, VkPipelineStageFlags dst_stages, VkAccessFlags dst_memory_access);

VkPipelineColorBlendAttachmentState create_color_blend_attachment_state(VkColorComponentFlags color_mask, bool blending_enabled = false);

#endif // VULKAN_INITIALIZERS_HPP
