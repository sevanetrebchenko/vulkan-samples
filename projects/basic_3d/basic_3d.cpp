
#include "sample.hpp"
#include "helpers.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// Basic scene for demonstrating rendering of 3D models

class Basic3D final : public Sample {
    public:
        Basic3D() : Sample("Basic 3D"),
                    model({ }),
                    vertex_buffer(nullptr),
                    vertex_buffer_memory(nullptr),
                    index_buffer(nullptr),
                    index_buffer_memory(nullptr),
                    pipeline(nullptr),
                    pipeline_layout(nullptr),
                    descriptor_sets({ }) {
        }
        
        ~Basic3D() override {
        }
        
    private:
        Model model;
        
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        
        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;
        
        VkDescriptorPool descriptor_pool;
        struct DescriptorSets {
            VkDescriptorSet global;
            VkDescriptorSetLayout global_layout;
            
            VkDescriptorSet object;
            VkDescriptorSetLayout object_layout;
        };
        std::array<DescriptorSets, NUM_FRAMES_IN_FLIGHT> descriptor_sets;
        
        std::array<VkBuffer, NUM_FRAMES_IN_FLIGHT> uniform_buffers;
        std::array<VkDeviceMemory, NUM_FRAMES_IN_FLIGHT> uniform_buffer_memory;
        std::array<void*, NUM_FRAMES_IN_FLIGHT> uniform_buffer_mapped;
        
        VkQueueFlags request_device_queues() const override {
            return VK_QUEUE_TRANSFER_BIT; // Demo needs transfer support for staging buffers
        }
        
        void record_command_buffers(unsigned image_index) override {
            VkCommandBuffer command_buffer = command_buffers[frame_index];
            
            VkCommandBufferBeginInfo command_buffer_begin_info { };
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.flags = 0;
            if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin command buffer recording!");
            }
        
            VkRenderPassBeginInfo render_pass_info { };
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = render_pass;
            render_pass_info.framebuffer = framebuffers[image_index]; // Bind the framebuffer for the swapchain image being rendered to
        
            // Specify render area and offset
            render_pass_info.renderArea.offset.x = 0;
            render_pass_info.renderArea.offset.y = 0;
            render_pass_info.renderArea.extent = swapchain_extent;
        
            VkClearValue clear_values[2] { };

            // Clear value for color attachment
            clear_values[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

            // Clear value for depth/stencil attachment
            clear_values[1].depthStencil = { 1.0f, 0 };
            
            render_pass_info.clearValueCount = 2;
            render_pass_info.pClearValues = clear_values;
        
            // Record command for starting the render pass
            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            
                // Bind graphics pipeline
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                
                unsigned set = 0;
                
                // Bind pipeline-global descriptor sets
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, set, 1, &descriptor_sets[frame_index].global, 0, nullptr);
                
                // Bind vertex buffer
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
                
                // Bind index buffer
                vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
            
                // Bind descriptor sets
                set = 1;
                
                // Bind the global descriptor set at set 0
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, set, 1, &descriptor_sets[frame_index].object, 0, nullptr);
                
                // Draw indices.size() vertices which make up 1 instance starting at vertex index 0 and instance index 0.
                vkCmdDrawIndexed(command_buffer, model.indices.size(), 1, 0, 0, 0);
                
            // Finish recording the command buffer
            vkCmdEndRenderPass(command_buffer);
            
            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
        
        void initialize_pipelines() override {
            // Load shaders
            VkShaderModule vertex_shader_module = load_shader("shaders/blinn_phong.vert");
            VkShaderModule fragment_shader_module = load_shader("shaders/blinn_phong.frag");
            
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
            VkVertexInputBindingDescription vertex_binding_description { };
            vertex_binding_description.binding = 0;
            vertex_binding_description.stride = sizeof(glm::vec3) * 2; // vertex position + vertex normal
            vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            vertex_input_create_info.vertexBindingDescriptionCount = 1;
            vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding_description;
        
            // Vertex attributes describe how to extract individual vertex data from a binding description (done above)
            VkVertexInputAttributeDescription vertex_attribute_descriptions[2] { };
            
            // Describe how to retrieve the vertex position from the vertex buffer
            vertex_attribute_descriptions[0].binding = 0; // Buffer bound at binding point 0
            vertex_attribute_descriptions[0].location = 0; // Location attribute, specified by layout (location = 0)
            
            // format - the format of the data type of the attribute
            vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
            vertex_attribute_descriptions[0].offset = 0;
        
            // Describe how to retrieve the vertex normal
            vertex_attribute_descriptions[1].binding = 0;
            vertex_attribute_descriptions[1].location = 1; // layout (location = 1)
            vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
            vertex_attribute_descriptions[1].offset = sizeof(glm::vec3);
        
            vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
        
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
            rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;
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
        
            // Depth/stencil buffers
            VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info { };
            depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable = VK_TRUE;
            depth_stencil_create_info.depthWriteEnable = VK_TRUE;
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS; // Fragments that are closer have a lower depth value and should be kept (fragments further away are discarded)
            
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
            
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            
            VkDescriptorSetLayout layouts[2] = { descriptor_sets[0].global_layout, descriptor_sets[0].object_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        
            VkGraphicsPipelineCreateInfo pipeline_create_info { };
            pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
            pipeline_create_info.pStages = shader_stages;
            pipeline_create_info.pVertexInputState = &vertex_input_create_info;
            pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
            pipeline_create_info.pViewportState = &viewport_create_info;
            pipeline_create_info.pRasterizationState = &rasterizer_create_info;
            pipeline_create_info.pMultisampleState = &multisampling_create_info;
            pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
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
            
            vkDestroyShaderModule(device, fragment_shader_module, nullptr);
            vkDestroyShaderModule(device, vertex_shader_module, nullptr);
        }
        
        void destroy_pipelines() override {
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        
        void initialize_render_passes() override {
            // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-objects
            // A render pass object represents a collection of attachments, subpasses, and dependencies between the subpasses
            // It describes how the attachments are used over the course of a set of subpasses
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            
            // This render pass uses one color attachment (0) and one depth attachment (1)
            VkAttachmentDescription attachment_descriptions[2] { };
        
            // Since this will be presented to the swapchain, use the format of the surface
            attachment_descriptions[0].format = surface_format.format;
            
            // This example does not use multisampling
            attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        
            // Clear the attachment to a uniform value (defined in VkRenderPassBeginInfo)
            attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        
            // Store the color for presentation to the swapchain
            attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
            // This example does not use a stencil buffer
            attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
            // Render pass images need to be transitioned to specific layouts that are suitable for the next operation
        
            // The color attachment was previously used for presenting before being returned to the swapchain
            // It doesn't matter what the contents of the attachment are
            attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        
            // The color attachment should be transitioned to a format that is suitable to be presented to the swapchain at the end of the render pass
            attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            
            // Depth attachment
            attachment_descriptions[1].format = depth_buffer_format; // Selected depth format, as supported by the physical device
            attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Depth buffer is not used after render pass is complete
            
            render_pass_create_info.attachmentCount = 2;
            render_pass_create_info.pAttachments = attachment_descriptions;
            
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
            
            VkAttachmentReference depth_attachment { };
            depth_attachment.attachment = 1;
            depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            
            subpass_description.pDepthStencilAttachment = &depth_attachment;
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
        
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            
            // Define subpass dependencies
            
            // srcSubpass - index of the subpass to depend on
            // dstSubpass - index of the current subpass (dstSubpass depends on srcSubpass)
            
            // srcStageMask - Vulkan stages that need to finish executing in the dependent subpass (srcSubpass) before we can execute the current subpass (dstSubpass)
            // dstStageMask - Vulkan stages in the current subpass (dstSubpass) that cannot be executed until after all stages specified in srcStageMask are completed
            
            // srcAccessMask - Vulkan memory access types used by the dependent subpass (srcSubpass)
            // dstAccessMask - Vulkan memory access types used by the current subpass (dstSubpass)
            
            VkSubpassDependency subpass_dependencies[2] { };

            // Dependency for transitioning the color attachment at the right time
            subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            subpass_dependencies[0].dstSubpass = 0;
            subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependencies[0].srcAccessMask = 0;
            subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
            // Dependency for transitioning the depth attachment at the right time
            subpass_dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
            subpass_dependencies[1].dstSubpass = 0;
            subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // Depth LOAD operations to clear the depth buffer happens at the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT pipeline stage
            subpass_dependencies[1].srcAccessMask = 0;
            subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            subpass_dependencies[1].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // LOAD operation clears, STORE operation writes
            
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
        
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void destroy_render_passes() override {
            vkDestroyRenderPass(device, render_pass, nullptr);
        }
        
        void initialize_framebuffers() override {
            for (unsigned i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkFramebufferCreateInfo framebuffer_create_info { };
                
                // The attachment index must match what is used in the render pass
                VkImageView attachments[] = { swapchain_image_views[i], depth_buffer_view };
                
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = render_pass;
                framebuffer_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
                framebuffer_create_info.pAttachments = attachments;
                framebuffer_create_info.width = swapchain_extent.width;
                framebuffer_create_info.height = swapchain_extent.height;
                framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
                
                if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }
        
        void initialize_geometry_buffers() {
            model = load_model("assets/models/bunny_high_poly.obj");
            
            // TODO: consolidate memory usage across vertex and index buffers?
            
            // This sample only uses vertex position and normal
            std::size_t vertex_buffer_size = model.vertices.size() * sizeof(Model::Vertex);
            std::size_t index_buffer_size = model.indices.size() * sizeof(unsigned);
            
            // In order to be able to write vertex data into the vertex buffer, the buffer needs to (at least) support VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT memory use (+ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, otherwise requires the use of vkFlushMappedMemoryRanges to make the data visible to the GPU)
            // The memory heap this buffer is allocated from is likely not in the optimal layout for the GPU to read from
            // To get around this, this example uses a two phase approach to upload vertex data to the GPU:
            //   1. Create a staging buffer that is visible from the CPU and write the vertex data into this buffer
            //   2. Use a buffer copy command to move the data from the staging buffer (SRC) into device local memory (DST), which has the optimal layout for the GPU to read data from
            
            VkBuffer staging_buffer { };
            VkDeviceMemory staging_buffer_memory { };
            VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkMemoryPropertyFlags staging_buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            
            create_buffer(physical_device, device, vertex_buffer_size + index_buffer_size, staging_buffer_usage, staging_buffer_memory_properties, staging_buffer, staging_buffer_memory);
            
            // Upload vertex data into staging buffer
            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, vertex_buffer_size, 0, &data);
                memcpy(data, model.vertices.data(), vertex_buffer_size);
            vkUnmapMemory(device, staging_buffer_memory);
            
            // Upload index data into staging buffer
            vkMapMemory(device, staging_buffer_memory, vertex_buffer_size, index_buffer_size, 0, &data);
                memcpy(data, model.indices.data(), index_buffer_size);
            vkUnmapMemory(device, staging_buffer_memory);
            
            // Create device-local buffers
            VkBufferUsageFlags vertex_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags vertex_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, vertex_buffer_size, vertex_buffer_usage, vertex_buffer_memory_properties, vertex_buffer, vertex_buffer_memory);
            
            VkBufferUsageFlags index_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT; // This buffer is the destination buffer in a memory transfer operation (and also the index buffer).
            VkMemoryPropertyFlags index_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, index_buffer_size, index_buffer_usage, index_buffer_memory_properties, index_buffer, index_buffer_memory);
            
            // Allocate a transient command pool for transient (one-time) commands
            VkCommandBuffer command_buffer = allocate_transient_command_buffer();
            VkCommandBufferBeginInfo begin_info { };
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(command_buffer, &begin_info);
            
            copy_buffer(command_buffer, staging_buffer, 0, vertex_buffer, 0, vertex_buffer_size);
            copy_buffer(command_buffer, staging_buffer, vertex_buffer_size, index_buffer, 0, index_buffer_size);
            
            vkEndCommandBuffer(command_buffer);
            
            VkSubmitInfo submit_info { };
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

            // This transfer is a one-time operation, easier just to wait for it to complete
            // An alternative approach here would be to use a fence, which would allow scheduling multiple transfers in parallel and give the GPU more opportunities to optimize
            vkQueueWaitIdle(queue);
            
            deallocate_transient_command_buffer(command_buffer);
            
            // Staging buffer resources are no longer necessary
            vkFreeMemory(device, staging_buffer_memory, nullptr);
            vkDestroyBuffer(device, staging_buffer, nullptr);
        }
        
        void destroy_geometry_buffers() {
            vkDestroyBuffer(device, index_buffer, nullptr);
            vkFreeMemory(device, index_buffer_memory, nullptr);

            vkDestroyBuffer(device, vertex_buffer, nullptr);
            vkFreeMemory(device, vertex_buffer_memory, nullptr);
        }
        
        void initialize_descriptor_pool() override {
            // A descriptor is a handle to a resource (such as a buffer or a sampler)
            // Descriptors also hold extra information such as the size of the buffer or the type of sampler
            
            // Descriptors are bound together into descriptor sets (Vulkan does not allow binding individual resources in shaders, this operation must be done in sets)
            // There is a limit to how many descriptor sets different devices support
            
            // This sample allocates 2 descriptor sets based on binding frequency
            //   1. Global resources, bound once per frame (contains information about the scene camera)
            //   2. Per-object resources, bound once per object (contains information about model transforms and material properties)
            
            VkDescriptorPoolSize descriptor_pool_size { };
            descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Descriptor sets allocated from this pool are to be used as descriptor sets for uniform buffers
            
            // Allocate a descriptor set per frame in flight to prevent writing to uniform buffers of one frame while they are still in use by the rendering operations of the previous frame
            descriptor_pool_size.descriptorCount = 3 * NUM_FRAMES_IN_FLIGHT;
            
            VkDescriptorPoolCreateInfo descriptor_pool_create_info { };
            descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptor_pool_create_info.poolSizeCount = 1;
            descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
            descriptor_pool_create_info.maxSets = 3 * NUM_FRAMES_IN_FLIGHT;
            
            if (vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }
        }
        
        void destroy_descriptor_pool() {
            // Destroying the descriptor pool automatically destroys the descriptor sets allocated from this pool
            vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
        }
        
        void initialize_descriptor_sets() override {
            // All descriptor sets are allocated from the same descriptor pool, allocated at the start of the frame
            // This sample aims to separate descriptor sets based on binding frequency
            
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkWriteDescriptorSet descriptor_set_writes[3] { };
                VkDescriptorBufferInfo buffer_infos[3] { };
                std::size_t offset = 0u;
                
                // Allocate descriptor set 0 for global uniforms
                // The global set is bound once per pipeline
                
                // This set has one binding at location 0
                // This binding references a uniform buffer and can be used in both the vertex and fragment stages
                VkDescriptorSetLayoutBinding binding = create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
                
                // Initialize the descriptor set layout
                VkDescriptorSetLayoutCreateInfo layout_create_info { };
                layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_create_info.bindingCount = 1;
                layout_create_info.pBindings = &binding;
                if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &descriptor_sets[i].global_layout) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate global descriptor set layout!");
                }
                
                // Initialize the descriptor set
                VkDescriptorSetAllocateInfo set_allocate_info { };
                set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                set_allocate_info.descriptorPool = descriptor_pool;
                set_allocate_info.descriptorSetCount = 1;
                set_allocate_info.pSetLayouts = &descriptor_sets[i].global_layout;
                if (vkAllocateDescriptorSets(device, &set_allocate_info, &descriptor_sets[i].global) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate global descriptor set!");
                }
                
                // Specify the range of data and buffer where data for this descriptor set is located
                
                buffer_infos[0].buffer = uniform_buffers[i];
                buffer_infos[0].offset = 0;
                buffer_infos[0].range = (sizeof(glm::mat4) * 2) + sizeof(glm::vec4);
                offset += align_to_device_boundary(physical_device, buffer_infos[0].range);
                
                descriptor_set_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_set_writes[0].dstSet = descriptor_sets[i].global;
                descriptor_set_writes[0].dstBinding = 0;
                descriptor_set_writes[0].dstArrayElement = 0;
                descriptor_set_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_set_writes[0].descriptorCount = 1;
                descriptor_set_writes[0].pBufferInfo = &buffer_infos[0];
                
                // Allocate descriptor set 1 for per-object uniforms
                // In theory, it would be better to separate this set into two (per-material and per-object) and render objects grouped by material, but for this sample each object has unique material properties
                
                // This set has two bindings at locations 0 and 1
                VkDescriptorSetLayoutBinding bindings[2] = {
                    // Binding at location 0 references a uniform buffer that can be used in the vertex stage
                    create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
                    create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                };
                
                VkDescriptorSetLayoutCreateInfo vertex_layout_create_info { };
                vertex_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                vertex_layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
                vertex_layout_create_info.pBindings = bindings;
                
                if (vkCreateDescriptorSetLayout(device, &vertex_layout_create_info, nullptr, &descriptor_sets[i].object_layout) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate per-model descriptor set layout!");
                }
                
                VkDescriptorSetAllocateInfo vertex_set_allocate_info { };
                vertex_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                vertex_set_allocate_info.descriptorPool = descriptor_pool;
                vertex_set_allocate_info.descriptorSetCount = 1;
                vertex_set_allocate_info.pSetLayouts = &descriptor_sets[i].object_layout;
                if (vkAllocateDescriptorSets(device, &vertex_set_allocate_info, &descriptor_sets[i].object) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate per-model descriptor set!");
                }
                
                // Specify the range of data and buffer where data for the vertex binding is located
                buffer_infos[1].buffer = uniform_buffers[i];
                buffer_infos[1].offset = offset;
                buffer_infos[1].range = sizeof(glm::mat4) * 2;
                offset += align_to_device_boundary(physical_device, buffer_infos[1].range);
                
                descriptor_set_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_set_writes[1].dstSet = descriptor_sets[i].object;
                descriptor_set_writes[1].dstBinding = 0;
                descriptor_set_writes[1].dstArrayElement = 0;
                descriptor_set_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_set_writes[1].descriptorCount = 1;
                descriptor_set_writes[1].pBufferInfo = &buffer_infos[1];
                
                // Specify the range of data and buffer where data for the fragment binding is located
                buffer_infos[2].buffer = uniform_buffers[i];
                buffer_infos[2].offset = offset;
                buffer_infos[2].range = (sizeof(glm::vec4) * 3) + 4;
                offset += align_to_device_boundary(physical_device, buffer_infos[2].range);
                
                descriptor_set_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_set_writes[2].dstSet = descriptor_sets[i].object;
                descriptor_set_writes[2].dstBinding = 1;
                descriptor_set_writes[2].dstArrayElement = 0;
                descriptor_set_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_set_writes[2].descriptorCount = 1;
                descriptor_set_writes[2].pBufferInfo = &buffer_infos[2];
                
                // Specify the buffer and region within it that contains the data for the allocated descriptors
                vkUpdateDescriptorSets(device, sizeof(descriptor_set_writes) / sizeof(descriptor_set_writes[0]), descriptor_set_writes, 0, nullptr);
            }
        }
        
        // Shader alignment rules:
        //   - scalars have to be aligned to 4-byte boundaries
        //   - vec2 must be aligned to an 8-byte boundary
        //   - vec3 must be aligned to a 16-byte boundary
        //   - mat4 must be aligned to a 16-byte boundary
        //   - nested structure must be aligned to the alignment of the structure rounded up to a multiple of 16
        
        void initialize_uniform_buffers() override {
//            // set 0 (vertex) + set 0 (fragment) + set 1 (vertex) + set 1 (fragment)
//            // If sub-allocated from the same buffer, different sets of uniform data need to be aligned with the minUniformBufferOffsetAlignment field from the device physical properties
            std::size_t uniform_buffer_size = align_to_device_boundary(physical_device, (sizeof(glm::mat4) * 2) + sizeof(glm::vec4)) + align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2) + align_to_device_boundary(physical_device, sizeof(glm::vec4) * 3 + 4);
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                create_buffer(physical_device, device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers[i], uniform_buffer_memory[i]);
                vkMapMemory(device, uniform_buffer_memory[i], 0, uniform_buffer_size, 0, &uniform_buffer_mapped[i]);
            }
        }
        
        void update_uniform_buffers(unsigned image_index) override {
            struct CameraData {
                glm::mat4 view;
                glm::mat4 projection;
                glm::vec3 eye;
            };
            CameraData globals { };
            globals.view = camera.get_view_matrix();
            globals.projection = camera.get_projection_matrix();
            globals.eye = camera.get_position();
            memcpy(uniform_buffer_mapped[frame_index], &globals, sizeof(CameraData));
            
            struct ObjectData {
                glm::mat4 model;
                glm::mat4 normal;
            };
            ObjectData object { };
            object.model = glm::scale(glm::vec3(3.0f));
            object.normal = glm::inverse(glm::transpose(object.model));
            
            std::size_t offset = align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2 + sizeof(glm::vec4));
            
            memcpy((void*)((const char*) (uniform_buffer_mapped[frame_index]) + offset), &object, sizeof(ObjectData));
            offset += align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2);
            
            struct MaterialData {
                // Must be aligned to vec4
                glm::vec4 ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
                glm::vec4 diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                glm::vec4 specular = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                float exponent = 15.0f;
            };
            MaterialData material { };
            
            memcpy((void*)((const char*) (uniform_buffer_mapped[frame_index]) + offset), &material, sizeof(MaterialData));
            offset += align_to_device_boundary(physical_device, sizeof(glm::vec4) * 3 + 4);
        }
        
        void destroy_uniform_buffer() {
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                vkFreeMemory(device, uniform_buffer_memory[i], nullptr);
                vkDestroyBuffer(device, uniform_buffers[i], nullptr);
            }
        }
        
        void initialize_resources() override {
            initialize_geometry_buffers();
        }
        
        void destroy_resources() override {
            destroy_geometry_buffers();
        }
        
};

DEFINE_SAMPLE_MAIN(Basic3D);
