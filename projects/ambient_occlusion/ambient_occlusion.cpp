
#include "sample.hpp"
#include "helpers.hpp"
#include "vulkan_initializers.hpp"
#include "loaders/obj.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <random>

// Ambient occlusion consists of doing 4 passes:
// 1. Geometry buffer pass
// 2. Generate ambient occlusion texture
// 3. Blur ambient occlusion texture
// 4. Composite ambient occlusion and geometry buffer attachments

class AmbientOcclusion final : public Sample {
    public:
        AmbientOcclusion() : Sample("Ambient Occlusion"),
                             ambient_occlusion_noise({}) {
            enabled_queue_types = VK_QUEUE_TRANSFER_BIT;
            debug_view = AO;
            
            camera.set_position(glm::vec3(0, 0.5f, 6));
            camera.set_look_direction(glm::vec3(0.0f, 0.25f, -1.0f));
        }
        
        ~AmbientOcclusion() override {
        }
        
    private:
        // Scene information
        std::vector<Model> models;
        std::vector<Transform> transforms;
        
        struct Scene {
            struct Object {
                unsigned model;
                unsigned vertex_offset;
                unsigned index_offset;
                Transform transform;
                
                glm::vec3 ambient;
                glm::vec3 diffuse;
                glm::vec3 specular;
                float specular_exponent;
                
                bool flat_shaded;
            };

            std::vector<Object> objects;
        } scene;
        
        int OUTPUT = -1;
        int AO = 0;
        int debug_view;
        
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        
        constexpr static const int KERNEL_SIZE = 36;
        constexpr static const float SAMPLE_RADIUS = 0.5f;
        
        glm::vec4 samples[KERNEL_SIZE];
        
        struct Texture {
            VkImage image { };
            VkDeviceMemory memory { };
            VkImageView image_view { };
        };
        
        // Geometry buffer
        // Position, normal, ambient, diffuse, specular, depth
        std::array<Texture, 6> geometry_framebuffer_attachments;
        
        VkFramebuffer geometry_framebuffer;
        VkPipelineLayout geometry_pipeline_layout;
        
        VkPipeline geometry_pipeline;
        VkRenderPass geometry_render_pass;
        
        VkDescriptorSetLayout geometry_global_descriptor_set_layout;
        VkDescriptorSet geometry_global_descriptor_set;
        struct GeometryGlobalUniforms {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 camera_position; // Padded vec3
        };
        
        VkDescriptorSetLayout geometry_object_descriptor_set_layout;
        std::vector<VkDescriptorSet> geometry_object_descriptor_sets; // One per scene object
        struct GeometryObjectVertexStageUniforms {
            glm::mat4 model;
            glm::mat4 normal;
        };
        struct GeometryObjectFragmentStageUniforms {
            glm::vec4 ambient; // Padded vec3
            glm::vec4 diffuse; // Padded vec3
            glm::vec3 specular; // Padded vec3
            float exponent;
            int flat_shaded;
        };
        
        // Ambient occlusion
        // Ambient occlusion, ambient occlusion blurred
        Texture ambient_occlusion_attachment;
        VkFramebuffer ambient_occlusion_framebuffer;
        
        Texture ambient_occlusion_blur_attachment;
        VkFramebuffer ambient_occlusion_blur_framebuffer;
        
        VkPipelineLayout ambient_occlusion_pipeline_layout;
        VkPipeline ambient_occlusion_pipeline;
        VkRenderPass ambient_occlusion_render_pass;
        
        VkDescriptorSetLayout ambient_occlusion_descriptor_set_layout;
        VkDescriptorSet ambient_occlusion_descriptor_set;
        struct AmbientOcclusionUniforms {
            glm::mat4 projection;
            glm::vec4 samples[KERNEL_SIZE];
        };
        
        VkPipelineLayout ambient_occlusion_blur_pipeline_layout;
        VkPipeline ambient_occlusion_blur_pipeline;
        VkRenderPass ambient_occlusion_blur_render_pass;
        
        VkDescriptorSetLayout ambient_occlusion_blur_descriptor_set_layout;
        VkDescriptorSet ambient_occlusion_blur_descriptor_set;
        
        Texture ambient_occlusion_noise;
        
        // Composition
        VkRenderPass composition_render_pass;
        
        VkPipelineLayout composition_pipeline_layout;
        VkPipeline composition_pipeline;
        
        VkDescriptorSetLayout composition_descriptor_set_layout;
        VkDescriptorSet composition_descriptor_set;
        struct CompositionUniforms {
            glm::mat4 view;
            glm::vec3 camera_position;
            int debug_view;
        };
        
        // Uniform buffers
        VkBuffer uniform_buffer; // One uniform buffer for all uniforms, across both passes
        VkDeviceMemory uniform_buffer_memory;
        void* uniform_buffer_mapped;
        
        VkSampler sampler; // Shared color sampler
        
        void initialize_resources() override {
            initialize_samplers();
            initialize_ambient_occlusion_resources();

            initialize_geometry_render_pass();
            initialize_ambient_occlusion_render_pass();
            initialize_ambient_occlusion_blur_render_pass();
            initialize_composition_render_pass();

            initialize_geometry_framebuffer();
            initialize_ambient_occlusion_framebuffer();
            initialize_ambient_occlusion_blur_framebuffer();
            initialize_composition_framebuffers();

            initialize_descriptor_pool(1 + 2 * scene.objects.size() + 1 + 1, 12);

            initialize_buffers();

            initialize_uniform_buffer();

            // Initialize descriptor sets
            initialize_geometry_global_descriptor_set();
            initialize_geometry_per_object_descriptor_sets();
            initialize_ambient_occlusion_descriptor_set();
            initialize_ambient_occlusion_blur_descriptor_set();
            initialize_composition_descriptor_set();

            initialize_geometry_pipeline();
            initialize_ambient_occlusion_pipeline();
            initialize_ambient_occlusion_blur_pipeline();
            initialize_composition_pipeline();
        }
        
        void destroy_resources() override {
            destroy_pipelines();
            destroy_descriptor_set_layouts();
            destroy_uniform_buffer();
            destroy_buffers();
            destroy_framebuffers();
            destroy_render_passes();
            destroy_ambient_occlusion_resources();
            destroy_samplers();
        }
        
        void update() override {
            Scene::Object& object = scene.objects.back();
            Transform& transform = object.transform;
            transform.set_rotation(transform.get_rotation() + (float)dt * glm::vec3(0.0f, -10.0f, 0.0f));
            update_uniform_buffers();
        }
        
        void render() override {
            VkSemaphore is_image_available = is_presentation_complete[frame_index];

            unsigned image_index;
            VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<std::uint64_t>::max(), is_image_available, VK_NULL_HANDLE, &image_index);

            // Record command buffer for this frame
            record_command_buffers(image_index);

            VkSubmitInfo submit_info { };
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            
            VkSemaphore wait_semaphores[] = { is_image_available }; // Semaphore(s) to wait on before the command buffers can begin execution
            VkPipelineStageFlags wait_stage_flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // Note: wait_stage_flags and wait_semaphores have a 1:1 correlation, meaning it is possible to wait on and signal different semaphores at different pipeline stages
            
            // Waiting on the swapchain image to be ready (if not yet) when the pipeline is ready to perform writes to color attachments
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = wait_semaphores;
            submit_info.pWaitDstStageMask = wait_stage_flags;
            
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffers[frame_index]; // Execute recorded command buffer for ambient occlusion
        
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &is_rendering_complete[frame_index];
        
            // Submit
            if (vkQueueSubmit(queue, 1, &submit_info, is_frame_in_flight[frame_index]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit command buffer!");
            }
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
        
            // Generate geometry buffer
            {
                VkRenderPassBeginInfo render_pass_info { };
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = geometry_render_pass;
                render_pass_info.framebuffer = geometry_framebuffer;
                render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
                
                VkClearValue clear_values[6] { };
                clear_values[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Position
                clear_values[1].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Normal
                clear_values[2].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Ambient
                clear_values[3].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Diffuse
                clear_values[4].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Specular
                clear_values[5].depthStencil = { 1.0f, 0 }; // Depth
                render_pass_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
                render_pass_info.pClearValues = clear_values;
                
                vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                    // Bind graphics pipeline
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pipeline);
    
                    // Bind global descriptor set
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pipeline_layout, 0, 1, &geometry_global_descriptor_set, 0, nullptr);
    
                    for (std::size_t i = 0u; i < scene.objects.size(); ++i) {
                        const Scene::Object& object = scene.objects[i];
                        const Model& model = models[object.model];
    
                        // Bind vertex + index buffers buffer
                        VkDeviceSize offsets[] = { object.vertex_offset  };
                        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
                        vkCmdBindIndexBuffer(command_buffer, index_buffer, object.index_offset, VK_INDEX_TYPE_UINT32);
    
                        // Bind per-object descriptor set
                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pipeline_layout, 1, 1, &geometry_object_descriptor_sets[i], 0, nullptr);
    
                        vkCmdDrawIndexed(command_buffer, (unsigned) model.indices.size(), 1, 0, 0, 0);
                    }
                vkCmdEndRenderPass(command_buffer);
            }
            
            // Ambient occlusion pass
            {
                VkRenderPassBeginInfo render_pass_info { };
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = ambient_occlusion_render_pass;
                render_pass_info.framebuffer = ambient_occlusion_framebuffer;
                render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
                
                // Ambient occlusion pass writes to one color attachment, no depth
                VkClearValue clear_value { };
                clear_value.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
                render_pass_info.clearValueCount = 1;
                render_pass_info.pClearValues = &clear_value;
                
                vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                    // Bind graphics pipeline
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ambient_occlusion_pipeline);
    
                    // Bind global descriptor set
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ambient_occlusion_pipeline_layout, 0, 1, &ambient_occlusion_descriptor_set, 0, nullptr);
                    
                    // Draw FSQ
                    vkCmdDraw(command_buffer, 3, 1, 0, 0);
                vkCmdEndRenderPass(command_buffer);
            }
            
            // Ambient occlusion blur pass
            {
                VkRenderPassBeginInfo render_pass_info { };
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = ambient_occlusion_blur_render_pass;
                render_pass_info.framebuffer = ambient_occlusion_blur_framebuffer;
                render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);

                // Ambient occlusion blur pass writes to one color attachment, no depth
                VkClearValue clear_value { };
                clear_value.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
                render_pass_info.clearValueCount = 1;
                render_pass_info.pClearValues = &clear_value;

                vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                    // Bind graphics pipeline
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ambient_occlusion_blur_pipeline);

                    // Bind global descriptor set
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ambient_occlusion_blur_pipeline_layout, 0, 1, &ambient_occlusion_blur_descriptor_set, 0, nullptr);

                    // Draw FSQ
                    vkCmdDraw(command_buffer, 3, 1, 0, 0);
                vkCmdEndRenderPass(command_buffer);
            }
            
            // Composition pass
            {
                VkRenderPassBeginInfo render_pass_info { };
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = composition_render_pass;
                render_pass_info.framebuffer = present_framebuffers[image_index];
                render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
                
                // Composition pipeline writes to one color attachment, no depth
                VkClearValue clear_value { };
                clear_value.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
                render_pass_info.clearValueCount = 1;
                render_pass_info.pClearValues = &clear_value;
                
                vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                    // Bind graphics pipeline
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pipeline);
    
                    // Bind global descriptor set
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pipeline_layout, 0, 1, &composition_descriptor_set, 0, nullptr);
                    
                    // Draw FSQ
                    vkCmdDraw(command_buffer, 3, 1, 0, 0);
                vkCmdEndRenderPass(command_buffer);
            }
            
            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
        
        void initialize_geometry_pipeline() {
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
            
            // Needs one color blend attachment per color attachment, otherwise colorMask will be set to 0 and the attachment will not receive any color output
            VkPipelineColorBlendAttachmentState color_blend_attachment_states[] {
                create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false), // Position
                create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false), // Normal
                create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false), // Ambient
                create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false), // Diffuse
                create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false), // Specular
            };
        
            VkPipelineColorBlendStateCreateInfo color_blend_create_info { };
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.logicOpEnable = VK_FALSE; // Use a bitwise operation to combine the old and new color values (setting this to true disables color mixing (specified above) for all framebuffers)
            color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_create_info.attachmentCount = sizeof(color_blend_attachment_states) / sizeof(color_blend_attachment_states[0]);
            color_blend_create_info.pAttachments = color_blend_attachment_states;
            color_blend_create_info.blendConstants[0] = 0.0f;
            color_blend_create_info.blendConstants[1] = 0.0f;
            color_blend_create_info.blendConstants[2] = 0.0f;
            color_blend_create_info.blendConstants[3] = 0.0f;
            
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            
            VkDescriptorSetLayout layouts[2] = { geometry_global_descriptor_set_layout, geometry_object_descriptor_set_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &geometry_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/geometry_buffer.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/geometry_buffer.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
            };
            
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
            pipeline_create_info.layout = geometry_pipeline_layout;
            pipeline_create_info.renderPass = geometry_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &geometry_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_ambient_occlusion_pipeline() {
            // Vertex data is generated directly in the vertex shader, no vertex input state to specify
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            
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
            depth_stencil_create_info.depthTestEnable = VK_FALSE;
            depth_stencil_create_info.depthWriteEnable = VK_FALSE;
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS; // Fragments that are closer have a lower depth value and should be kept (fragments further away are discarded)
            
            // Needs one color blend attachment, otherwise colorMask will be set to 0 and the attachment will not receive any color output
            VkPipelineColorBlendAttachmentState color_blend_attachment_state = create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false);
        
            VkPipelineColorBlendStateCreateInfo color_blend_create_info { };
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.logicOpEnable = VK_FALSE; // Use a bitwise operation to combine the old and new color values (setting this to true disables color mixing (specified above) for all framebuffers)
            color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_create_info.attachmentCount = 1;
            color_blend_create_info.pAttachments = &color_blend_attachment_state;
            color_blend_create_info.blendConstants[0] = 0.0f;
            color_blend_create_info.blendConstants[1] = 0.0f;
            color_blend_create_info.blendConstants[2] = 0.0f;
            color_blend_create_info.blendConstants[3] = 0.0f;
            
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts = &ambient_occlusion_descriptor_set_layout;
            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &ambient_occlusion_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
            
            // Fragment shader constants
            VkSpecializationMapEntry specializations[2] { };
            
            // layout (constant_id = 0) int KERNEL_SIZE;
            specializations[0].constantID = 0;
            specializations[0].size = sizeof(int);
            specializations[0].offset = 0;
            
            // layout (constant_id = 1) float SAMPLE_RADIUS;
            specializations[1].constantID = 1;
            specializations[1].size = sizeof(float);
            specializations[1].offset = sizeof(int);
            
            struct SpecializationData {
                int kernel_size;
                float sample_radius;
            };
            SpecializationData data { };
            data.kernel_size = KERNEL_SIZE;
            data.sample_radius = SAMPLE_RADIUS;
            
            VkSpecializationInfo specialization_info { };
            specialization_info.mapEntryCount = 2;
            specialization_info.pMapEntries = specializations;
            specialization_info.dataSize = sizeof(SpecializationData);
            specialization_info.pData = &data;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/fullscreen.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/ambient_occlusion.frag"), VK_SHADER_STAGE_FRAGMENT_BIT, &specialization_info),
            };
            
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
            pipeline_create_info.layout = ambient_occlusion_pipeline_layout;
            pipeline_create_info.renderPass = ambient_occlusion_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ambient_occlusion_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_ambient_occlusion_blur_pipeline() {
            // Vertex data is generated directly in the vertex shader, no vertex input state to specify
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            
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
            depth_stencil_create_info.depthTestEnable = VK_FALSE;
            depth_stencil_create_info.depthWriteEnable = VK_FALSE;
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS; // Fragments that are closer have a lower depth value and should be kept (fragments further away are discarded)
            
            // Needs one color blend attachment, otherwise colorMask will be set to 0 and the attachment will not receive any color output
            VkPipelineColorBlendAttachmentState color_blend_attachment_state = create_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false);
        
            VkPipelineColorBlendStateCreateInfo color_blend_create_info { };
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.logicOpEnable = VK_FALSE; // Use a bitwise operation to combine the old and new color values (setting this to true disables color mixing (specified above) for all framebuffers)
            color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_create_info.attachmentCount = 1;
            color_blend_create_info.pAttachments = &color_blend_attachment_state;
            color_blend_create_info.blendConstants[0] = 0.0f;
            color_blend_create_info.blendConstants[1] = 0.0f;
            color_blend_create_info.blendConstants[2] = 0.0f;
            color_blend_create_info.blendConstants[3] = 0.0f;
            
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts = &ambient_occlusion_blur_descriptor_set_layout;
            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &ambient_occlusion_blur_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/fullscreen.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/blur.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
            };
            
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
            pipeline_create_info.layout = ambient_occlusion_blur_pipeline_layout;
            pipeline_create_info.renderPass = ambient_occlusion_blur_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ambient_occlusion_blur_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_composition_pipeline() {
            // Vertex data is generated directly in the vertex shader, no vertex input state to specify
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/composition.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/composition.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
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
            depth_stencil_create_info.depthTestEnable = VK_FALSE;
            depth_stencil_create_info.depthWriteEnable = VK_FALSE;
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
            
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
            
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts = &composition_descriptor_set_layout;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &composition_pipeline_layout) != VK_SUCCESS) {
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
            pipeline_create_info.layout = composition_pipeline_layout;
            pipeline_create_info.renderPass = composition_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &composition_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create composition pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void destroy_pipelines() {
            // Composition pipeline
            vkDestroyPipelineLayout(device, composition_pipeline_layout, nullptr);
            vkDestroyPipeline(device, composition_pipeline, nullptr);
            
            // Ambient occlusion blur pipeline
            vkDestroyPipelineLayout(device, ambient_occlusion_blur_pipeline_layout, nullptr);
            vkDestroyPipeline(device, ambient_occlusion_blur_pipeline, nullptr);
            
            // Ambient occlusion pipeline
            vkDestroyPipelineLayout(device, ambient_occlusion_pipeline_layout, nullptr);
            vkDestroyPipeline(device, ambient_occlusion_pipeline, nullptr);
            
            // Geometry pipeline
            vkDestroyPipelineLayout(device, geometry_pipeline_layout, nullptr);
            vkDestroyPipeline(device, geometry_pipeline, nullptr);
        }
        
        void initialize_geometry_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                // Vertex positions
                create_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                
                // Vertex normals
                create_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                
                // Ambient
                create_attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                
                // Diffuse
                create_attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                
                // Specular
                create_attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                
                // Depth
                create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL),
            };
            
            VkAttachmentReference color_attachment_references[] {
                create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), // Position
                create_attachment_reference(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), // Normals
                create_attachment_reference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), // Ambient
                create_attachment_reference(3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), // Diffuse
                create_attachment_reference(4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), // Specular
            };
            VkAttachmentReference depth_stencil_attachment_reference = create_attachment_reference(5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL); // Depth
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = sizeof(color_attachment_references) / sizeof(color_attachment_references[0]);
            subpass_description.pColorAttachments = color_attachment_references;
            subpass_description.pDepthStencilAttachment = &depth_stencil_attachment_reference;
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Ensure that color attachment read operations during the composition stage of the previous frame complete before resetting them for the new render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill color attachments should complete before the ambient occlusion stage reads from them
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &geometry_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void initialize_ambient_occlusion_render_pass() {
            // Ambient occlusion output attachment only has one channel to store the occlusion factor [0.0, 1.0]
            VkAttachmentDescription attachment_description = create_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            VkAttachmentReference color_attachment_reference = create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pDepthStencilAttachment = nullptr; // Not used for this pass
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Ensure that the previous subpass (geometry) finishes entirely before beginning to write the ambient occlusion render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill the color attachment should complete before the ambient occlusion blur render pass reads from it
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = 1;
            render_pass_create_info.pAttachments = &attachment_description;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &ambient_occlusion_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void initialize_ambient_occlusion_blur_render_pass() {
            // Ambient occlusion blur output attachment only has one channel to store the occlusion factor [0.0, 1.0]
            VkAttachmentDescription attachment_description = create_attachment_description(VK_FORMAT_R8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            VkAttachmentReference color_attachment_reference = create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pDepthStencilAttachment = nullptr; // Not used for this pass
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Ensure that the previous subpass (ambient occlusion) finishes entirely
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill the color attachment should complete before the composition render pass reads from it
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = 1;
            render_pass_create_info.pAttachments = &attachment_description;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &ambient_occlusion_blur_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void initialize_composition_render_pass() {
            VkAttachmentDescription attachment_description = create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            VkAttachmentReference color_attachment_reference = create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pDepthStencilAttachment = nullptr; // Depth information is not used for rendering a FSQ
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Dependency for ensuring that the color attachment (retrieved from the swapchain) is not transitioned to the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout before it is available
                // Color attachments are guaranteed to be available at the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage, as that is where the color attachment LOAD operation happens
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = 1;
            render_pass_create_info.pAttachments = &attachment_description;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &composition_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void destroy_render_passes() {
            vkDestroyRenderPass(device, composition_render_pass, nullptr);
            vkDestroyRenderPass(device, ambient_occlusion_blur_render_pass, nullptr);
            vkDestroyRenderPass(device, ambient_occlusion_render_pass, nullptr);
            vkDestroyRenderPass(device, geometry_render_pass, nullptr);
        }
        
        void initialize_geometry_framebuffer() {
            // Color attachments
            for (std::size_t i = 0; i < 5; ++i) {
                VkFormat format;
                if (i == 0 || i == 1) {
                    format = VK_FORMAT_R16G16B16A16_SFLOAT; // 64-bit floating point image format (for higher precision) for position + normal textures
                }
                else {
                    // Color cannot be negative, so use an 32-bit unsigned image format for the rest
                    format = VK_FORMAT_R8G8B8A8_UNORM;
                }
                
                create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, 1, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, geometry_framebuffer_attachments[i].image, geometry_framebuffer_attachments[i].memory);
                create_image_view(device, geometry_framebuffer_attachments[i].image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, geometry_framebuffer_attachments[i].image_view);
            }
            
            // Depth attachment
            create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, 1, 1, VK_SAMPLE_COUNT_1_BIT, depth_buffer_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, geometry_framebuffer_attachments[5].image, geometry_framebuffer_attachments[5].memory);
            create_image_view(device, geometry_framebuffer_attachments[5].image, VK_IMAGE_VIEW_TYPE_2D, depth_buffer_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, geometry_framebuffer_attachments[5].image_view);
            
            VkImageView attachments[] {
                geometry_framebuffer_attachments[0].image_view, // Positions
                geometry_framebuffer_attachments[1].image_view, // Normals
                geometry_framebuffer_attachments[2].image_view, // Ambient
                geometry_framebuffer_attachments[3].image_view, // Diffuse
                geometry_framebuffer_attachments[4].image_view, // Specular
                geometry_framebuffer_attachments[5].image_view // Depth
            };
            
            VkFramebufferCreateInfo framebuffer_create_info { };
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = geometry_render_pass;
            framebuffer_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            framebuffer_create_info.pAttachments = attachments;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
            
            if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &geometry_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        void initialize_ambient_occlusion_framebuffer() {
            // Color attachments
            create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ambient_occlusion_attachment.image, ambient_occlusion_attachment.memory);
            create_image_view(device, ambient_occlusion_attachment.image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, ambient_occlusion_attachment.image_view);
            
            VkFramebufferCreateInfo framebuffer_create_info { };
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = ambient_occlusion_render_pass;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments = &ambient_occlusion_attachment.image_view;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
            
            if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &ambient_occlusion_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        void initialize_ambient_occlusion_blur_framebuffer() {
            // Color attachment
            create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ambient_occlusion_blur_attachment.image, ambient_occlusion_blur_attachment.memory);
            create_image_view(device, ambient_occlusion_blur_attachment.image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, ambient_occlusion_blur_attachment.image_view);
            
            VkFramebufferCreateInfo framebuffer_create_info { };
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = ambient_occlusion_blur_render_pass;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments = &ambient_occlusion_blur_attachment.image_view;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = 1; // Number of layers in the swapchain images
            
            if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &ambient_occlusion_blur_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        void initialize_composition_framebuffers() {
            present_framebuffers.resize(NUM_FRAMES_IN_FLIGHT);
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkFramebufferCreateInfo framebuffer_create_info { };
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = composition_render_pass;
                framebuffer_create_info.attachmentCount = 1;
                framebuffer_create_info.pAttachments = &swapchain_image_views[i];
                framebuffer_create_info.width = swapchain_extent.width;
                framebuffer_create_info.height = swapchain_extent.height;
                framebuffer_create_info.layers = 1;
                
                if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &present_framebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }
        
        void destroy_framebuffers() {
            // Ambient occlusion blur
            vkDestroyImage(device, ambient_occlusion_blur_attachment.image, nullptr);
            vkDestroyImageView(device, ambient_occlusion_blur_attachment.image_view, nullptr);
            vkFreeMemory(device, ambient_occlusion_blur_attachment.memory, nullptr);
            
            vkDestroyFramebuffer(device, ambient_occlusion_blur_framebuffer, nullptr);
            
            // Ambient occlusion
            vkDestroyImage(device, ambient_occlusion_attachment.image, nullptr);
            vkDestroyImageView(device, ambient_occlusion_attachment.image_view, nullptr);
            vkFreeMemory(device, ambient_occlusion_attachment.memory, nullptr);
            
            vkDestroyFramebuffer(device, ambient_occlusion_framebuffer, nullptr);
            
            // Geometry buffer
            for (std::size_t i = 0u; i < geometry_framebuffer_attachments.size(); ++i) {
                vkDestroyImage(device, geometry_framebuffer_attachments[i].image, nullptr);
                vkDestroyImageView(device, geometry_framebuffer_attachments[i].image_view, nullptr);
                vkFreeMemory(device, geometry_framebuffer_attachments[i].memory, nullptr);
            }
            vkDestroyFramebuffer(device, geometry_framebuffer, nullptr);
        }
        
        void initialize_geometry_global_descriptor_set() {
            // Initialize the global descriptor set (used across both vertex and fragment shader stages)
            // This descriptor set is mapped to set 0 and contains a uniform buffer at binding 0 with the following members:
            //   - mat4 (camera view)
            //   - mat4 (camera projection)
            //   - vec3 (camera position)
            
            // Initialize set layout
            unsigned binding_point = 0;
            VkDescriptorSetLayoutBinding binding = create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, binding_point);
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = 1;
            layout_create_info.pBindings = &binding;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &geometry_global_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // Initialize set
            VkDescriptorSetAllocateInfo set_create_info { };
            set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_create_info.descriptorPool = descriptor_pool;
            set_create_info.descriptorSetCount = 1;
            set_create_info.pSetLayouts = &geometry_global_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_create_info, &geometry_global_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            
            // Configure the offsets at which the data for the global uniforms for the geometry pass is located
            
            // This descriptor set is located at the very start of the uniform buffer
            std::size_t offset = 0u;
            
            VkDescriptorBufferInfo buffer_info { };
            buffer_info.buffer = uniform_buffer;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(GeometryGlobalUniforms);
            
            VkWriteDescriptorSet descriptor_write { };
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = geometry_global_descriptor_set;
            descriptor_write.dstBinding = binding_point;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;
            
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
        }
        
        void initialize_geometry_per_object_descriptor_sets() {
            // Initialize descriptor sets for each object
            // This descriptor set is mapped to set 1 and contains 2 uniform buffers at binding points 0 and 1 in the vertex and fragment shaders (respectively) with the following members:
            
            // Vertex uniform buffer (binding point 0):
            //   - mat4 (model matrix)
            //   - mat4 (normal matrix)
            
            // Fragment uniform buffer (binding point 1):
            //   - vec3 (ambient color)
            //   - vec3 (diffuse color)
            //   - vec3 (specular color)
            //   - float (specular exponent)
            
            // There needs to be a descriptor set allocated per object in the scene that points into the uniform buffer at the correct offset

            // Initialize set layout
            VkDescriptorSetLayoutBinding bindings[] {
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0), // Uniform buffer for the vertex stage at binding point 0
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1) // Uniform buffer for the fragment stage at bidning point 1
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &geometry_object_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // Initialize sets (one per scene object)
            geometry_object_descriptor_sets.resize(scene.objects.size());
            
            // In memory, the data for the vertex and fragment uniform buffers for a given object are consecutive
            // This data is located directly after the section containing the global uniform buffer for the geometry pass (initialized above)

            std::size_t globals_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryGlobalUniforms));
            
            // Configure per-object uniform ranges (each object requires its own descriptor set)
            std::size_t object_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryObjectVertexStageUniforms)) + align_to_device_boundary(physical_device, sizeof(GeometryObjectFragmentStageUniforms));
            for (std::size_t i = 0u; i < scene.objects.size(); ++i) {
                
                VkDescriptorSetAllocateInfo set_create_info { };
                set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                set_create_info.descriptorPool = descriptor_pool;
                set_create_info.descriptorSetCount = 1;
                set_create_info.pSetLayouts = &geometry_object_descriptor_set_layout;
                if (vkAllocateDescriptorSets(device, &set_create_info, &geometry_object_descriptor_sets[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor set!");
                }
                
                std::size_t offset = globals_uniform_block_size + i * object_uniform_block_size;
                
                VkDescriptorBufferInfo buffer_infos[2] { };
                VkWriteDescriptorSet descriptor_writes[2] { };
                
                // Configure ranges for vertex uniform block
                buffer_infos[0].buffer = uniform_buffer;
                buffer_infos[0].offset = offset;
                buffer_infos[0].range = sizeof(GeometryObjectVertexStageUniforms);
                
                descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[0].dstSet = geometry_object_descriptor_sets[i];
                descriptor_writes[0].dstBinding = 0; // Vertex uniform block is bound at binding point 0
                descriptor_writes[0].dstArrayElement = 0;
                descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[0].descriptorCount = 1;
                descriptor_writes[0].pBufferInfo = &buffer_infos[0];
                
                // Vertex and fragment uniform blocks are consecutive
                offset += align_to_device_boundary(physical_device, buffer_infos[0].range);
                
                // Configure ranges for fragment uniform block
                buffer_infos[1].buffer = uniform_buffer;
                buffer_infos[1].offset = offset;
                buffer_infos[1].range = sizeof(GeometryObjectFragmentStageUniforms);
                
                descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[1].dstSet = geometry_object_descriptor_sets[i];
                descriptor_writes[1].dstBinding = 1; // Fragment uniform block is bound at binding point 1
                descriptor_writes[1].dstArrayElement = 0;
                descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[1].descriptorCount = 1;
                descriptor_writes[1].pBufferInfo = &buffer_infos[1];
                
                // Specify the buffer and region within it that contains the data for the allocated descriptors
                vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, nullptr);
            }
        }
        
        void initialize_ambient_occlusion_descriptor_set() {
            // Initialize the global descriptor set used in the fragment shader for ambient occlusion calculations
            // This descriptor set is mapped to set 0 and contains a uniform buffer at binding 0 with the following members:
            //   - mat4 (camera projection)
            //   - array of vec4s (ambient occlusion kernel)
            
            // Initialize set layout
            VkDescriptorSetLayoutBinding bindings[] {
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0), // Positions
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1), // Normals
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2), // Depth
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3), // Noise
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &ambient_occlusion_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // Initialize set
            VkDescriptorSetAllocateInfo set_create_info { };
            set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_create_info.descriptorPool = descriptor_pool;
            set_create_info.descriptorSetCount = 1;
            set_create_info.pSetLayouts = &ambient_occlusion_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_create_info, &ambient_occlusion_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            
            VkWriteDescriptorSet descriptor_writes[5] { };
            VkDescriptorImageInfo image_infos[4] { };
            
            unsigned binding_point = 0;
            
            image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[binding_point].imageView = geometry_framebuffer_attachments[0].image_view; // Positions
            image_infos[binding_point].sampler = sampler;
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = ambient_occlusion_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            
            ++binding_point;
            
            image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[binding_point].imageView = geometry_framebuffer_attachments[1].image_view; // Normals
            image_infos[binding_point].sampler = sampler;
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = ambient_occlusion_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            
            ++binding_point;
            
            image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            image_infos[binding_point].imageView = geometry_framebuffer_attachments[5].image_view; // Depth
            image_infos[binding_point].sampler = sampler;
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = ambient_occlusion_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            
            ++binding_point;
            
            image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[binding_point].imageView = ambient_occlusion_noise.image_view; // Random noise
            image_infos[binding_point].sampler = sampler;
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = ambient_occlusion_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            
            ++binding_point;
            
            // Configure the offsets at which the data for the global uniforms for the geometry pass is located
            
            // This descriptor set is located after the per-model descriptor sets for the geometry pass
            std::size_t globals_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryGlobalUniforms));
            std::size_t object_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryObjectVertexStageUniforms)) + align_to_device_boundary(physical_device, sizeof(GeometryObjectFragmentStageUniforms));
            
            VkDescriptorBufferInfo buffer_info { };
            buffer_info.buffer = uniform_buffer;
            buffer_info.offset = globals_uniform_block_size + object_uniform_block_size * scene.objects.size();
            buffer_info.range = sizeof(AmbientOcclusionUniforms);
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = ambient_occlusion_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pBufferInfo = &buffer_info;
            
            vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
        }
        
        void initialize_ambient_occlusion_blur_descriptor_set() {
            // Initialize the global descriptor set used in the fragment shader for blurring
            
            // Initialize set layout
            VkDescriptorSetLayoutBinding binding = create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = 1;
            layout_create_info.pBindings = &binding;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &ambient_occlusion_blur_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // Initialize set
            VkDescriptorSetAllocateInfo set_create_info { };
            set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_create_info.descriptorPool = descriptor_pool;
            set_create_info.descriptorSetCount = 1;
            set_create_info.pSetLayouts = &ambient_occlusion_blur_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_create_info, &ambient_occlusion_blur_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            
            VkWriteDescriptorSet descriptor_write { };
            VkDescriptorImageInfo image_info { };
            
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = ambient_occlusion_attachment.image_view;
            image_info.sampler = sampler;
            
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = ambient_occlusion_blur_descriptor_set;
            descriptor_write.dstBinding = 0;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pImageInfo = &image_info;
            
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
        }
        
        void initialize_composition_descriptor_set() {
            // Initialize the global descriptor set used in the fragment shader for the composition of the final scene
            
            // Initialize set layout
            VkDescriptorSetLayoutBinding bindings[] {
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0), // Position
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1), // Normals
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2), // Ambient
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3), // Diffuse
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4), // Specular
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5), // Ambient occlusion
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 6)
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &composition_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            VkDescriptorSetAllocateInfo set_allocate_info { };
            set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_allocate_info.descriptorPool = descriptor_pool;
            set_allocate_info.descriptorSetCount = 1;
            set_allocate_info.pSetLayouts = &composition_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_allocate_info, &composition_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            
            VkWriteDescriptorSet descriptor_writes[7] { };
            VkDescriptorImageInfo image_infos[6] { };
            
            unsigned binding_point;
            
            // Descriptors 0 - 4 come from the geometry framebuffer (position, normal, ambient, diffuse, specular)
            for (binding_point = 0; binding_point < 5; ++binding_point) {
                image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_infos[binding_point].imageView = geometry_framebuffer_attachments[binding_point].image_view;
                image_infos[binding_point].sampler = sampler;
                
                descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[binding_point].dstSet = composition_descriptor_set;
                descriptor_writes[binding_point].dstBinding = binding_point;
                descriptor_writes[binding_point].dstArrayElement = 0;
                descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_writes[binding_point].descriptorCount = 1;
                descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            }
            
            // Descriptor 5 comes from the ambient occlusion framebuffer
            image_infos[binding_point].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[binding_point].imageView = ambient_occlusion_blur_attachment.image_view;
            image_infos[binding_point].sampler = sampler;
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = composition_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pImageInfo = &image_infos[binding_point];
            
            // Uniforms for the final composition pass are located at the very end of the uniform buffer
            
            std::size_t globals_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryGlobalUniforms));
            std::size_t object_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryObjectVertexStageUniforms)) + align_to_device_boundary(physical_device, sizeof(GeometryObjectFragmentStageUniforms));
            std::size_t ambient_occlusion_uniform_block_size = align_to_device_boundary(physical_device, sizeof(AmbientOcclusionUniforms));
            
            VkDescriptorBufferInfo buffer_info { };
            buffer_info.buffer = uniform_buffer;
            buffer_info.offset = globals_uniform_block_size + object_uniform_block_size * scene.objects.size() + ambient_occlusion_uniform_block_size;
            buffer_info.range = sizeof(CompositionUniforms);
            
            ++binding_point; // 6
            
            descriptor_writes[binding_point].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding_point].dstSet = composition_descriptor_set;
            descriptor_writes[binding_point].dstBinding = binding_point;
            descriptor_writes[binding_point].dstArrayElement = 0;
            descriptor_writes[binding_point].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding_point].descriptorCount = 1;
            descriptor_writes[binding_point].pBufferInfo = &buffer_info;
            
            // Specify the buffer and region within it that contains the data for the allocated descriptors
            vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
        }
        
        void destroy_descriptor_set_layouts() {
            vkDestroyDescriptorSetLayout(device, composition_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, ambient_occlusion_blur_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, ambient_occlusion_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, geometry_object_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, geometry_global_descriptor_set_layout, nullptr);
        }
        
        void initialize_buffers() {
            models.emplace_back(load_obj("assets/models/cube.obj"));
            models.emplace_back(load_obj("assets/models/knight.obj"));
            float box_size = 3.0f;
            float height = 2.0f;
            float thickness = 0.05f;
            
            // Walls
            Scene::Object& right = scene.objects.emplace_back();
            right.model = 0;
            right.ambient = glm::vec3(0.1f);
            right.diffuse = glm::vec3(205,92,92) / glm::vec3(255); // red
            right.specular = glm::vec3(0.0f);
            right.specular_exponent = 0.0f;
            right.transform = Transform(glm::vec3(box_size, height, 0), glm::vec3(thickness, box_size, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
            right.flat_shaded = true;
            
            Scene::Object& left = scene.objects.emplace_back();
            left.model = 0;
            left.ambient = glm::vec3(0.1f);
            left.diffuse = glm::vec3(46,139,87) / glm::vec3(255); // green
            left.specular = glm::vec3(0.0f);
            left.specular_exponent = 0.0f;
            left.transform = Transform(glm::vec3(-box_size, height, 0), glm::vec3(thickness, box_size, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
            left.flat_shaded = true;

            Scene::Object& back = scene.objects.emplace_back();
            back.model = 0;
            back.ambient = glm::vec3(0.1f);
            back.diffuse = glm::vec3(70,130,180) / glm::vec3(255); // blue
            back.specular = glm::vec3(0.0f);
            back.specular_exponent = 0.0f;
            back.transform = Transform(glm::vec3(0, height, -box_size), glm::vec3(box_size, box_size, thickness), glm::vec3(0.0f, 0.0f, 0.0f));
            back.flat_shaded = true;

            Scene::Object& ceiling = scene.objects.emplace_back();
            ceiling.model = 0;
            ceiling.ambient = glm::vec3(0.1f);
            ceiling.diffuse = glm::vec3(255,235,205) / glm::vec3(255); // offwhite
            ceiling.specular = glm::vec3(0.0f);
            ceiling.specular_exponent = 0.0f;
            ceiling.transform = Transform(glm::vec3(0, box_size + height, 0), glm::vec3(box_size, thickness, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
            ceiling.flat_shaded = true;

            Scene::Object& floor = scene.objects.emplace_back();
            floor.model = 0;
            floor.ambient = glm::vec3(0.1f);
            floor.diffuse = glm::vec3(255,235,205) / glm::vec3(255); // offwhite
            floor.specular = glm::vec3(0.0f);
            floor.specular_exponent = 0.0f;
            floor.transform = Transform(glm::vec3(0, -box_size + height, 0), glm::vec3(box_size, thickness, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
            floor.flat_shaded = true;
            
            // Center model
            Scene::Object& knight = scene.objects.emplace_back();
            knight.model = 1;
            knight.ambient = glm::vec3(0.3f);
            knight.diffuse = glm::vec3(0.8f); // offwhite
            knight.specular = glm::vec3(0.0f);
            knight.specular_exponent = 0.0f;
            knight.transform = Transform(glm::vec3(0, 0.5f, 0), glm::vec3(1.5f), glm::vec3(0.0f, -55.0f, 0.0f));
            knight.flat_shaded = true;
            
            // This sample only uses vertex position and normal
            unsigned vertex_buffer_size = 0u;
            unsigned index_buffer_size = 0u;
            
            for (std::size_t i = 0u; i < models.size(); ++i) {
                for (Scene::Object& object : scene.objects) {
                    if (object.model == i) {
                        object.vertex_offset = vertex_buffer_size;
                        object.index_offset = index_buffer_size;
                    }
                }
                
                vertex_buffer_size += models[i].vertices.size() * sizeof(Model::Vertex);
                index_buffer_size += models[i].indices.size() * sizeof(unsigned);
            }
            
            VkBuffer staging_buffer { };
            VkDeviceMemory staging_buffer_memory { };
            VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkMemoryPropertyFlags staging_buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            
            create_buffer(physical_device, device, vertex_buffer_size + index_buffer_size, staging_buffer_usage, staging_buffer_memory_properties, staging_buffer, staging_buffer_memory);
            
            // Upload vertex data into staging buffer
            {
                std::size_t offset = 0u;
                
                for (const Model& model : models) {
                    std::size_t model_vertices_size = model.vertices.size() * sizeof(Model::Vertex);
                    
                    void* data;
                    vkMapMemory(device, staging_buffer_memory, offset, model_vertices_size, 0, &data);
                        memcpy(data, model.vertices.data(), model_vertices_size);
                    vkUnmapMemory(device, staging_buffer_memory);
                    
                    offset += model_vertices_size;
                }
            }
            
            // Upload index data into staging buffer
            {
                std::size_t offset = 0u;
                
                for (const Model& model : models) {
                    std::size_t model_indices_size = model.indices.size() * sizeof(unsigned);
                    
                    void* data;
                    vkMapMemory(device, staging_buffer_memory, vertex_buffer_size + offset, model_indices_size, 0, &data);
                        memcpy(data, model.indices.data(), model_indices_size);
                    vkUnmapMemory(device, staging_buffer_memory);
                    
                    offset += model_indices_size;
                }
            }
            
            // Create device-local buffers
            VkBufferUsageFlags vertex_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags vertex_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, vertex_buffer_size, vertex_buffer_usage, vertex_buffer_memory_properties, vertex_buffer, vertex_buffer_memory);
            
            VkBufferUsageFlags index_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT; // This buffer is the destination buffer in a memory transfer operation (and also the index buffer).
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
        
        void initialize_samplers() {
            // Texture access in shaders is typically done through image samplers
            // Image samplers apply filtering and transformations to compute the final color retrieved from the image
            VkSamplerCreateInfo sampler_create_info { };
            sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            
            // Filters help deal with oversampling (when there is more geometry fragments than texels) and blend between texture colors
            //   - VK_FILTER_LINEAR - bilinear filtering (interpolates from texel neighbors)
            //   - VK_FILTER_NEAREST - no filtering, takes the nearest texel value
            sampler_create_info.magFilter = VK_FILTER_NEAREST;
            sampler_create_info.minFilter = VK_FILTER_NEAREST;
            
            // Samplers also take care of transformations (what happens when texels are read from outside the image)
            //   - VK_SAMPLER_ADDRESS_MODE_REPEAT - texture is repeated when sampled beyond the original image dimension
            //   - VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT - like VK_SAMPLER_ADDRESS_MODE_REPEAT, but sample coordinates are inverted to mirror the image across each of the principal axes
            //   - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE - takes the color of the edge closest to the coordinate beyond the image dimensions
            //   - VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE - like VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, but takes the edge opposite to the closest edge
            //   - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER - returns a solid color when sampled beyond the image dimensions
            sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // x
            sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // y
            sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // z (for 3D textures)
            
            // Anisotropic filtering helps deal with oversampling (where there are more texels than fragments) at very steep viewing angles, taking into consideration each texture's location on the screen relative to the camera angle
            if (enabled_physical_device_features.samplerAnisotropy) {
                sampler_create_info.anisotropyEnable = VK_TRUE;
                
                // Set to the maximum supported number of texel samples that should be used to calculate the final pixel color
                // A lower number yields higher performance but has a lower quality final image
                sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
            }
            else {
                // Selected physical device does not support anisotropic filtering
                sampler_create_info.anisotropyEnable = VK_FALSE;
                sampler_create_info.maxAnisotropy = 1.0f;
            }
        
            // Specify what color to return when sampling beyond image dimensions when using VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
            // Choose between black, white, or transparent (FLOAT/INT formats)
            sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            
            // Specify if sample coordinates should use the unnormalized [0, texture_width) to [0, texture_height) range instead of the normalized [0, 1) range on all axes
            sampler_create_info.unnormalizedCoordinates = VK_FALSE;
            
            sampler_create_info.compareEnable = VK_FALSE;
            sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
            
            // Pseudocode for selecting mip level during rendering:
            // lod = get_lod_from_screensize(); // Note: smaller when the object is closer, may be negative
            // lod = clamp(lod + mipLodBias, minLod, maxLod);
            //
            // level = clamp(floor(lod), 0, texture.mipLevels - 1); // Do not exceed the number of mip levels of the actual image
        
            // If VK_SAMPLER_MIPMAP_MODE_LINEAR is selected as mipmapMode, the value of lod is used to linearly blend between two of the nearest mip levels
            // if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
            //     color = sample(level);
            // } else {
            //     color = blend(sample(level), sample(level + 1));
            // }
            
            // Note that sampling from the image is also affected by the LOD.
            // if (lod <= 0) {
            //     color = readTexture(uv, magFilter);
            // } else {
            //     color = readTexture(uv, minFilter);
            // }
            sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_create_info.mipLodBias = 0.0f;
            sampler_create_info.minLod = 0.0f;
            sampler_create_info.maxLod = 1.0f;
            
            if (vkCreateSampler(device, &sampler_create_info, nullptr, &sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }
        
        void destroy_samplers() {
            vkDestroySampler(device, sampler, nullptr);
        }
        
        void initialize_uniform_buffer() {
            std::size_t geometry_global_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryGlobalUniforms));
            std::size_t geometry_object_uniform_block_size = align_to_device_boundary(physical_device, sizeof(GeometryObjectVertexStageUniforms)) + align_to_device_boundary(physical_device, sizeof(GeometryObjectFragmentStageUniforms));
            std::size_t ambient_occlusion_uniform_block_size = align_to_device_boundary(physical_device, sizeof(AmbientOcclusionUniforms));
            std::size_t composition_uniform_block_size = align_to_device_boundary(physical_device, sizeof(CompositionUniforms));
            
            std::size_t uniform_buffer_size = geometry_global_uniform_block_size + geometry_object_uniform_block_size * scene.objects.size() + ambient_occlusion_uniform_block_size + composition_uniform_block_size;

            create_buffer(physical_device, device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
            vkMapMemory(device, uniform_buffer_memory, 0, uniform_buffer_size, 0, &uniform_buffer_mapped);
        }
        
        void update_uniform_buffers() {
            // TODO: this data only needs to change when dirty
            std::size_t offset = 0u;
            
            // Geometry pass global uniforms
            {
                GeometryGlobalUniforms uniforms { };
                uniforms.camera_position = glm::vec4(camera.get_position(), 1.0f);
                uniforms.view = camera.get_view_matrix();
                uniforms.projection = camera.get_projection_matrix();
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(uniforms));
                offset += align_to_device_boundary(physical_device, sizeof(GeometryGlobalUniforms));
            }
            
            // Geometry pass per-object uniforms
            for (Scene::Object& object : scene.objects) {
                Transform& transform = object.transform;
                
                // Vertex
                GeometryObjectVertexStageUniforms vertex { };
                vertex.model = transform.get_matrix();
                vertex.normal = glm::transpose(glm::inverse(vertex.model));
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &vertex, sizeof(GeometryObjectVertexStageUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(GeometryObjectVertexStageUniforms));
                
                // Fragment
                GeometryObjectFragmentStageUniforms fragment { };
                fragment.ambient = glm::vec4(object.ambient, 1.0f);
                fragment.diffuse = glm::vec4(object.diffuse, 1.0f);
                fragment.specular = glm::vec4(object.specular, 1.0f);
                fragment.exponent = object.specular_exponent;
                fragment.flat_shaded = (int) object.flat_shaded;
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &fragment, sizeof(GeometryObjectFragmentStageUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(GeometryObjectFragmentStageUniforms));
            }
            
            // Ambient occlusion uniforms
            {
                AmbientOcclusionUniforms uniforms { };
                uniforms.projection = camera.get_projection_matrix();
                memcpy(&uniforms.samples, &samples, KERNEL_SIZE * sizeof(glm::vec4));
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(AmbientOcclusionUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(AmbientOcclusionUniforms));
            }
            
            // Composition uniforms
            {
                CompositionUniforms uniforms { };
                uniforms.view = camera.get_view_matrix();
                uniforms.camera_position = camera.get_position();
                uniforms.debug_view = debug_view;
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(CompositionUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(CompositionUniforms));
            }
        }
        
        void destroy_uniform_buffer() {
            vkFreeMemory(device, uniform_buffer_memory, nullptr);
            vkDestroyBuffer(device, uniform_buffer, nullptr);
        }
        
        float lerp(float a, float b, float f) {
            return a + f * (b - a);
        }
        
        void initialize_ambient_occlusion_resources() {
            // Initialize sample kernel
            std::uniform_real_distribution<float> distribution(0.0f, 1.0f); // Uniform distribution of floats in the range [0.0, 1.0]
            std::default_random_engine generator(time(nullptr));
            
            // Generate samples in tangent space, with the normal vector pointing at +z
            for (std::size_t i = 0u; i < KERNEL_SIZE; ++i) {
                // Generate x and y coordinates on the range [-1.0, 1.0], but z on the range [0.0, 1.0] to generates samples within a unit hemisphere oriented along the z axis
                glm::vec3 sample(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator));
                sample = glm::normalize(sample);
                
                // Uniformly randomize around the unit hemisphere
                sample *= distribution(generator);
                
                // Distance from the origin should fall off as more points are generated (more points should be closer to the origin of the hemisphere)
                float scale = (float) i / (float) KERNEL_SIZE;
                scale = lerp(0.1f, 1.0f, scale * scale);
                
                samples[i] = glm::vec4(sample * scale, 0.0f);
            }
            
            // Introducing slight variation (randomness) to the sample kernel for each fragment is a good way to reduce artifacting and the number of samples to get convincing ambient occlusion
            // One way to do this would be to generate a random rotation vector for each fragment of the scene, but this is costly to store (width * height * 4 channels of 32-bit floating point data)
            // A better approach would be to tile a smaller subset of of randomized rotation vectors across the image
            
            std::vector<glm::vec4> noise_values;
            unsigned dimension = 6;
            noise_values.resize(dimension * dimension); // 64 random rotation vectors
            
            for (std::size_t i = 0u; i < dimension * dimension; ++i) {
                // Rotation kernel (initialized above) is oriented along the z axis, so keep the z component of the random offset 0 to rotate around this axis
                noise_values[i] = glm::vec4(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, 0.0f, 0.0f);
            }
            
            // Generate texture for storing noise values
            unsigned mip_levels = 1;
            create_image(physical_device, device, dimension, dimension, mip_levels, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ambient_occlusion_noise.image, ambient_occlusion_noise.memory);

            // Use staging buffer to upload image into device local memory for optimal layout
            std::size_t image_size = dimension * dimension * sizeof(glm::vec4);
            
            VkBuffer staging_buffer { };
            VkDeviceMemory staging_buffer_memory { };
            create_buffer(physical_device, device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
            
            // Upload image data into staging buffer
            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
                memcpy(data, noise_values.data(), image_size);
            vkUnmapMemory(device, staging_buffer_memory);
            
            VkImageSubresourceRange subresource_range { };
            subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource_range.baseMipLevel = 0;
            subresource_range.levelCount = mip_levels;
            subresource_range.layerCount = 1;
            subresource_range.baseArrayLayer = 0; // Image is not an array
            
            VkImageSubresourceLayers subresource_layers { };
            subresource_layers.layerCount = 1;
            subresource_layers.baseArrayLayer = 0;
            subresource_layers.mipLevel = 0;
            subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            
            VkBufferImageCopy copy_region { };
            copy_region.imageSubresource = subresource_layers;
            copy_region.imageExtent = { dimension, dimension, 1 };
            copy_region.imageOffset = { 0, 0, 0 };
            
            // Applicable only if tiling is VK_IMAGE_TILING_LINEAR, which allows for direct memory access
            copy_region.bufferOffset = 0; // Pixels are tightly packed
            copy_region.bufferImageHeight = 0;
            copy_region.bufferRowLength = 0;
            
            // Copy over image data from staging buffer using a transient command buffer
            VkCommandBuffer command_buffer = begin_transient_command_buffer();
                // Transition the device-local image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                transition_image(command_buffer, ambient_occlusion_noise.image,
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range,
                                 // Image initial layout is undefined, no access flags are required
                                 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 // Ensure any writes to the image are done after transitioning the image layout
                                 VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                
                // Copy buffer to image
                vkCmdCopyBufferToImage(command_buffer, staging_buffer, ambient_occlusion_noise.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
            
                // Transition image layout to a layout for optimal reads from shaders (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) after all mip levels have been copied
                transition_image(command_buffer, ambient_occlusion_noise.image,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range,
                                 // Wait until the image is fully written to
                                 VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 // Ensure any reads from the image are done after transitioning the image layout
                                 VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            submit_transient_command_buffer(command_buffer);
            
            create_image_view(device, ambient_occlusion_noise.image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, ambient_occlusion_noise.image_view);
            
            // Staging buffer resources are no longer necessary
            vkFreeMemory(device, staging_buffer_memory, nullptr);
            vkDestroyBuffer(device, staging_buffer, nullptr);
        }
        
        void destroy_ambient_occlusion_resources() {
            vkFreeMemory(device, ambient_occlusion_noise.memory, nullptr);
            vkDestroyImageView(device, ambient_occlusion_noise.image_view, nullptr);
            vkDestroyImage(device, ambient_occlusion_noise.image, nullptr);
        }
        
        void on_key_pressed(int key) {
            if (key == GLFW_KEY_1) {
                debug_view = AO;
            }
            else if (key == GLFW_KEY_2) {
                debug_view = OUTPUT;
            }
            else if (key == GLFW_KEY_F) {
                take_screenshot(swapchain_images[frame_index], surface_format.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, "ambient_occlusion.ppm"); // Output attachment
            }
        }
        
};

DEFINE_SAMPLE_MAIN(AmbientOcclusion);
