

#include "sample.hpp"
#include "helpers.hpp"
#include "vulkan_initializers.hpp"
#include "loaders/obj.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

class DeferredRendering final : public Sample {
    public:
        DeferredRendering() : Sample("Compute: Cloth Simulation") {
            camera.set_position(glm::vec3(0, 2, 6));
            camera.set_look_direction(glm::vec3(0.0f, 0.25f, -1.0f));
            
            dimension = 80;
        }
        
        ~DeferredRendering() override {
        }
        
    private:
        struct Buffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
        };

        struct Object {
            Model model;
            Transform transform;
            
            glm::vec3 diffuse;
            glm::vec3 specular;
            float specular_exponent;
            
            bool flat_shaded;
        } model;
        
        Buffer model_vertex_buffer; // Holds vertex data for the main model
        Buffer index_buffer; // Holds indices for the main model and the cloth

        // SSBO data
        struct Particle {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 velocity;
            glm::vec2 uv;
            alignas(16) glm::vec3 normal;
        };
        
        int dimension; // Number of particles one one side of the cloth
        float size; // World-space size of the cloth
        
        // Updated by the compute pipeline, displayed by the graphics pipeline
        // Holds vertex data for the cloth
        Buffer ssbo;

        // Uniforms
        struct SimulationUniforms {
            float dt;
            float particle_mass;
            float spring_length;
            float spring_length_diagonal;
            glm::vec3 gravity;
            float spring_stiffness;
            glm::vec3 sphere_position;
            float sphere_radius;
            float dampening;
            int dimension;
        };
        
        struct CameraUniforms {
            glm::mat4 camera;
            glm::vec3 camera_position;
        };
        
        // This sample only uses one light
        struct LightUniforms {
            glm::vec3 position;
            float radius;
        };
        
        struct ObjectUniforms {
            glm::mat4 model;
            glm::mat4 normal;
        };
        
        struct PhongUniforms {
            glm::vec3 diffuse;
            int flat_shaded;
            glm::vec3 specular;
            float specular_exponent;
        };
        
        // Uniform buffer layout:
        // simulation uniforms - camera uniforms - lighting uniforms - per model uniforms
        Buffer uniform_buffer;
        void* uniform_buffer_mapped;
        
        struct FramebufferAttachment {
            VkImage image;
            VkDeviceMemory memory;
            VkImageView image_view;
            VkFormat format;
        };

        FramebufferAttachment output_attachment;
        
        VkRenderPass model_render_pass;
        VkPipelineLayout model_pipeline_layout;
        VkPipeline model_pipeline;
        
        VkRenderPass cloth_render_pass;
        VkPipelineLayout cloth_pipeline_layout;
        VkPipeline cloth_pipeline;
        
        VkPipeline compute_pipeline;
        VkPipelineLayout compute_pipeline_layout;
        
        // Descriptor sets
        VkDescriptorSetLayout compute_descriptor_set_layout;
        std::array<VkDescriptorSet, 2> compute_descriptor_sets;
        
        VkDescriptorSetLayout global_descriptor_set_layout;
        VkDescriptorSet global_descriptor_set;
        
        VkDescriptorSetLayout object_descriptor_set_layout;
        VkDescriptorSet object_descriptor_set;
        
        VkSampler sampler;
        
        void initialize_resources() override {
            initialize_geometry_buffers();
            initialize_uniform_buffer();
            
            initialize_samplers();
            
            initialize_command_buffer();
            
            initialize_model_render_pass();
            initialize_cloth_render_pass();
            initialize_framebuffers();
            
            initialize_descriptor_pool(4, 0);
            
            initialize_global_descriptor_set();
            initialize_compute_descriptor_sets();
            initialize_object_descriptor_sets();
            
            initialize_model_pipeline();
            initialize_cloth_pipeline();
            initialize_compute_pipeline();
        }
        
        void destroy_resources() override {
        }
        
        void update() override {
        }
        
        void render() override {
//            VkSemaphore is_image_available = is_presentation_complete[frame_index];
//
//            unsigned image_index;
//            VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<std::uint64_t>::max(), is_image_available, VK_NULL_HANDLE, &image_index);
//
//            // Record command buffer(s)
//            record_command_buffers(image_index);
//
//            VkSubmitInfo submit_info { };
//            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//
//            // Ensure that the swapchain image is available before executing any color operations (writes) by waiting on the pipeline stage that writes to the color attachment (discussed in detail during render pass creation above)
//            // Another approach that can be taken here is to wait on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, which would ensure that no command buffers are executed before the image swapchain image is ready (vkAcquireNextImageKHR signals is_image_available, queue execution waits on is_image_available)
//            // However, this is not the preferred approach - waiting on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT will completely block the pipeline until the swapchain image is ready
//            // Instead, waiting on the pipeline stage where writes are performed to the color attachment allows Vulkan to begin scheduling other work that happens before the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage is reached for execution (such as invoking the vertex shader)
//            // This way, the implementation waits only the time that is absolutely necessary for coherent memory operations
//
//            // Presentation -> geometry buffer pass
//            {
//                VkSemaphore wait_semaphores[] = { is_image_available, is_offscreen_rendering_complete }; // Semaphore(s) to wait on before the command buffers can begin execution
//                VkPipelineStageFlags wait_stage_flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // Note: wait_stage_flags and wait_semaphores have a 1:1 correlation, meaning it is possible to wait on and signal different semaphores at different pipeline stages
//
//                // Waiting on the swapchain image to be ready (if not yet) when the pipeline is ready to perform writes to color attachments
//                submit_info.waitSemaphoreCount = 1;
//                submit_info.pWaitSemaphores = wait_semaphores;
//                submit_info.pWaitDstStageMask = wait_stage_flags;
//
//                submit_info.commandBufferCount = 1;
//                submit_info.pCommandBuffers = &offscreen_command_buffers[frame_index]; // Command buffer(s) to execute
//
//                submit_info.signalSemaphoreCount = 1;
//                submit_info.pSignalSemaphores = &is_offscreen_rendering_complete; // Signal offscreen semaphore
//
//                // Submit
//                if (vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
//                    throw std::runtime_error("failed to submit offscreen command buffer!");
//                }
//            }
//
//            // Geometry buffer pass -> presentation
//            {
//                VkSemaphore wait_semaphores[] = { is_offscreen_rendering_complete };
//                VkPipelineStageFlags wait_stage_flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
//
//                submit_info.waitSemaphoreCount = 1;
//                submit_info.pWaitSemaphores = wait_semaphores;
//                submit_info.pWaitDstStageMask = wait_stage_flags;
//                submit_info.commandBufferCount = 1;
//                submit_info.pCommandBuffers = &command_buffers[frame_index];
//
//                submit_info.signalSemaphoreCount = 1;
//                submit_info.pSignalSemaphores = &is_rendering_complete[frame_index];
//
//                // Submit
//                // TODO: fence is not necessary here
//                if (vkQueueSubmit(queue, 1, &submit_info, is_frame_in_flight[frame_index]) != VK_SUCCESS) {
//                    throw std::runtime_error("failed to submit presentation command buffer!");
//                }
//            }
        }
        
        void initialize_command_buffer() {

        }
        
        void record_command_buffers(unsigned image_index) override {
            record_offscreen_command_buffer();
            record_composition_command_buffer(image_index);
        }
        
        void record_offscreen_command_buffer() {
//            VkCommandBuffer command_buffer = offscreen_command_buffers[frame_index];
//            vkResetCommandBuffer(command_buffer, 0);
//
//            VkCommandBufferBeginInfo command_buffer_begin_info { };
//            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//            command_buffer_begin_info.flags = 0;
//            if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
//                throw std::runtime_error("failed to begin command buffer recording!");
//            }
//
//            VkRenderPassBeginInfo render_pass_info { };
//            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//            render_pass_info.renderPass = offscreen_render_pass;
//            render_pass_info.framebuffer = offscreen_framebuffer; // Only one geometry buffer
//
//            // Specify render area and offset
//            render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
//
//            VkClearValue clear_values[6] { };
//            clear_values[POSITION].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            clear_values[NORMAL].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            clear_values[AMBIENT].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            clear_values[DIFFUSE].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            clear_values[SPECULAR].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            clear_values[DEPTH].depthStencil = { 1.0f, 0 };
//
//            render_pass_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
//            render_pass_info.pClearValues = clear_values;
//
//            unsigned set;
//
//            // Record command for starting the offscreen render pass
//            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
//                // Bind graphics pipeline
//                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreen_pipeline);
//
//                // Bind pipeline-global descriptor sets
//                set = 0;
//                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreen_pipeline_layout, set, 1, &offscreen_global, 0, nullptr);
//
//                for (std::size_t i = 0u; i < scene.objects.size(); ++i) {
//                    const Scene::Object& object = scene.objects[i];
//                    const Model& model = models[object.model];
//
//                    // Bind vertex buffer
//                    VkDeviceSize offsets[] = { object.vertex_offset  };
//                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
//
//                    // Bind index buffer
//                    vkCmdBindIndexBuffer(command_buffer, index_buffer, object.index_offset, VK_INDEX_TYPE_UINT32);
//
//                    // Descriptor sets need to be bound per object
//                    set = 1;
//                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreen_pipeline_layout, set, 1, &offscreen_objects[i], 0, nullptr);
//
//                    // Draw indices.size() vertices which make up 1 instance starting at vertex index 0 and instance index 0.
//                    vkCmdDrawIndexed(command_buffer, (unsigned) model.indices.size(), 1, 0, 0, 0);
//                }
//
//            vkCmdEndRenderPass(command_buffer);
//
//            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
//                throw std::runtime_error("failed to record command buffer!");
//            }
        }
        
        void record_composition_command_buffer(unsigned image_index) {
//            VkCommandBuffer command_buffer = command_buffers[frame_index];
//            vkResetCommandBuffer(command_buffer, 0);
//
//            VkCommandBufferBeginInfo command_buffer_begin_info { };
//            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//            command_buffer_begin_info.flags = 0;
//            if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
//                throw std::runtime_error("failed to begin command buffer recording!");
//            }
//
//            VkRenderPassBeginInfo render_pass_info { };
//            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//            render_pass_info.renderPass = composition_render_pass;
//            render_pass_info.framebuffer = present_framebuffers[image_index]; // Bind the framebuffer for the swapchain image being rendered to
//
//            // Specify render area and offset
//            render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
//
//            VkClearValue clear_value { };
//
//            // Clear value for color attachment
//            clear_value.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
//            render_pass_info.clearValueCount = 1;
//
//            render_pass_info.pClearValues = &clear_value;
//
//            // Record command for starting the composition render pass
//            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
//                // Bind graphics pipeline
//                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pipeline);
//
//                // Bind pipeline-global descriptor sets
//                unsigned set = 0;
//                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pipeline_layout, set, 1, &composition_global, 0, nullptr);
//
//                // Draw a full screen triangle
//                vkCmdDraw(command_buffer, 3, 1, 0, 0);
//
//            // Finish recording the command buffer
//            vkCmdEndRenderPass(command_buffer);
//
//            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
//                throw std::runtime_error("failed to record command buffer!");
//            }
        }
        
        void initialize_model_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
                create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
            };
            
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
                // Ensure that color attachment read operations during the previous frame complete before resetting them for the new render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill color attachments should complete before being read from
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &model_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model render pass!");
            }
        }
        
        void initialize_cloth_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
                create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
            };
            
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
                // Ensure that color attachment read operations during the previous frame complete before resetting them for the new render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill color attachments should complete before being read from
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &cloth_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create cloth render pass!");
            }
        }
        
        void initialize_model_pipeline() {
            VkVertexInputBindingDescription vertex_binding_description = create_vertex_binding_description(0, sizeof(Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
            VkVertexInputAttributeDescription vertex_attribute_descriptions[] {
                create_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Model::Vertex, position)),
                create_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Model::Vertex, normal)),
            };
            
            // Describe the format of the vertex data passed to the vertex shader
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_create_info.vertexBindingDescriptionCount = 1;
            vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding_description;
            vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/model.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/model.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
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
            
            VkDescriptorSetLayout layouts[] = { global_descriptor_set_layout, object_descriptor_set_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &model_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model pipeline layout!");
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
            pipeline_create_info.layout = model_pipeline_layout;
            pipeline_create_info.renderPass = model_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &model_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_cloth_pipeline() {
            // One vertex for the cloth contains position, velocity, uv, and normal
            VkVertexInputBindingDescription vertex_binding_description = create_vertex_binding_description(0, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX);

            // Velocity is only used for the computation stage of the sample, not the rendering
            VkVertexInputAttributeDescription vertex_attribute_descriptions[] {
                create_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, position)),
                create_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, normal)),
                create_vertex_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Particle, uv))
            };
            
            // Describe the format of the vertex data passed to the vertex shader
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_create_info.vertexBindingDescriptionCount = 1;
            vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding_description;
            vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/cloth.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/cloth.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
            };
            
            // Input assembly describes the topology of the geometry being rendered
            VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = create_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        
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
            
            VkDescriptorSetLayout layouts[] = { global_descriptor_set_layout, object_descriptor_set_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &cloth_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model pipeline layout!");
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
            pipeline_create_info.layout = cloth_pipeline_layout;
            pipeline_create_info.renderPass = cloth_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &cloth_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_compute_pipeline() {
            VkPipelineLayoutCreateInfo pipeline_layout_create_info { };
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts = { &compute_descriptor_set_layout };
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &compute_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute pipeline layout!");
            }
            
            VkComputePipelineCreateInfo pipeline_create_info { };
            pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipeline_create_info.layout = compute_pipeline_layout;
            pipeline_create_info.stage = create_shader_stage(create_shader_module(device, "shaders/cloth.comp"), VK_SHADER_STAGE_COMPUTE_BIT);
            if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &compute_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute pipeline!");
            }
        }
        
        void destroy_pipelines() {
            vkDestroyPipelineLayout(device, compute_pipeline_layout, nullptr);
            vkDestroyPipeline(device, compute_pipeline, nullptr);
            
            vkDestroyPipelineLayout(device, cloth_pipeline_layout, nullptr);
            vkDestroyPipeline(device, cloth_pipeline, nullptr);
            
            vkDestroyPipelineLayout(device, model_pipeline_layout, nullptr);
            vkDestroyPipeline(device, model_pipeline, nullptr);
        }
        
        void initialize_graphics_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
                create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
            };
            
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
                // Ensure that color attachment read operations during the previous frame complete before resetting them for the new render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill color attachments should complete before being read from
                create_subpass_dependency(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            
            VkRenderPassCreateInfo render_pass_create_info { };
            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &model_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create model render pass!");
            }
        }
        
        void destroy_render_passes() {
            vkDestroyRenderPass(device, cloth_render_pass, nullptr);
            vkDestroyRenderPass(device, model_render_pass, nullptr);
        }
        
        void initialize_framebuffers() {
            present_framebuffers.resize(NUM_FRAMES_IN_FLIGHT);
            
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkImageView attachments[] = { swapchain_image_views[i], depth_buffer_view };
                
                VkFramebufferCreateInfo framebuffer_create_info { };
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = model_render_pass;
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
        
        void destroy_framebuffers() {
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                vkDestroyFramebuffer(device, present_framebuffers[i], nullptr);
            }
        }
        
        // Uniform buffer layout
        // simulation uniforms - camera uniforms - lighting uniforms - per model uniforms
        
        void initialize_global_descriptor_set() {
            // Descriptor set 1 is used for global uniforms in the graphics pipeline
            VkDescriptorSetLayoutBinding bindings[] = {
                // Binding 0 contains camera uniforms
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
                // Binding 1 contains uniforms for lights
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &global_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate global descriptor set layout!");
            }
            
            VkDescriptorSetAllocateInfo set_allocate_info { };
            set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_allocate_info.descriptorPool = descriptor_pool;
            set_allocate_info.descriptorSetCount = 1;
            set_allocate_info.pSetLayouts = &global_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_allocate_info, &global_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate global descriptor set!");
            }
            
            VkDescriptorBufferInfo buffer_infos[2] { };
            VkWriteDescriptorSet descriptor_writes[2] { };
            
            std::size_t offset = align_to_device_boundary(physical_device, sizeof(SimulationUniforms));
            unsigned binding = 0u;
            
            buffer_infos[binding].buffer = uniform_buffer.buffer;
            buffer_infos[binding].offset = offset;
            buffer_infos[binding].range = sizeof(CameraUniforms);
            
            descriptor_writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding].dstSet = global_descriptor_set;
            descriptor_writes[binding].dstBinding = binding;
            descriptor_writes[binding].dstArrayElement = 0;
            descriptor_writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding].descriptorCount = 1;
            descriptor_writes[binding].pBufferInfo = &buffer_infos[binding];
            
            offset += align_to_device_boundary(physical_device, sizeof(CameraUniforms));
            ++binding;
            
            buffer_infos[binding].buffer = uniform_buffer.buffer;
            buffer_infos[binding].offset = offset;
            buffer_infos[binding].range = sizeof(LightUniforms);
            
            descriptor_writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding].dstSet = global_descriptor_set;
            descriptor_writes[binding].dstBinding = binding;
            descriptor_writes[binding].dstArrayElement = 0;
            descriptor_writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding].descriptorCount = 1;
            descriptor_writes[binding].pBufferInfo = &buffer_infos[binding];
            
            vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
        }
        
        void initialize_object_descriptor_sets() {
            // Descriptor set 2 is used for per-object uniforms in the graphics pipeline
            VkDescriptorSetLayoutBinding bindings[] = {
                // Binding 0 contains model transformations
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
                // Binding 1 contains uniforms for the Phong lighting model
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &object_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate object descriptor set layout!");
            }
            
            VkDescriptorSetAllocateInfo set_allocate_info { };
            set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            set_allocate_info.descriptorPool = descriptor_pool;
            set_allocate_info.descriptorSetCount = 1;
            set_allocate_info.pSetLayouts = &object_descriptor_set_layout;
            if (vkAllocateDescriptorSets(device, &set_allocate_info, &object_descriptor_set) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate object descriptor set!");
            }
            
            // Scene contains two objects that may have different lighting properties
            VkWriteDescriptorSet descriptor_write { };
            VkDescriptorBufferInfo buffer_info { };
            
            std::size_t offset = align_to_device_boundary(physical_device, sizeof(SimulationUniforms)) + align_to_device_boundary(physical_device, sizeof(CameraUniforms)) + align_to_device_boundary(physical_device, sizeof(LightUniforms));
            
            // Object uniforms only for the model
            buffer_info.buffer = uniform_buffer.buffer;
            buffer_info.offset = offset;
            buffer_info.range = sizeof(ObjectUniforms);
            
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = object_descriptor_set;
            descriptor_write.dstBinding = 0;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;
            
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
            
            offset += align_to_device_boundary(physical_device, sizeof(ObjectUniforms));
            
            // Lighting descriptor sets for both the model and cloth
            for (int i = 0; i < 2; ++i) {
                buffer_info.buffer = uniform_buffer.buffer;
                buffer_info.offset = offset;
                buffer_info.range = sizeof(PhongUniforms);
    
                descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_write.dstSet = object_descriptor_set;
                descriptor_write.dstBinding = 1;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.descriptorCount = 1;
                descriptor_write.pBufferInfo = &buffer_info;
                
                vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
                
                offset += align_to_device_boundary(physical_device, sizeof(PhongUniforms));
            }
        }
        
        void initialize_compute_descriptor_sets() {
            // Descriptor set 0 is used for the compute pipeline
            VkDescriptorSetLayoutBinding bindings[] = {
                // Binding 0 contains the input SSBO (particle data from the previous frame)
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
                // Binding 1 contains the output SSBO for the current frame
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
                // Binding 2 contains cloth simulation uniforms
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
            };
            
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &compute_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate compute descriptor set layout!");
            }
            
            // Input and output SSBOs switch between frames
            // This is primarily done to reduce the number of individual buffers required to support a dual-SSBO setup such as this one (both logical buffers are contained within the same SSBO object)
            // One frame uses the first SSBO as readonly and second SSBO as the output, and the next frame uses the second SSBO as input and the first SSBO as output
            // The correct descriptor set is bound at compute at render time
            
            VkWriteDescriptorSet descriptor_writes[3] { };
            VkDescriptorBufferInfo buffer_infos[3] { };
            
            std::size_t shader_storage_buffer_size = sizeof(Particle) * dimension * dimension;
            
            {
                VkDescriptorSetAllocateInfo set_allocate_info { };
                set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                set_allocate_info.descriptorPool = descriptor_pool;
                set_allocate_info.descriptorSetCount = 1;
                set_allocate_info.pSetLayouts = &compute_descriptor_set_layout;
                if (vkAllocateDescriptorSets(device, &set_allocate_info, &compute_descriptor_sets[0]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate compute descriptor set!");
                }
                
                // First SSBO as input and second SSBO as output
                buffer_infos[0].buffer = ssbo.buffer;
                buffer_infos[0].offset = 0u;
                buffer_infos[0].range = shader_storage_buffer_size;
                
                descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[0].dstSet = compute_descriptor_sets[0];
                descriptor_writes[0].dstBinding = 0;
                descriptor_writes[0].dstArrayElement = 0;
                descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptor_writes[0].descriptorCount = 1;
                descriptor_writes[0].pBufferInfo = &buffer_infos[0];
                
                buffer_infos[1].buffer = ssbo.buffer;
                buffer_infos[1].offset = shader_storage_buffer_size;
                buffer_infos[1].range = shader_storage_buffer_size;
                
                descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[1].dstSet = compute_descriptor_sets[0];
                descriptor_writes[1].dstBinding = 1;
                descriptor_writes[1].dstArrayElement = 0;
                descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptor_writes[1].descriptorCount = 1;
                descriptor_writes[1].pBufferInfo = &buffer_infos[1];
                
                buffer_infos[2].buffer = uniform_buffer.buffer;
                buffer_infos[2].offset = 0u;
                buffer_infos[2].range = sizeof(SimulationUniforms);
                
                descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[2].dstSet = compute_descriptor_sets[0];
                descriptor_writes[2].dstBinding = 2;
                descriptor_writes[2].dstArrayElement = 0;
                descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[2].descriptorCount = 1;
                descriptor_writes[2].pBufferInfo = &buffer_infos[2];
                
                vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
            }
            
            // Second SSBO as input and first SSBO as output
            {
                VkDescriptorSetAllocateInfo set_allocate_info { };
                set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                set_allocate_info.descriptorPool = descriptor_pool;
                set_allocate_info.descriptorSetCount = 1;
                set_allocate_info.pSetLayouts = &compute_descriptor_set_layout;
                if (vkAllocateDescriptorSets(device, &set_allocate_info, &compute_descriptor_sets[1]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate compute descriptor set!");
                }
                
                buffer_infos[0].buffer = ssbo.buffer;
                buffer_infos[0].offset = shader_storage_buffer_size;
                buffer_infos[0].range = shader_storage_buffer_size;
                
                descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[0].dstSet = compute_descriptor_sets[1];
                descriptor_writes[0].dstBinding = 0;
                descriptor_writes[0].dstArrayElement = 0;
                descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptor_writes[0].descriptorCount = 1;
                descriptor_writes[0].pBufferInfo = &buffer_infos[0];
                
                buffer_infos[1].buffer = ssbo.buffer;
                buffer_infos[1].offset = 0u;
                buffer_infos[1].range = shader_storage_buffer_size;
                
                descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[1].dstSet = compute_descriptor_sets[1];
                descriptor_writes[1].dstBinding = 1;
                descriptor_writes[1].dstArrayElement = 0;
                descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptor_writes[1].descriptorCount = 1;
                descriptor_writes[1].pBufferInfo = &buffer_infos[1];
                
                buffer_infos[2].buffer = uniform_buffer.buffer;
                buffer_infos[2].offset = 0u;
                buffer_infos[2].range = sizeof(SimulationUniforms);
                
                descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[2].dstSet = compute_descriptor_sets[1];
                descriptor_writes[2].dstBinding = 2;
                descriptor_writes[2].dstArrayElement = 0;
                descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[2].descriptorCount = 1;
                descriptor_writes[2].pBufferInfo = &buffer_infos[2];
                
                vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
            }
        }

        void initialize_geometry_buffers() {
            model.model = load_obj("assets/models/knight.obj");
            model.diffuse = glm::vec3(0.8f);
            model.specular = glm::vec3(0.0f);
            model.specular_exponent = 0.0f;
            model.transform = Transform(glm::vec3(0.0f), glm::vec3(1.5f), glm::vec3(0.0f, 50.0f, 0.0f));
            model.flat_shaded = true;
            
            std::size_t model_vertex_buffer_size = model.model.vertices.size() * sizeof(Model::Vertex);
            std::size_t model_index_buffer_size = model.model.indices.size() * sizeof(unsigned);
            
            std::vector<Particle> cloth_vertices(dimension * dimension);
            float dp = (float) size / (float) (dimension - 1);
            
            // Initialize cloth particles
            for (int z = 0; z < dimension; ++z) {
                for (int x = 0; x < dimension; ++x) {
                    // Center cloth at (0, 0)
                    cloth_vertices[x + z * dimension].position = glm::vec3((float)(-dimension) / 2.0f + dp * (float) x, 4.0f, (float) -dimension / 2.0f + dp * (float) z);
                    cloth_vertices[x + z * dimension].velocity = glm::vec3(0.0f);
                    cloth_vertices[x + z * dimension].uv = glm::vec2(0.0f);
                    cloth_vertices[x + z * dimension].normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
            
            std::size_t cloth_vertex_buffer_size = cloth_vertices.size() * sizeof(Particle);
            
            std::vector<unsigned> cloth_indices;
            cloth_indices.reserve(dimension * dimension);
            unsigned primitive_restart_index = 0xFFFFFFFF;
            
            // Order cloth vertex indices in a triangle strip pattern:
            //  1     3     5     7     9     11    13
            //  o-----o-----o-----o-----o-----o-----o
            //  | \   | \   | \   | \   | \   | \   | -
            //  |   \ |   \ |   \ |   \ |   \ |   \ |   -
            //  o-----o-----o-----o-----o-----o-----o - - 0xFFFFFFFF (primitive restart
            //  0     2     4     6     8     10    12
            
            for (int z = 0; z < dimension - 1; ++z) {
                for (int x = 0; x < dimension; ++x) {
                    cloth_indices.emplace_back((z + 1) * dimension + x);
                    cloth_indices.emplace_back(z * dimension + x);
                }
                cloth_indices.emplace_back(primitive_restart_index);
            }
            
            std::size_t cloth_index_buffer_size = cloth_indices.size() * sizeof(unsigned);
            
            Buffer staging_buffer { };
            VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkMemoryPropertyFlags staging_buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            
            // Use one staging buffer to transfer all data from subranges
            std::size_t storage_buffer_size = model_vertex_buffer_size + cloth_vertex_buffer_size + model_index_buffer_size + cloth_index_buffer_size;
            create_buffer(physical_device, device, storage_buffer_size, staging_buffer_usage, staging_buffer_memory_properties, staging_buffer.buffer, staging_buffer.memory);
            
            void* data = nullptr;
            std::size_t offset = 0u;
            vkMapMemory(device, staging_buffer.memory, 0, storage_buffer_size, 0, &data);
                // Copy model vertex data
                memcpy((void*)(((const char*) data) + offset), model.model.vertices.data(), model_vertex_buffer_size);
                offset += model_vertex_buffer_size;
                
                // Copy cloth vertex data
                memcpy((void*)(((const char*) data) + offset), cloth_vertices.data(), cloth_vertex_buffer_size);
                offset += cloth_vertex_buffer_size;
                
                // Copy model index data
                memcpy((void*)(((const char*) data) + offset), model.model.indices.data(), model_index_buffer_size);
                offset += model_index_buffer_size;
                
                // Copy cloth index data
                memcpy((void*)(((const char*) data) + offset), cloth_indices.data(), cloth_index_buffer_size);
                offset += cloth_index_buffer_size;
            vkUnmapMemory(device, staging_buffer.memory);
            
            // Model vertex buffer
            VkBufferUsageFlags model_vertex_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags model_vertex_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, model_vertex_buffer_size, model_vertex_buffer_usage, model_vertex_buffer_memory_properties, model_vertex_buffer.buffer, model_vertex_buffer.memory);
            
            // Index buffer (for both the main model and cloth)
            VkBufferUsageFlags index_buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT; // This buffer is the destination buffer in a memory transfer operation (and also the index buffer).
            VkMemoryPropertyFlags index_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, model_index_buffer_size + cloth_index_buffer_size, index_buffer_usage, index_buffer_memory_properties, index_buffer.buffer, index_buffer.memory);
            
            // Storage buffer (also used as vertex buffer for rendering the cloth)
            // Buffers are used as storage buffers (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), inputs to the vertex shader (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), and a destination for transfer operations (VK_BUFFER_USAGE_TRANSFER_DST_BIT) for transferring cloth particle data from the staging buffer
            VkBufferUsageFlags storage_buffer_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VkMemoryPropertyFlags storage_buffer_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            create_buffer(physical_device, device, cloth_vertex_buffer_size * 2, storage_buffer_usage, storage_buffer_memory_properties, ssbo.buffer, ssbo.memory);
            
            // Copy from staging buffer into device-local memory
            offset = 0u;
            VkCommandBuffer command_buffer = begin_transient_command_buffer();
                // Copy model vertex data
                copy_buffer(command_buffer, staging_buffer.buffer, offset, model_vertex_buffer.buffer, 0, model_vertex_buffer_size);
                offset += model_vertex_buffer_size;
                
                // Copy cloth vertex data
                copy_buffer(command_buffer, staging_buffer.buffer, offset, ssbo.buffer, 0, cloth_vertex_buffer_size);
                offset += cloth_vertex_buffer_size;
                
                // Copy model index data
                copy_buffer(command_buffer, staging_buffer.buffer, offset, index_buffer.buffer, 0, model_index_buffer_size);
                offset += model_index_buffer_size;
                
                // Copy cloth index data
                copy_buffer(command_buffer, staging_buffer.buffer, offset, index_buffer.buffer, model_index_buffer_size, cloth_index_buffer_size);
                offset += cloth_index_buffer_size;
            submit_transient_command_buffer(command_buffer);
            
            // Staging buffer resources are no longer necessary
            vkFreeMemory(device, staging_buffer.memory, nullptr);
            vkDestroyBuffer(device, staging_buffer.buffer, nullptr);
        }
        
        void destroy_buffers() {
            // Destroy geometry buffers
            vkFreeMemory(device, index_buffer.memory, nullptr);
            vkDestroyBuffer(device, index_buffer.buffer, nullptr);

            vkFreeMemory(device, model_vertex_buffer.memory, nullptr);
            vkDestroyBuffer(device, model_vertex_buffer.buffer, nullptr);
            
            // Destroy shader storage buffer
            vkFreeMemory(device, ssbo.memory, nullptr);
            vkDestroyBuffer(device, ssbo.buffer, nullptr);
        }
        
        void initialize_samplers() {
            // Texture access in shaders is typically done through image samplers
            // Image samplers apply filtering and transformations to compute the final color retrieved from the image
            VkSamplerCreateInfo texture_sampler_create_info { };
            texture_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            
            // Filters help deal with oversampling (when there is more geometry fragments than texels) and blend between texture colors
            //   - VK_FILTER_LINEAR - bilinear filtering (interpolates from texel neighbors)
            //   - VK_FILTER_NEAREST - no filtering, takes the nearest texel value
            texture_sampler_create_info.magFilter = VK_FILTER_LINEAR; // Magnification filter, more than one pixel per texel
            texture_sampler_create_info.minFilter = VK_FILTER_LINEAR; // Minification filter, more than one texel per pixel
            
            // Samplers also take care of transformations (what happens when texels are read from outside the image)
            //   - VK_SAMPLER_ADDRESS_MODE_REPEAT - texture is repeated when sampled beyond the original image dimension
            //   - VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT - like VK_SAMPLER_ADDRESS_MODE_REPEAT, but sample coordinates are inverted to mirror the image across each of the principal axes
            //   - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE - takes the color of the edge closest to the coordinate beyond the image dimensions
            //   - VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE - like VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, but takes the edge opposite to the closest edge
            //   - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER - returns a solid color when sampled beyond the image dimensions
            texture_sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // x
            texture_sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // y
            texture_sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // z (for 3D textures)
            
            // Anisotropic filtering helps deal with oversampling (where there are more texels than fragments) at very steep viewing angles, taking into consideration each texture's location on the screen relative to the camera angle
            if (enabled_physical_device_features.samplerAnisotropy) {
                texture_sampler_create_info.anisotropyEnable = VK_TRUE;
                
                // Set to the maximum supported number of texel samples that should be used to calculate the final pixel color
                // A lower number yields higher performance but has a lower quality final image
                texture_sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
            }
            else {
                // Selected physical device does not support anisotropic filtering
                texture_sampler_create_info.anisotropyEnable = VK_FALSE;
                texture_sampler_create_info.maxAnisotropy = 1.0f;
            }
        
            // Specify what color to return when sampling beyond image dimensions when using VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
            // Choose between black, white, or transparent (FLOAT/INT formats)
            texture_sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            
            // Specify if sample coordinates should use the unnormalized [0, texture_width) to [0, texture_height) range instead of the normalized [0, 1) range on all axes
            texture_sampler_create_info.unnormalizedCoordinates = VK_FALSE;
            
            texture_sampler_create_info.compareEnable = VK_FALSE;
            texture_sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
            
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
            texture_sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            texture_sampler_create_info.mipLodBias = 0.0f;
            texture_sampler_create_info.minLod = 0.0f;
            texture_sampler_create_info.maxLod = 1.0f;
            
            if (vkCreateSampler(device, &texture_sampler_create_info, nullptr, &sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }
        
        void initialize_uniform_buffer() {
            // simulation uniforms + camera uniforms + lighting uniforms + per model uniforms (for 2 models)
            std::size_t uniform_buffer_size = align_to_device_boundary(physical_device, sizeof(SimulationUniforms)) +
                                              align_to_device_boundary(physical_device, sizeof(CameraUniforms)) +
                                              align_to_device_boundary(physical_device, sizeof(LightUniforms)) +
                                              align_to_device_boundary(physical_device, sizeof(ObjectUniforms) + align_to_device_boundary(physical_device, sizeof(PhongUniforms))) * 2;
            
            create_buffer(physical_device, device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer.buffer, uniform_buffer.memory);
            vkMapMemory(device, uniform_buffer.memory, 0, uniform_buffer_size, 0, &uniform_buffer_mapped);
        }
        
        void update_object_uniform_buffers(unsigned id) {
        }
        
        void update_uniform_buffers() {
            SimulationUniforms simulation_uniforms { };
            simulation_uniforms.dt = (float) dt;
            simulation_uniforms.particle_mass = 0.5f;
            simulation_uniforms.spring_length = size / (float) dimension;
            simulation_uniforms.spring_length_diagonal = std::sqrt(simulation_uniforms.spring_length * simulation_uniforms.spring_length);
            simulation_uniforms.gravity = glm::vec3(0.0f, 1.0f, 0.0f);
            simulation_uniforms.spring_stiffness = 2.0f;
            simulation_uniforms.sphere_position = model.transform.get_position();
            simulation_uniforms.sphere_radius = model.transform.get_scale().x; // Assume the same in all 3 directions
            simulation_uniforms.dampening = 1.0f;
            simulation_uniforms.dimension = dimension;
            
            CameraUniforms camera_uniforms { };
            camera_uniforms.camera = camera.get_projection_matrix() * camera.get_view_matrix();
            camera_uniforms.camera_position = camera.get_position();
            
            LightUniforms light_uniforms { };
            light_uniforms.position = glm::vec3(0.0f, 3.0f, 0.0f);
            light_uniforms.radius = 5.0f;
            
            // Model lighting uniforms
            ObjectUniforms model_uniforms { };
            model_uniforms.model = model.transform.get_matrix();
            model_uniforms.normal = glm::transpose(glm::inverse(model_uniforms.model));
            
            PhongUniforms model_lighting_uniforms { };
            model_lighting_uniforms.diffuse = model.diffuse;
            model_lighting_uniforms.specular = model.specular;
            model_lighting_uniforms.specular_exponent = model.specular_exponent;
            model_lighting_uniforms.flat_shaded = (int) model.flat_shaded;
            
            PhongUniforms cloth_lighting_uniforms { };
            cloth_lighting_uniforms.diffuse = glm::vec3(0.0f);
            cloth_lighting_uniforms.specular = glm::vec3(0.0f);
            cloth_lighting_uniforms.specular_exponent = 1.0f;
            cloth_lighting_uniforms.flat_shaded = 1;
        }
        
        void destroy_uniform_buffer() {
            vkFreeMemory(device, uniform_buffer.memory, nullptr);
            vkDestroyBuffer(device, uniform_buffer.buffer, nullptr);
        }
        
        void on_key_pressed(int key) override {
        }
        
};

DEFINE_SAMPLE_MAIN(DeferredRendering);