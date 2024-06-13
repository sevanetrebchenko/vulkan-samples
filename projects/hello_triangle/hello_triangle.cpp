
#include "sample.hpp"
#include <iostream> // std::cout, std::endl

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;
};

class HelloTriangle final : public Sample {
    public:
        HelloTriangle() : Sample("Hello Triangle"),
                          pipeline_layout(nullptr),
                          pipeline(nullptr) {
            settings.use_depth_buffer = false;
        }
        
        ~HelloTriangle() override {
        }
        
    private:
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
        
        void record_command_buffers() override {
        }
        
        void initialize_render_passes() override {
            // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-objects
            // A render pass object represents a collection of attachments, subpasses, and dependencies between the subpasses
            // It describes how the attachments are used over the course of a set of subpasses
            
            VkRenderPassCreateInfo render_pass_ci { };
            render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            
            // An attachment description describes the properties of a render pass attachment
            // This render pass uses one color attachment
            VkAttachmentDescription color_attachment_description { };
        
            // format - specifies the format of the image that will be used for the attachment
            // Since this will be presented to the swapchain, use the format of the surface
            color_attachment_description.format = surface_format.format;
            
            // samples - number of samples to use, for multisampling support
            // This example does not use multisampling
            color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        
            // loadOp - what to do with the attachment data before the render pass:
            //   - VK_ATTACHMENT_LOAD_OP_LOAD - preserve the existing contents of the attachment
            //   - VK_ATTACHMENT_LOAD_OP_CLEAR - clear the values to a constant at the start
            //   - VK_ATTACHMENT_LOAD_OP_DONT_CARE - existing contents are undefined
            // Clear the attachment to a uniform value (defined in VkRenderPassBeginInfo)
            color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        
            // storeOp - what to do with the attachment data after the render pass:
            //   - VK_ATTACHMENT_STORE_OP_STORE - rendered contents will be stored in memory and can be read later
            //   - VK_ATTACHMENT_STORE_OP_DONT_CARE - contents of the framebuffer will be undefined after the rendering operation
            // Store the color for presentation to the swapchain
            color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
            // stencilLoadOp - what to do with the stencil buffer data before the render pass (same values as loadOp)
            // stencilStoreOp - what to do with the stencil buffer data after the render pass (same values as storeOp)
            // This example does not use a stencil buffer
            color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
            // Render pass images need to be transitioned to specific layouts that are suitable for the next operation
        
            // The color attachment was previously used for presenting before being returned to the swapchain
            // It doesn't matter what the contents of the attachment are
            color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        
            // The color attachment should be transitioned to a format that is suitable to be presented to the swapchain at the end of the render pass
            color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            
            render_pass_ci.attachmentCount = 1;
            render_pass_ci.pAttachments = &color_attachment_description;
            
            // A subpass defines a phase of a render pass that reads from and writes to a subset of attachments
            
            // When a render pass is created, attachments are provided an initial layout and final layout, defined as initialLayout and finalLayout (respectively) on the attachment description above
            // Using the subpass description during subpass creation, color attachments are given a layout representing how the subpass uses the attachment
            // Vulkan performs implicit layout transitions for attachments at the following points:
            //   1. At the start of the render pass (vkCmdBeginRenderPass), each attachment is transitioned to its initial layout (attachment_description.initialLayout)
            //   2. Between subpasses, each used attachment is transitioned to the layout the subpass is expecting (subpass_description[i].color_attachment.layout)
            //   3. At the end of the render pass (vkCmdEndRenderPass), each attachment layout is transitioned to its final layout (attachment_description.finalLayout)
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            
            VkAttachmentReference color_attachment { };
        
            // attachment - index that is referenced by the layout(location = 0) out vec4 outColor line in the fragment shader
            // Note: this must also align with the first element in the attachment_descriptions array initialized above (order of attachment descriptions and attachment references matters), as this specifies which attachment is being referred to
            color_attachment.attachment = 0;
            
            // layout - specify the layout the attachment uses during the subpass
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment;
            
            subpass_description.pDepthStencilAttachment = nullptr; // Used for depth buffering
            subpass_description.pResolveAttachments = nullptr; // Used for resolving multisampled color attachments
            subpass_description.pInputAttachments = nullptr; // readonly attachments
            subpass_description.pPreserveAttachments = nullptr; // Attachments that are not used in this subpass but that should be preserved for later subpasses
        
            render_pass_ci.subpassCount = 1;
            render_pass_ci.pSubpasses = &subpass_description;
            
            // Define subpass dependencies
            // Subpass dependencies ensure that operations in one subpass complete before operations in another subpass that depend on the same resources begin (synchronization)
            // Subpass dependencies specify memory and execution dependencies between subpasses

            
            // srcSubpass - index of the subpass to depend on
            // dstSubpass - index of the current subpass (dstSubpass depends on srcSubpass)
            
            // srcStageMask - Vulkan stages that need to finish executing in the dependent subpass (srcSubpass) before we can execute the current subpass (dstSubpass)
            // dstStageMask - Vulkan stages in the current subpass (dstSubpass) that cannot be executed until after all stages specified in srcStageMask are completed
            
            // srcAccessMask - Vulkan memory access types used by the dependent subpass (srcSubpass)
            // dstAccessMask - Vulkan memory access types used by the current subpass (dstSubpass)
            
            
            // Vulkan defines a set of implicit subpass dependencies:
            //   1. Between external and initial subpass - all external operations before the start of the render pass (before the call to vkCmdBeginRenderPass) must complete before the start of the first subpass
            //
            //      Implicit subpass structure:
            //        srcStageMask = top of the render pipeline (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), the beginning of the render pass
            //        srcAccessMask = 0 (no access)
            //        dstStageMask = earliest specified stage in the first subpass
            //        dstAccessMask = memory access required by the first subpass
            //
            //      This subpass ensures that any image layout transitions necessary for the first subpass occur before the specified subpass stage
            //
            //   2. Between each pair of adjacent subpasses - all operations from one subpass must complete before the next subpass starts
            //
            //      Implicit subpass structure:
            //        srcStageMask = last stage of the previous subpass
            //        srcAccessMask = memory access required by the previous subpass
            //        dstStageMask = earliest stage of the next subpass
            //        dstAccessMask = memory access required by the next subpass
            //
            //   3. Between the final subpass and external - all operations from the final subpass must complete before any external operations (after the call to vkCmdEndRenderPass) begin
            //
            //      Implicit subpass structure:
            //        srcStageMask = last stage of the final subpass
            //        srcAccessMask = memory access required by the final subpass
            //        dstStageMask = bottom of the render pipeline (VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT), the end of the render pass
            //        dstAccessMask = 0 (no access)
            //
            //      This subpass ensures that any image layout transitions necessary after the final subpass occur before after the specified subpass stage
            //
            // Setting srcSubpass to VK_SUBPASS_EXTERNAL means creating a dependency on the first implicit subpass (commands that need to be completed are those that happened before the start of the render pass)
            // Setting dstSubpass to VK_SUBPASS_EXTERNAL means creating a dependency on the third implicit subpass (commands that need to be completed are those that will happen after the end of the render pass)
            
            // The main purpose of external subpass dependencies is to deal with initial and final layouts of an attachment reference
            // If initialLayout is not the layout used in the first subpass, the external subpass performs the transition
            // If nothing else is specified, the layout transition will wait on nothing before it performs the transition (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) - this is problematic because the render pass may proceed without waiting for the swapchain image to be available (potentially resulting in writes to a swapchain image that is still being presented)
            
            VkSubpassDependency subpass_dependency { };

            // The first color attachment refers to an image retrieved from the swapchain (call to vkAcquireNextImageKHR)
            // We must ensure this image is available before any color operations (writes) happen
            // LOAD operations for color attachments happen in the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage, so this is a good stage to wait on to guarantee that the swapchain image is ready for color output
            //   ref: https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-load-operations
            
            subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpass_dependency.dstSubpass = 0;

            // Somewhere between the start of the pipeline and the color write stage (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT), the attachment will be transitioned from the initial layout into the layout for the first subpass
            subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependency.srcAccessMask = 0;
        
            subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
            render_pass_ci.dependencyCount = 1;
            render_pass_ci.pDependencies = &subpass_dependency;
        
            if (vkCreateRenderPass(device, &render_pass_ci, nullptr, &render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void initialize_framebuffers() override {
            // Render passes operate in conjunction with framebuffers
            // A framebuffer object represents the collection of attachments that a render pass instance uses
            
            // The image that we use for the color attachment of the render pass (initialized above) depends on the image returned by the swapchain
            // Hence, initialize a framebuffer per swapchain image and use the one that corresponds to the returned image when rendering
            for (unsigned i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                // The attachment index must match what is used in the render pass
                
                VkFramebufferCreateInfo framebuffer_create_info { };
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = render_pass;
                
                // This sample uses a single color attachment that is presented to the swapchain at the end of the render pass
                framebuffer_create_info.attachmentCount = 1;
                framebuffer_create_info.pAttachments = &swapchain_image_views[i];
                
                framebuffer_create_info.width = swapchain_extent.width;
                framebuffer_create_info.height = swapchain_extent.height;
                framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
                
                if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }
        
        void initialize_pipelines() override {
            // Load shaders
            VkShaderModule vertex_shader_module = load_shader("");
            VkShaderModule fragment_shader_module = load_shader("");
            
            // Bundle shader stages
            VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info { };
            vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertex_shader_stage_create_info.module = vertex_shader_module;
            vertex_shader_stage_create_info.pName = "main"; // TODO: define programmatically with reflection
    
            VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info { };
            fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragment_shader_stage_create_info.module = fragment_shader_module;
            fragment_shader_stage_create_info.pName = "main";
            
            // Assign shader stages to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_create_info, fragment_shader_stage_create_info };
            
            // Describe the format of the vertex data passed to the vertex shader
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        
            // Vertex bindings specify the spacing between data and whether data is per-vertex or per-instance
            std::array<VkVertexInputBindingDescription, 1> vertex_binding_descriptions { };
        
            // binding - binding point of the buffer used to read data from
            vertex_binding_descriptions[0].binding = 0;
            
            // stride - number of bytes between consecutive elements in the buffer
            vertex_binding_descriptions[0].stride = sizeof(Vertex);
            
            // inputRate - specifies whether this data is updated per vertex or per instance (for instanced rendering)
            //   - VK_VERTEX_INPUT_RATE_VERTEX - updated per vertex
            //   - VK_VERTEX_INPUT_RATE_INSTANCE - updated per instance
            vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            vertex_input_create_info.vertexBindingDescriptionCount = 1;
            vertex_input_create_info.pVertexBindingDescriptions = vertex_binding_descriptions.data();
        
            // Vertex attributes describe how to extract individual vertex data from a binding description (done above)
            // This also depends on if the vertex data is presented as a SoA or AoS
            std::array<VkVertexInputAttributeDescription, 2> vertex_attribute_descriptions { };
            
            // Describe how to retrieve the vertex position from the vertex buffer
            vertex_attribute_descriptions[0].binding = 0; // Buffer bound at binding point 0
            vertex_attribute_descriptions[0].location = 0; // Location attribute, specified by layout (location = 0)
            
            // format - the format of the data type of the attribute
            //   - float: VK_FORMAT_R32_SFLOAT
            //   - double: VK_FORMAT_R64_SFLOAT
            //   - vec2: VK_FORMAT_R32G32_SFLOAT
            //   - vec3: VK_FORMAT_R32G32B32_SFLOAT
            //   - vec4: VK_FORMAT_R32G32B32A32_SFLOAT
            //   - ivec2: VK_FORMAT_R32G32_SINT
            //   - uvec4: VK_FORMAT_R32G32B32A32_UINT
            
            // If a format that is larger (has more channels) than the format of the input shader type is specified, the data in the extra channels will be silently discarded by the vertex shader
            // An example of this would be specifying the vertex position has format VK_FORMAT_R32G32B32_SFLOAT, when the shader expects the format of a vec2 (the B32 component will be discarded)
            
            // If a format that is smaller (has fewer channels) than the input shader type is specified, the remaining components will use default values (0.0 for color, 1.0 for alpha)
            // An example of this would be specifying the vertex color has format VK_FORMAT_R32G32_SFLOAT, when the shader expects the format of a vec3 (the G32 component will contain the default color value 0.0)
            vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            
            vertex_attribute_descriptions[0].offset = 0; // Offset of the vertex position relative to the start of a Vertex struct
        
            // Describe how to retrieve the vertex color from the vertex buffer
            vertex_attribute_descriptions[1].binding = 0;
            vertex_attribute_descriptions[1].location = 1; // layout (location = 1)
            vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
            vertex_attribute_descriptions[0].offset = sizeof(glm::vec2); // Attribute data is interleaved in the buffer and reads to access vertex color must be offset from the start of the Vertex
        
            vertex_input_create_info.vertexAttributeDescriptionCount = 1;
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();
        
            // Input assembly describes what kind of geometry is being drawn (topology) and if primitive restart is enabled (primitiveRestartEnable), which allows breaking up STRIP topology modes
            VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info { };
            input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            
            // Specify whether input vertices are points, a list of lines / triangles, or strips
            // Topology options: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
            input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            
            // Specifying VK_TRUE here breaking up lines + triangles with a special index (0xFFFF, 0xFFFFFFFF) when using STRIP topology methods
            input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
        
            // The viewport describes the region of the framebuffer that the output will be rendered to
            VkViewport viewport { };
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapchain_extent.width);
            viewport.height = static_cast<float>(swapchain_extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
        
            // The scissor region defines the portion of the viewport that will be drawn to the screen
            VkRect2D scissor { };
            scissor.offset = { 0, 0 };
            scissor.extent = swapchain_extent;
        
            // Note: using VkPipelineViewportStateCreateInfo to set the viewport/scissor state as part of the pipeline makes them immutable
            // Viewport size, scissor state, line width, blend constants, etc... can instead be marked as dynamic state so that they can be instead specified at draw time (without requiring the recompilation of the entire pipeline)
            // Note: using dynamic states causes the configuration of these values in the pipeline to be ignored in the VkPipelineViewportStateCreateInfo struct above and required to be specified at draw time
            //    std::vector<VkDynamicState> dynamic_states {
            //        VK_DYNAMIC_STATE_VIEWPORT,
            //        VK_DYNAMIC_STATE_SCISSOR
            //    };
            //
            //    VkPipelineDynamicStateCreateInfo dynamic_state_create_info { };
            //    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            //    dynamic_state_create_info.dynamicStateCount = static_cast<u32>(dynamic_states.size());
            //    dynamic_state_create_info.pDynamicStates = dynamic_states.data();
            
            VkPipelineViewportStateCreateInfo viewport_create_info { };
            viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_create_info.viewportCount = 1;
            viewport_create_info.pViewports = &viewport;
            viewport_create_info.scissorCount = 1;
            viewport_create_info.pScissors = &scissor;
        
            // Rasterization properties
            VkPipelineRasterizationStateCreateInfo rasterizer_create_info { };
            rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer_create_info.depthClampEnable = VK_FALSE; // Fragments beyond the near/far depth planes are clamped instead of discarded
            rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE; // Discards any / all output fragments to the framebuffer
        
            // polygonMode - specifies how fragments are generated and passed to the fragment shader (using anything other than FILL requires enabling a GPU feature)
            //   - VK_POLYGON_MODE_FILL - fill the area of the polygon with fragments (fill and shade polygons fully)
            //   - VK_POLYGON_MODE_LINE - polygon edges are drawn as lines
            //   - VK_POLYGON_MODE_POINT - polygon vertices are drawn as points
            rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
            
            rasterizer_create_info.lineWidth = 1.0f;
            rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer_create_info.depthBiasEnable = VK_FALSE;
            rasterizer_create_info.depthBiasConstantFactor = 0.0f;
            rasterizer_create_info.depthBiasClamp = 0.0f;
            rasterizer_create_info.depthBiasSlopeFactor = 0.0f;
        
            // Multisampling is disabled for this sample
            VkPipelineMultisampleStateCreateInfo multisampling_create_info { };
            multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling_create_info.sampleShadingEnable = VK_FALSE;
            multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling_create_info.minSampleShading = 1.0f;
            multisampling_create_info.pSampleMask = nullptr;
            multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
            multisampling_create_info.alphaToOneEnable = VK_FALSE;
        
            // Color blending
            //   VkPipelineColorBlendAttachmentState configures color blending per attached framebuffer.
            //   if (blendEnable) {
            //       finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
            //       finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
            //   } else {
            //       finalColor = newColor;
            //   }
            //   finalColor = finalColor & colorWriteMask;
            
            VkPipelineColorBlendAttachmentState color_blend_attachment_create_info { };
            color_blend_attachment_create_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment_create_info.blendEnable = VK_FALSE;
            color_blend_attachment_create_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment_create_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment_create_info.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment_create_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment_create_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment_create_info.alphaBlendOp = VK_BLEND_OP_ADD;
        
            VkPipelineColorBlendStateCreateInfo color_blend_create_info { };
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.logicOpEnable = VK_FALSE; // Use a bitwise operation to combine the old and new color values (setting this to true disables color mixing (specified above) for all framebuffers)
            color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_create_info.attachmentCount = 1;
            color_blend_create_info.pAttachments = &color_blend_attachment_create_info;
            color_blend_create_info.blendConstants[0] = 0.0f;
            color_blend_create_info.blendConstants[1] = 0.0f;
            color_blend_create_info.blendConstants[2] = 0.0f;
            color_blend_create_info.blendConstants[3] = 0.0f;
        
            // This sample does not use any uniforms or push constants
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 0;
            pipeline_layout_create_info.pSetLayouts = nullptr;
            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        
            // The pipeline combines:
            //   - shader stages that define the functionality of the programmable stages of the graphics pipeline.
            //   - pipeline fixed-function state (non-programmable pipeline steps such as input descriptions, input assembly, rasterization, color blending, etc.)
            //   - the uniform and push constant values referenced by the shaders
            //   - the attachments referenced by the pipeline stages in their render pass and their usage
            VkGraphicsPipelineCreateInfo pipeline_create_info { };
            pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
            pipeline_create_info.pStages = shader_stages;
            pipeline_create_info.pVertexInputState = &vertex_input_create_info;
            pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
            pipeline_create_info.pViewportState = &viewport_create_info;
            pipeline_create_info.pRasterizationState = &rasterizer_create_info;
            pipeline_create_info.pMultisampleState = &multisampling_create_info;
            pipeline_create_info.pDepthStencilState = nullptr; // Sample does not use depth / stencil buffers
            pipeline_create_info.pColorBlendState = &color_blend_create_info;
            pipeline_create_info.pDynamicState = nullptr;
            pipeline_create_info.layout = pipeline_layout;
            pipeline_create_info.renderPass = render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline!");
            }
            
            // SPIR-V bytecode compilation and linking happens when the graphics pipeline is created
            // The modules are not necessary after the pipeline is initialized
            vkDestroyShaderModule(device, fragment_shader_module, nullptr);
            vkDestroyShaderModule(device, vertex_shader_module, nullptr);
        }
        
        void render() override {
            // The base sample uses multiple frames in flight to avoid forcing the CPU to wait on the GPU to finish rendering the previous frame to start rendering a new one
            // With multiple frames in flight, the GPU can be rendering one frame while the CPU is recording commands for rendering another
            // This requires separate command buffers and synchronization primitives (semaphores, fences) per frame in flight to avoid any interference across two frames
            
            // The render function is kicked off once the resources associated with the current frame (frame_index) are no longer in use by the GPU
            // The sample is free to use the resources identified above at index frame_index to begin recording commands for rendering a new frame
            // The Sample base also takes care of presenting the finished frame to the screen
            
            VkCommandBuffer command_buffer = command_buffers[frame_index];
            VkSemaphore is_image_available = is_presentation_complete[frame_index];
            
            // Retrieve the index of the swapchain image to use for this frame
            // Note that this may differ from frame_index as this is controlled by swapchain internals
            unsigned image_index;
            VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<std::uint64_t>::max(), is_image_available, VK_NULL_HANDLE, &image_index);
            // TODO: handle different return values
    
            // Record command buffer(s)
            vkResetCommandBuffer(command_buffer, 0);
            
            // TODO:
            // record_command_buffers(image_index);
    
            VkSubmitInfo submit_info { };
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Ensure that the swapchain image is available before executing any color operations (writes) by waiting on the pipeline stage that writes to the color attachment (discussed in detail during render pass creation above)
            // Another approach that can be taken here is to wait on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, which would ensure that no command buffers are executed before the image swapchain image is ready (vkAcquireNextImageKHR signals is_image_available, queue execution waits on is_image_available)
            // However, this is not the preferred approach - waiting on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT will completely block the pipeline until the swapchain image is ready
            // Instead, waiting on the pipeline stage where writes are performed to the color attachment allows Vulkan to begin scheduling other work that happens before the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage is reached for execution (such as invoking the vertex shader)
            // This way, the implementation waits only the time that is absolutely necessary for coherent memory operations
            
            VkSemaphore wait_semaphores[] = { is_image_available }; // Semaphore(s) to wait on before the command buffers can begin execution
            VkPipelineStageFlags wait_stage_flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // Note: wait_stage_flags and wait_semaphores have a 1:1 correlation, meaning it is possible to wait on and signal different semaphores at different pipeline stages
            
            // Waiting on the swapchain image to be ready (if not yet) when the pipeline is ready to perform writes to color attachments
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = wait_semaphores;
            submit_info.pWaitDstStageMask = wait_stage_flags;
            
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer; // Command buffer(s) to execute
    
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &is_rendering_complete[frame_index]; // Semaphore to signal when all command buffer(s) have finished executing
    
            // Submit
            if (vkQueueSubmit(queue, 1, &submit_info, is_frame_in_flight[frame_index]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }
        
};

DEFINE_SAMPLE_MAIN(HelloTriangle);