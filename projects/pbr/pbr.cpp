
#include "sample.hpp"
#include "helpers.hpp"
#include "vulkan_initializers.hpp"
#include "loaders/obj.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan requires depth values to range [0.0, 1.0], not the default [-1.0, 1.0] that OpenGL uses
#include <glm/gtx/transform.hpp>
#include <string> // std::string, std::to_string

class PBR final : public Sample {
    public:
        PBR() : Sample("Physically-Based Rendering") {
            enabled_physical_device_features.geometryShader = (VkBool32) true;
            
            camera.set_position(glm::vec3(0, 0, 12));
            camera.set_look_direction(glm::vec3(0.0f, 0.0f, -1.0f));
        }
        
        ~PBR() override {
        }
        
    private:
        // PBR scene consists of an array of models to showcase different material properties
        Model model;
        std::vector<Transform> transforms;
        
        // Sphere geometry buffers
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        
        VkDescriptorSetLayout global_descriptor_set_layout;
        VkDescriptorSet global_descriptor_set;
        
        VkDescriptorSetLayout object_descriptor_set_layout;
        std::vector<VkDescriptorSet> object_descriptor_sets;
        
        // Uniform buffer layout:
        //
        
        struct GlobalUniforms {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec3 camera_position;
            int debug_view;
        };
        
        struct ObjectUniforms {
            glm::mat4 model;
            glm::mat4 normal;
        };
        
        struct MaterialUniforms {
        };

        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
        VkRenderPass render_pass;
        
        VkBuffer uniform_buffer;
        VkDeviceMemory uniform_buffer_memory;
        void* uniform_buffer_mapped;

        void initialize_resources() override {
            initialize_transforms();
            
            initialize_buffers();
            
            initialize_render_pass();
            initialize_framebuffers();

            initialize_descriptor_pool(1 + transforms.size(), 0);
            
            initialize_uniform_buffer();
            
            initialize_global_descriptor_set();
            initialize_object_descriptor_sets();

            initialize_pipeline();
        }
        
        void destroy_resources() override {
            destroy_pipelines();
            destroy_descriptor_sets();
            destroy_uniform_buffer();
            destroy_buffers();
            destroy_render_passes();
        }
        
        void update() override {
            update_uniform_buffers();
        }
        
        void record_command_buffers(unsigned image_index) override {
            VkCommandBuffer command_buffer = command_buffers[frame_index];
            vkResetCommandBuffer(command_buffer, 0);
            
            VkCommandBufferBeginInfo command_buffer_begin_info { };
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.flags = 0;
            if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin command buffer recording!");
            }
            
            VkRenderPassBeginInfo render_pass_info { };
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = render_pass;
            render_pass_info.framebuffer = present_framebuffers[image_index];
            render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
            
            // Composition pass
            VkClearValue clear_values[2] { };
            clear_values[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            clear_values[1].depthStencil = { 1.0f, 0 };
            render_pass_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
            render_pass_info.pClearValues = clear_values;
            
            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                // Bind graphics pipeline
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                // Bind global descriptor set
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &global_descriptor_set, 0, nullptr);
                
                // TODO: convert to instanced rendering
                for (std::size_t i = 0u; i < transforms.size(); ++i) {
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &object_descriptor_sets[i], 0, nullptr);
                    
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
                    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
                    
                    vkCmdDrawIndexed(command_buffer, (unsigned) model.indices.size(), 1, 0, 0, 0);
                }
                
            vkCmdEndRenderPass(command_buffer);
            
            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
        
        void initialize_pipeline() {
            VkVertexInputBindingDescription vertex_binding_descriptions[] {
                create_vertex_binding_description(0, sizeof(glm::vec3) * 2, VK_VERTEX_INPUT_RATE_VERTEX) // One element is vertex position (vec3) + normal (vec3)
            };
            
            // Vertex attributes describe how to extract individual vertex data from a binding description (done above)
            VkVertexInputAttributeDescription vertex_attribute_descriptions[] {
                create_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0), // Vertex position
                create_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)), // Vertex normal
            };
            
            // Describe the format of the vertex data passed to the vertex shader
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_create_info.vertexBindingDescriptionCount = sizeof(vertex_binding_descriptions) / sizeof(vertex_binding_descriptions[0]);
            vertex_input_create_info.pVertexBindingDescriptions = vertex_binding_descriptions;
            vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/brdf.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/brdf.frag"), VK_SHADER_STAGE_FRAGMENT_BIT)
            };
            
            // Input assembly describes the topology of the geometry being rendered
            VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = create_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        
            // The viewport describes the region of the framebuffer that the output will be rendered to
            VkViewport viewport = create_viewport(0.0f, 0.0f, (float) swapchain_extent.width, (float) swapchain_extent.height, 0.0f, 1.0f);
        
            // The scissor region defines the portion of the viewport that will be drawn to the screen
            VkRect2D scissor = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
            
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
        
            // Depth/stencil buffers
            VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info { };
            depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable = VK_TRUE;
            depth_stencil_create_info.depthWriteEnable = VK_TRUE;
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
            
            // Additive blending
            VkPipelineColorBlendAttachmentState color_blend_attachment_create_info { };
            color_blend_attachment_create_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment_create_info.blendEnable = VK_FALSE;
            color_blend_attachment_create_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_blend_attachment_create_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
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
            
            VkDescriptorSetLayout layouts[2] = { global_descriptor_set_layout, object_descriptor_set_layout };
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
            pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
            pipeline_create_info.pViewportState = &viewport_create_info;
            pipeline_create_info.pRasterizationState = &rasterizer_create_info;
            pipeline_create_info.pMultisampleState = &multisampling_create_info;
            pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
            pipeline_create_info.pColorBlendState = &color_blend_create_info;
            pipeline_create_info.pDynamicState = nullptr;
            pipeline_create_info.layout = pipeline_layout;
            pipeline_create_info.renderPass = render_pass;
            pipeline_create_info.subpass = 0;
        
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void destroy_pipelines() {
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        
        void initialize_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                // Color attachment
                create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
                // Depth buffer
                create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
            };
            
            // Describe what layout the attachments should be in the subpass
            VkAttachmentReference color_attachment_reference = create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkAttachmentReference depth_stencil_attachment_reference = create_attachment_reference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pDepthStencilAttachment = &depth_stencil_attachment_reference;
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Dependency for ensuring that the color attachment (retrieved from the swapchain) is not transitioned to the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout before it is available
                // Color attachments are guaranteed to be available at the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage, as that is where the color attachment LOAD operation happens
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Since we are only using one depth attachment, ensure that both late fragment tests from the previous frame (STORE) and early fragment tests from the current frame (LOAD) are complete before attempting to overwrite the depth buffer
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                          0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT), // Depth buffer is written to during rendering and read from for late fragment tests
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = 2;
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void destroy_render_passes() {
            vkDestroyRenderPass(device, render_pass, nullptr);
        }
        
        void initialize_framebuffers() {
            present_framebuffers.resize(NUM_FRAMES_IN_FLIGHT);
            
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkImageView attachments[] = { swapchain_image_views[i], depth_buffer_view };
                
                VkFramebufferCreateInfo framebuffer_create_info { };
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = render_pass;
                framebuffer_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
                framebuffer_create_info.pAttachments = attachments;
                framebuffer_create_info.width = swapchain_extent.width;
                framebuffer_create_info.height = swapchain_extent.height;
                framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
                
                if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &present_framebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create present framebuffer!");
                }
            }
        }
        
        // Framebuffers are released automatically
        
        void initialize_global_descriptor_set() {
            // Descriptor set 0 is allocated for global uniforms that do not change between pipelines
            VkDescriptorSetLayoutBinding bindings[] {
                // Global camera information
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
                
//                // Positions
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
//
//                // Normals
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
//
//                // Ambient
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
//
//                // Diffuse
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
//
//                // Specular
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
//
//                // Shadow map
//                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7),
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &global_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create global descriptor set layout!");
            }
            
            // Initialize descriptor sets
            VkDescriptorSetAllocateInfo set_create_info { };
            set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_create_info.descriptorPool = descriptor_pool;
            set_create_info.descriptorSetCount = 1;
            set_create_info.pSetLayouts = &global_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_create_info, &global_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            
            VkWriteDescriptorSet descriptor_write { };
            VkDescriptorBufferInfo buffer_info { };
            
            // Binding 0
            buffer_info.buffer = uniform_buffer;
            buffer_info.offset = 0u;
            buffer_info.range = sizeof(GlobalUniforms);
            
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = global_descriptor_set;
            descriptor_write.dstBinding = 0;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;
            
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
        }
        
        void initialize_object_descriptor_sets() {
            // Allocate descriptor set 1 for per-object bindings
            VkDescriptorSetLayoutBinding bindings[] {
                // Object transforms (model, normal)
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
            };
            
            // Initialize the descriptor set layout
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &object_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            std::size_t offset = align_to_device_boundary(physical_device, sizeof(GlobalUniforms));
            
            std::size_t num_objects = transforms.size();
            object_descriptor_sets.resize(num_objects);
            
            // Initialize descriptor sets
            for (std::size_t i = 0u; i < num_objects; ++i) {
                VkDescriptorSetAllocateInfo set_create_info { };
                set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                set_create_info.descriptorPool = descriptor_pool;
                set_create_info.descriptorSetCount = 1;
                set_create_info.pSetLayouts = &object_descriptor_set_layout;
                if (vkAllocateDescriptorSets(device, &set_create_info, &object_descriptor_sets[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor set!");
                }
                
                VkWriteDescriptorSet descriptor_write { };
                VkDescriptorBufferInfo buffer_info { };
                
                buffer_info.buffer = uniform_buffer;
                buffer_info.offset = offset;
                buffer_info.range = sizeof(ObjectUniforms);
                
                descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_write.dstSet = object_descriptor_sets[i];
                descriptor_write.dstBinding = 0;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.descriptorCount = 1;
                descriptor_write.pBufferInfo = &buffer_info;
                
                offset += align_to_device_boundary(physical_device, sizeof(ObjectUniforms));
                
                vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
            }
        }

        void destroy_descriptor_sets() {
            // Destroy descriptor set layouts
            vkDestroyDescriptorSetLayout(device, global_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, object_descriptor_set_layout, nullptr);
            
            // Descriptor sets get cleaned up alongside the descriptor pool
        }
        
        void initialize_buffers() {
            model = load_obj("assets/models/sphere.obj");
            std::size_t vertex_buffer_size = model.vertices.size() * sizeof(Model::Vertex);
            std::size_t index_buffer_size = model.indices.size() * sizeof(unsigned);
            
            VkBuffer staging_buffer { };
            VkDeviceMemory staging_buffer_memory { };
            VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkMemoryPropertyFlags staging_buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            
            create_buffer(physical_device, device, vertex_buffer_size + index_buffer_size, staging_buffer_usage, staging_buffer_memory_properties, staging_buffer, staging_buffer_memory);
            
            // Upload vertex and index data into staging buffer
            std::size_t offset = 0u;
            
            void* data;
            vkMapMemory(device, staging_buffer_memory, offset, vertex_buffer_size + index_buffer_size, 0, &data);
                memcpy((void*)((char*)data + offset), model.vertices.data(), vertex_buffer_size);
                offset += vertex_buffer_size;
                memcpy((void*)((char*)data + offset), model.indices.data(), index_buffer_size);
                offset += index_buffer_size;
            vkUnmapMemory(device, staging_buffer_memory);
            
            // Create device-local buffers
            VkBufferUsageFlags vertex_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags vertex_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, vertex_buffer_size, vertex_buffer_usage, vertex_buffer_memory_properties, vertex_buffer, vertex_buffer_memory);
            
            VkBufferUsageFlags index_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            VkMemoryPropertyFlags index_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, index_buffer_size, index_buffer_usage, index_buffer_memory_properties, index_buffer, index_buffer_memory);
            
            // Copy over vertex / index buffers from staging buffer using a transient (one-time) command buffer
            VkCommandBuffer command_buffer = begin_transient_command_buffer();
                copy_buffer(command_buffer, staging_buffer, 0, vertex_buffer, 0, vertex_buffer_size);
                copy_buffer(command_buffer, staging_buffer, vertex_buffer_size, index_buffer, 0, index_buffer_size);
            submit_transient_command_buffer(command_buffer);
            
            // Staging buffer resources are no longer necessary
            vkFreeMemory(device, staging_buffer_memory, nullptr);
            vkDestroyBuffer(device, staging_buffer, nullptr);
        }
        
        void destroy_buffers() {
            vkDestroyBuffer(device, index_buffer, nullptr);
            vkFreeMemory(device, index_buffer_memory, nullptr);

            vkDestroyBuffer(device, vertex_buffer, nullptr);
            vkFreeMemory(device, vertex_buffer_memory, nullptr);
        }
        
        void initialize_transforms() {
            glm::ivec2 dimension = glm::vec2(5, 3);
            float scale = 1.0f;
            float spacing = 2.5f;
            transforms.resize(dimension.x * dimension.y);

            for (int y = 0; y < dimension.y; ++y) {
                for (int x = 0; x < dimension.x; ++x) {
                    Transform& transform = transforms[y * dimension.x + x];
                    transform.set_position(glm::vec3(((float)x - (float)dimension.x / 2.0f + scale / 2.0f) * spacing, ((float)y - (float)dimension.y / 2.0f + scale / 2.0f) * spacing, 0.0f));
                    transform.set_scale(glm::vec3(scale));
                }
            }
        }
        
//
//        void initialize_samplers() {
//            VkSamplerCreateInfo color_sampler_create_info { };
//            color_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//            color_sampler_create_info.magFilter = VK_FILTER_LINEAR;
//            color_sampler_create_info.minFilter = VK_FILTER_LINEAR;
//            color_sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//            color_sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//            color_sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//
//            if (enabled_physical_device_features.samplerAnisotropy) {
//                color_sampler_create_info.anisotropyEnable = VK_TRUE;
//                color_sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
//            }
//            else {
//                color_sampler_create_info.anisotropyEnable = VK_FALSE;
//                color_sampler_create_info.maxAnisotropy = 1.0f;
//            }
//
//            color_sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
//            color_sampler_create_info.unnormalizedCoordinates = VK_FALSE;
//            color_sampler_create_info.compareEnable = VK_FALSE;
//            color_sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
//            color_sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//            color_sampler_create_info.mipLodBias = 0.0f;
//            color_sampler_create_info.minLod = 0.0f;
//            color_sampler_create_info.maxLod = 1.0f;
//
//            if (vkCreateSampler(device, &color_sampler_create_info, nullptr, &color_sampler) != VK_SUCCESS) {
//                throw std::runtime_error("failed to create color sampler!");
//            }
//
//            VkSamplerCreateInfo depth_sampler_create_info { };
//            depth_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//            depth_sampler_create_info.magFilter = VK_FILTER_LINEAR;
//            depth_sampler_create_info.minFilter = VK_FILTER_LINEAR;
//            depth_sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//            depth_sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//            depth_sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//
//            if (enabled_physical_device_features.samplerAnisotropy) {
//                depth_sampler_create_info.anisotropyEnable = VK_TRUE;
//                depth_sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
//            }
//            else {
//                depth_sampler_create_info.anisotropyEnable = VK_FALSE;
//                depth_sampler_create_info.maxAnisotropy = 1.0f;
//            }
//
//            depth_sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
//            depth_sampler_create_info.unnormalizedCoordinates = VK_FALSE;
//            depth_sampler_create_info.compareEnable = VK_FALSE;
//            depth_sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
//            depth_sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//            depth_sampler_create_info.mipLodBias = 0.0f;
//            depth_sampler_create_info.minLod = 0.0f;
//            depth_sampler_create_info.maxLod = 1.0f;
//
//            if (vkCreateSampler(device, &depth_sampler_create_info, nullptr, &depth_sampler) != VK_SUCCESS) {
//                throw std::runtime_error("failed to create depth sampler!");
//            }
//        }
//
//        void destroy_samplers() {
//            vkDestroySampler(device, color_sampler, nullptr);
//            vkDestroySampler(device, depth_sampler, nullptr);
//        }
        
        void initialize_uniform_buffer() {
            std::size_t uniform_buffer_size = align_to_device_boundary(physical_device, sizeof(GlobalUniforms)) + (align_to_device_boundary(physical_device, sizeof(ObjectUniforms))) * transforms.size();
            create_buffer(physical_device, device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
            vkMapMemory(device, uniform_buffer_memory, 0, uniform_buffer_size, 0, &uniform_buffer_mapped);
        }
        
        void update_uniform_buffers() {
            std::size_t offset = 0u;
            
            // set 0 binding 0 (global uniforms)
            {
                GlobalUniforms uniforms { };
                uniforms.view = camera.get_view_matrix();
                uniforms.projection = camera.get_projection_matrix();
                uniforms.camera_position = camera.get_position();
                uniforms.debug_view = 0;
                
                memcpy((void*)(((char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(GlobalUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(GlobalUniforms));
            }
            
            std::size_t per_object_offset = align_to_device_boundary(physical_device, sizeof(ObjectUniforms));
            
            for (Transform& transform : transforms) {
                // set 1 binding 0 (per-object uniforms)
                ObjectUniforms uniforms { };
                uniforms.model = transform.get_matrix();
                uniforms.normal = glm::transpose(glm::inverse(uniforms.model));
                
                memcpy((void*)(((char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(ObjectUniforms));
                offset += per_object_offset;
            }
        }
        
        void destroy_uniform_buffer() {
            vkFreeMemory(device, uniform_buffer_memory, nullptr);
            vkDestroyBuffer(device, uniform_buffer, nullptr);
        }
        
        void on_key_pressed(int key) override {
            if (key == GLFW_KEY_F) {
                take_screenshot(swapchain_images[frame_index], surface_format.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, "pbr.ppm"); // Output attachment
            }
        }
        
};

DEFINE_SAMPLE_MAIN(PBR);
