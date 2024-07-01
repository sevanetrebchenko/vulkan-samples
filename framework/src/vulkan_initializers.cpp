
#include "vulkan_initializers.hpp"
#include "helpers.hpp"
#include <shaderc/shaderc.hpp>
#include <filesystem> // std::filesystem
#include <fstream> // std::ifstream

VkShaderModule create_shader_module(VkDevice device, const char* filepath, std::initializer_list<std::pair<std::string, std::string>> preprocessor_definitions) {
    // Read shader into memory
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader: " + std::string(filepath));
    }

    std::streamsize file_size = file.tellg();
    std::string source;
    source.resize(file_size);

    // Read data
    file.seekg(0);
    file.read(source.data(), file_size);
    file.close();

    shaderc::CompileOptions options { };
    for (const auto&[directive, value] : preprocessor_definitions) {
        options.AddMacroDefinition(directive, value);
    }
    
    // TODO: support multiple shader languages
    shaderc_shader_kind type;
    std::filesystem::path path = std::filesystem::path(filepath);
    std::filesystem::path extension = path.extension();
    if (extension == ".vert") {
        type = shaderc_glsl_vertex_shader;
    }
    else if (extension == ".frag") {
        type = shaderc_glsl_fragment_shader;
    }
    else if (extension == ".geom") {
        type = shaderc_glsl_geometry_shader;
    }
    else if (extension == ".comp") {
        type = shaderc_glsl_compute_shader;
    }
    else {
        throw std::runtime_error("unknown shader type!");
    }
    
    shaderc::Compiler compiler { };
    std::string filename = path.stem().u8string(); // Convert from wchar_t
    
    // Function assumes entry point is 'main'
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, type, filename.c_str());
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("failed to compile " + std::string(filepath) + ": " + result.GetErrorMessage());
    }
    
    std::vector<unsigned> spirv = { result.cbegin(), result.cend() };
    VkShaderModuleCreateInfo shader_module_create_info { };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = spirv.size() * sizeof(unsigned); // Bytes
    shader_module_create_info.pCode = spirv.data();
    
    VkShaderModule shader_module { };
    if (vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    return shader_module;
}

VkPipelineShaderStageCreateInfo create_shader_stage(VkShaderModule module, VkShaderStageFlagBits stage, VkSpecializationInfo* specialization_info, const char* entry) {
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateInfo.html
    VkPipelineShaderStageCreateInfo create_info { };
    
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.stage = stage;
    create_info.module = module;
    create_info.pName = entry;
    create_info.pSpecializationInfo = specialization_info;
    
    return create_info;
}

VkVertexInputBindingDescription create_vertex_binding_description(unsigned binding, unsigned stride, VkVertexInputRate input_rate) {
    VkVertexInputBindingDescription description { };
    
    // Specify the binding point of the buffer
    description.binding = binding;
    
    // Specify the stride between consecutive elements in the buffer
    description.stride = stride;
    
    // Specify the rate at which vertex attributes are pulled from the buffer
    //   - VK_VERTEX_INPUT_RATE_VERTEX - attributes are pulled per vertex
    //   - VK_VERTEX_INPUT_RATE_INSTANCE - attributes are pulled per instance
    description.inputRate = input_rate;
    
    return description;
}

VkVertexInputAttributeDescription create_vertex_attribute_description(unsigned binding, unsigned location, VkFormat format, unsigned offset) {
    VkVertexInputAttributeDescription description { };
    
    // Specify the binding point of the buffer
    description.binding = binding;
    
    // Attribute location, references the 'layout (location = 0) in ...' portion of the vertex shader
    description.location = location;
    
    // Format of the attribute data type
    description.format = format;
    
    // Offset from the start of an element in the vertex input binding, for interleaved data types
    description.offset = offset;
    
    return description;
}

VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(VkPrimitiveTopology topology, bool enable_primitive_restart) {
    // Input assembly describes what kind of geometry is being drawn (topology) and if primitive restart is enabled (primitiveRestartEnable), which allows breaking up STRIP topology modes
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineInputAssemblyStateCreateInfo.html
    VkPipelineInputAssemblyStateCreateInfo create_info { };
    
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    
    // Specify whether input vertices are points, a list of lines / triangles, or strips
    // Topology options: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
    create_info.topology = topology;
    
    // Specifying VK_TRUE here breaking up lines + triangles with a special index (0xFFFF, 0xFFFFFFFF) when using STRIP topology methods
    create_info.primitiveRestartEnable = (VkBool32) enable_primitive_restart;
    
    return create_info;
}

VkViewport create_viewport(float x, float y, float width, float height, float min_depth, float max_depth) {
    VkViewport viewport { };

    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;
    
    return viewport;
}

VkRect2D create_region(int x, int y, unsigned width, unsigned height) {
    VkRect2D region { };
    region.offset = { x, y };
    region.extent = { width, height };
    return region;
}

// LOAD operations define the initial values of an attachment at the start of a render pass
//   For attachments with a depth/stencil format, LOAD operations execute during the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT pipeline stage
//   For attachments with a color format, LOAD operations execute during the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage
// LOAD operations that can be used for a render pass:
//   - VK_ATTACHMENT_LOAD_OP_LOAD      - previous attachment contents will be preserved as initial values
//                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_READ_BIT (color)
//   - VK_ATTACHMENT_LOAD_OP_CLEAR     - previous attachment contents will be cleared to a uniform value
//                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//   - VK_ATTACHMENT_LOAD_OP_DONT_CARE - previous attachment contents will not be preserved or cleared (undefined)
//                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//   - VK_ATTACHMENT_LOAD_OP_NONE_KHR  - previous attachment is not used
//                                     - memory access type: N/A

// https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-store-operations
// STORE operations define how the values written to an attachment during a render pass are stored in memory
//   For attachments with a depth/stencil format, STORE operations execute during the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT pipeline stage
//   For attachments with a color format, STORE operations execute during the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage

// STORE operations that can be used for a render pass:
//   - VK_ATTACHMENT_STORE_OP_STORE - attachment contents generated during the render pass are written to memory
//                                  - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//   - VK_ATTACHMENT_STORE_OP_DONT_CARE - attachment contents are not needed after the render pass, and may be discarded (undefined)
//                                      - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//   - VK_ATTACHMENT_STORE_OP_NONE - attachment contents are not referenced by the STORE operation as long as no values are written (*otherwise same as VK_ATTACHMENT_STORE_OP_DONT_CARE)
//                                 - memory access type: *N/A

VkAttachmentDescription create_attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp load, VkAttachmentStoreOp store, VkAttachmentLoadOp stencil_load, VkAttachmentStoreOp stencil_store, VkImageLayout initial, VkImageLayout final) {
    // A
    VkAttachmentDescription description { };

    description.format = format;
    
    // Number of samples this attachment uses (for multisampling)
    description.samples = samples;

    // Describes the attachment load + store operations, what should happen to the attachment at the beginning (load) and end (store) of the first (load) or last (store) subpass in which the attachment is used
    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-store-operations
    description.loadOp = load;
    description.storeOp = store;
    
    description.stencilLoadOp = stencil_load;
    description.stencilStoreOp = stencil_store;
    
    // Describes the initial layout of the image resource is in before the start of the render pass instance
    description.initialLayout = initial;
    
    // Describes the final layout the image resource will be transitioned to at the end of the render pass instance
    description.finalLayout = final;
    
    return description;
}

VkSubpassDependency create_subpass_dependency(unsigned src_subpass, VkPipelineStageFlags src_stages, VkAccessFlags src_memory_access, unsigned dst_subpass, VkPipelineStageFlags dst_stages, VkAccessFlags dst_memory_access) {
    VkSubpassDependency dependency { };
    
    // Describes the index of the subpass to depend on
    dependency.srcSubpass = src_subpass;
    
    // Describes the index of the current subpass (dstSubpass depends on srcSubpass)
    dependency.dstSubpass = dst_subpass;
    
    // Pipeline stages that need to finish executing in the dependent subpass before the current subpass can begin
    dependency.srcStageMask = src_stages;
    
    // Pipeline stages in the current subpass that cannot be executed until after all stages specified in the dependent subpass are completed
    dependency.dstStageMask = dst_stages;
    
    // Memory access types used by the dependent subpass
    dependency.srcAccessMask = src_memory_access;
    
    // Memory access types used by the current subpass
    dependency.dstAccessMask = dst_memory_access;
    
    return dependency;
}

VkAttachmentReference create_attachment_reference(unsigned attachment, VkImageLayout layout) {
    // Used to specify the layout of an attachment during a render pass
    VkAttachmentReference attachment_reference { };
    
    attachment_reference.attachment = attachment;
    attachment_reference.layout = layout;
    
    return attachment_reference;
}

VkPipelineColorBlendAttachmentState create_color_blend_attachment_state(VkColorComponentFlags color_mask, bool blending_enabled) {
    VkPipelineColorBlendAttachmentState blend_attachment_state { };
    blend_attachment_state.colorWriteMask = color_mask;
    blend_attachment_state.blendEnable = (VkBool32) blending_enabled;
    blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    return blend_attachment_state;
}
