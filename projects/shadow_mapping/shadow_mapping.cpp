
#include "sample.hpp"
#include "helpers.hpp"
#include "vulkan_initializers.hpp"
#include "loaders/obj.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

class ShadowMapping final : public Sample {
    public:
        ShadowMapping() : Sample("Shadow Mapping") {
            enabled_physical_device_features.geometryShader = (VkBool32) true;
            
            camera.set_position(glm::vec3(0, 2, 6));
            camera.set_look_direction(glm::vec3(0.0f, 0.25f, -1.0f));
        }
        
        ~ShadowMapping() override {
        }
        
    private:
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
            
            // Custom structures must be aligned to a 16-byte boundary in uniform buffers
            struct alignas(16) Light {
                glm::mat4 transform;
                glm::vec3 position;
                float outer;
                glm::vec3 direction;
                float inner;
                glm::vec3 color;
                int type;
            };

            std::vector<Object> objects;
            std::vector<Light> lights;
        } scene;
        
        // Geometry buffers
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        
        struct FramebufferAttachment {
            VkImage image;
            VkDeviceMemory memory;
            VkImageView image_view;
            VkFormat format;
        };
        
        // Section: shadow map
        FramebufferAttachment shadow_attachment;
        VkFramebuffer shadow_framebuffer;
        
        VkPipeline shadow_pipeline;
        VkPipelineLayout shadow_pipeline_layout;
        
        VkRenderPass shadow_render_pass;
        
        // Descriptor set for shader globals used across the shadow map and geometry buffer render passes
        VkDescriptorSetLayout global_descriptor_set_layout;
        VkDescriptorSet global_descriptor_set;
        
        VkDescriptorSetLayout object_descriptor_set_layout;
        std::vector<VkDescriptorSet> object_descriptor_sets;
        
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
        
        struct PhongUniforms {
            glm::vec3 ambient;
            float specular_exponent;
            glm::vec3 diffuse;
            int flat_shaded;
            alignas(16) glm::vec3 specular;
        };

        // Section: geometry buffer
        
        // Position, normals, ambient, diffuse, specular
        std::array<FramebufferAttachment, 5> geometry_framebuffer_attachments;
        VkFramebuffer geometry_framebuffer;
        
        VkPipeline geometry_pipeline;
        VkPipelineLayout geometry_pipeline_layout;
        
        VkRenderPass geometry_render_pass;
        
        // Composition
        VkPipelineLayout composition_pipeline_layout;
        VkPipeline composition_pipeline;
        
        VkRenderPass composition_render_pass;
        
        // Uniform buffers
        VkBuffer uniform_buffer; // One uniform buffer for all uniforms, across both passes
        VkDeviceMemory uniform_buffer_memory;
        void* uniform_buffer_mapped;

        VkSampler color_sampler;
        VkSampler depth_sampler;
        
        void initialize_resources() override {
            initialize_buffers();
            initialize_lights();
            
            initialize_samplers();
            
            initialize_shadow_map_render_pass();
            initialize_geometry_render_pass();
            initialize_composition_render_pass();
            
            initialize_shadow_map_framebuffer();
            initialize_geometry_framebuffer();
            initialize_composition_framebuffers();

            // Two global descriptor sets (camera + lights)
            // Two descriptor sets per object (transforms + material properties)
            // 7 image samplers (positions, normals, ambient, diffuse, specular, depth, shadow)
            initialize_descriptor_pool(2 + scene.objects.size() * 2, 7);
            
            initialize_uniform_buffer();
            
            initialize_global_descriptor_set();
            initialize_object_descriptor_sets();

            initialize_shadow_map_pipeline();
            initialize_geometry_pipeline();
            initialize_composition_pipeline();
        }
        
        void destroy_resources() override {
        }
        
        void update() override {
            Scene::Object& object = scene.objects.back();
            Transform& transform = object.transform;
            transform.set_rotation(transform.get_rotation() + (float)dt * glm::vec3(0.0f, -10.0f, 0.0f));
            
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
        
            // Overview of a frame:
            // 1. Generate shadow map
            // 2. Generate geometry buffer
            // 3. Composition pass
            
            VkRenderPass render_passes[3] = {
                shadow_render_pass,
                geometry_render_pass,
                composition_render_pass
            };
            
            VkFramebuffer framebuffers[3] = {
                shadow_framebuffer,
                geometry_framebuffer,
                present_framebuffers[image_index]
            };
            
            VkPipeline pipelines[3] = {
                shadow_pipeline,
                geometry_pipeline,
                composition_pipeline
            };
            
            VkPipelineLayout pipeline_layouts[3] {
                shadow_pipeline_layout,
                geometry_pipeline_layout,
                composition_pipeline_layout
            };
            
            for (std::size_t pass = 0u; pass < 3; ++pass) {
                VkRenderPassBeginInfo render_pass_info { };
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = render_passes[pass];
                render_pass_info.framebuffer = framebuffers[pass];
                render_pass_info.renderArea = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
                
                // TODO: this can be simplified
                if (pass == 0) {
                    // Shadow pass
                    VkClearValue clear_value { };
                    clear_value.depthStencil = { 1.0f, 0 }; // Shadow pass only uses depth information
                    render_pass_info.clearValueCount = 1;
                    render_pass_info.pClearValues = &clear_value;
                }
                else if (pass == 1) {
                    // Geometry pass
                    VkClearValue clear_values[6] { };
                    clear_values[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Positions
                    clear_values[1].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Normals
                    clear_values[2].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Ambient
                    clear_values[3].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Diffuse
                    clear_values[4].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Specular
                    clear_values[5].depthStencil = { 1.0f, 0 }; // Depth
                    
                    render_pass_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
                    render_pass_info.pClearValues = clear_values;
                }
                else if (pass == 2) {
                    // Composition pass
                    VkClearValue clear_value { };
                    clear_value.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }}; // Output attachment
                    render_pass_info.clearValueCount = 1;
                    render_pass_info.pClearValues = &clear_value;
                }
                
                vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                    // Bind graphics pipeline
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[pass]);
    
                    // Bind global descriptor set
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[pass], 0, 1, &global_descriptor_set, 0, nullptr);
    
                    if (pass == 2) {
                        // Draw a full screen triangle
                        vkCmdDraw(command_buffer, 3, 1, 0, 0);
                    }
                    else {
                        for (std::size_t i = 0u; i < scene.objects.size(); ++i) {
                            const Scene::Object& object = scene.objects[i];
                            const Model& model = models[object.model];
        
                            // Bind vertex + index buffers buffer
                            VkDeviceSize offsets[] = { object.vertex_offset };
                            vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
                            vkCmdBindIndexBuffer(command_buffer, index_buffer, object.index_offset, VK_INDEX_TYPE_UINT32);
        
                            // Bind per-object descriptor set
                            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[pass], 1, 1, &object_descriptor_sets[i], 0, nullptr);
        
                            vkCmdDrawIndexed(command_buffer, (unsigned) model.indices.size(), 1, 0, 0, 0);
                        }
                    }
                vkCmdEndRenderPass(command_buffer);
            }
            
            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
        
        void initialize_shadow_map_pipeline() {
            VkVertexInputBindingDescription vertex_binding_descriptions[] {
                create_vertex_binding_description(0, sizeof(glm::vec3) * 2, VK_VERTEX_INPUT_RATE_VERTEX) // One element is vertex position (vec3) + normal (vec3)
            };

            // The shadow map generation pipeline only uses vertex positions
            VkVertexInputAttributeDescription vertex_attribute_descriptions[] {
                create_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            };
            
            // Describe the format of the vertex data passed to the vertex shader
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_create_info.vertexBindingDescriptionCount = sizeof(vertex_binding_descriptions) / sizeof(vertex_binding_descriptions[0]);
            vertex_input_create_info.pVertexBindingDescriptions = vertex_binding_descriptions;
            vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);
            vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
            
            // Bundle shader stages to assign to pipeline
            
            // Shader constants
            VkSpecializationMapEntry specialization { };
            
            // layout (constant_id = 0) const int LIGHT_COUNT = 32;
            specialization.constantID = 0;
            specialization.size = sizeof(int);
            specialization.offset = 0;
            
            
            VkSpecializationInfo specialization_info { };
            specialization_info.mapEntryCount = 1;
            specialization_info.pMapEntries = &specialization;
            specialization_info.dataSize = sizeof(int);
            
            int light_count = scene.lights.size();
            specialization_info.pData = &light_count;
            
            // A custom fragment shader stage is not necessary, since the only thing we care about is depth information and that gets written automatically
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/shadow_map.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/shadow_map.geom"), VK_SHADER_STAGE_GEOMETRY_BIT, &specialization_info),
            };
            
            VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = create_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            
            VkViewport viewport = create_viewport(0.0f, 0.0f, (float) swapchain_extent.width, (float) swapchain_extent.height, 0.0f, 1.0f);
            VkRect2D scissor = create_region(0, 0, swapchain_extent.width, swapchain_extent.height);
            
            VkPipelineViewportStateCreateInfo viewport_create_info { };
            viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_create_info.viewportCount = 1;
            viewport_create_info.pViewports = &viewport;
            viewport_create_info.scissorCount = 1;
            viewport_create_info.pScissors = &scissor;
            
            VkPipelineRasterizationStateCreateInfo rasterizer_create_info { };
            rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer_create_info.depthClampEnable = VK_FALSE;
            rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
            rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer_create_info.lineWidth = 1.0f;
            rasterizer_create_info.cullMode = VK_CULL_MODE_FRONT_BIT; // Cull front-facing polygons to prevent peter panning of shadows
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
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Fragments that are closer have a lower depth value and should be kept (fragments further away are discarded)
            
            VkPipelineColorBlendStateCreateInfo color_blend_create_info { };
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.logicOpEnable = VK_FALSE; // Use a bitwise operation to combine the old and new color values (setting this to true disables color mixing (specified above) for all framebuffers)
            color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_create_info.attachmentCount = 0; // Shadow map uses no color attachments
            color_blend_create_info.pAttachments = nullptr;
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
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &shadow_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shadow pipeline layout!");
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
            pipeline_create_info.layout = shadow_pipeline_layout;
            pipeline_create_info.renderPass = shadow_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &shadow_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shadow pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
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
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/geometry_buffer.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/geometry_buffer.frag"), VK_SHADER_STAGE_FRAGMENT_BIT),
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
            
            VkPipelineColorBlendAttachmentState color_blend_attachment_create_info { };
            color_blend_attachment_create_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment_create_info.blendEnable = VK_FALSE;
            color_blend_attachment_create_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment_create_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment_create_info.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment_create_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment_create_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment_create_info.alphaBlendOp = VK_BLEND_OP_ADD;
        
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
            
            VkDescriptorSetLayout layouts[2] = { global_descriptor_set_layout, object_descriptor_set_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &geometry_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create geometry pipeline layout!");
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
            pipeline_create_info.layout = geometry_pipeline_layout;
            pipeline_create_info.renderPass = geometry_render_pass;
            pipeline_create_info.subpass = 0;
        
            // TODO: Allows for recreating a pipeline from an existing pipeline
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;
        
            // TODO: pipeline caching.
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &geometry_pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create geometry pipeline!");
            }

            for (const VkPipelineShaderStageCreateInfo& stage : shader_stages) {
                vkDestroyShaderModule(device, stage.module, nullptr);
            }
        }
        
        void initialize_composition_pipeline() {
            // Vertex data is generated directly in the vertex shader, no vertex input state to specify
            VkPipelineVertexInputStateCreateInfo vertex_input_create_info { };
            vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            
            // Shader constants
            VkSpecializationMapEntry specialization { };
            
            // layout (constant_id = 0) const int LIGHT_COUNT = 32;
            specialization.constantID = 0;
            specialization.size = sizeof(int);
            specialization.offset = 0;
            
            VkSpecializationInfo specialization_info { };
            specialization_info.mapEntryCount = 1;
            specialization_info.pMapEntries = &specialization;
            specialization_info.dataSize = sizeof(int);
            int light_count = scene.lights.size();
            specialization_info.pData = &light_count;
            
            // Bundle shader stages to assign to pipeline
            VkPipelineShaderStageCreateInfo shader_stages[] = {
                create_shader_stage(create_shader_module(device, "shaders/composition.vert"), VK_SHADER_STAGE_VERTEX_BIT),
                create_shader_stage(create_shader_module(device, "shaders/composition.frag"), VK_SHADER_STAGE_FRAGMENT_BIT, &specialization_info),
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
            
            VkDescriptorSetLayout layouts[1] = { global_descriptor_set_layout };
            pipeline_layout_create_info.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
            pipeline_layout_create_info.pSetLayouts = layouts;

            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
            if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &composition_pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create composition pipeline layout!");
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
            vkDestroyPipelineLayout(device, composition_pipeline_layout, nullptr);
            vkDestroyPipeline(device, composition_pipeline, nullptr);
            
            vkDestroyPipelineLayout(device, geometry_pipeline_layout, nullptr);
            vkDestroyPipeline(device, geometry_pipeline, nullptr);
            
            vkDestroyPipelineLayout(device, shadow_pipeline_layout, nullptr);
            vkDestroyPipeline(device, shadow_pipeline, nullptr);
        }
        
        void initialize_shadow_map_render_pass() {
            // TODO: consolidate specifying attachment format into one place
            VkAttachmentDescription attachment_description = create_attachment_description(depth_buffer_format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL);
            VkAttachmentReference depth_attachment_reference = create_attachment_reference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = 0;
            subpass_description.pColorAttachments = nullptr;
            subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
            subpass_description.pResolveAttachments = nullptr; // Not used in this sample
            subpass_description.pInputAttachments = nullptr; // Not used in this sample
            subpass_description.pPreserveAttachments = nullptr; // Not used in this sample
            
            // Define subpass dependencies
            VkSubpassDependency subpass_dependencies[] {
                // Ensure that attachment read operations during the previous frame complete before resetting them for the new render pass
                create_subpass_dependency(VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                          0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
                                          
                // Write operations to fill attachments should complete before being read from
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
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &shadow_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shadow map render pass!");
            }
        }
        
        void initialize_geometry_render_pass() {
            // TODO: consolidate specifying attachment format into one place
            VkAttachmentDescription attachment_descriptions[] {
                // Positions
                create_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                // Normals
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
                create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                create_attachment_reference(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                create_attachment_reference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                create_attachment_reference(3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                create_attachment_reference(4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            };
            VkAttachmentReference depth_stencil_attachment_reference = create_attachment_reference(5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            
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
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &geometry_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create geometry render pass!");
            }
        }
        
        void initialize_composition_render_pass() {
            VkAttachmentDescription attachment_descriptions[] {
                create_attachment_description(surface_format.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
            };
            
            VkAttachmentReference color_attachment_references[] {
                create_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            };
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.colorAttachmentCount = sizeof(color_attachment_references) / sizeof(color_attachment_references[0]);
            subpass_description.pColorAttachments = color_attachment_references;
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
            render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);
            render_pass_create_info.pAttachments = attachment_descriptions;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass_description;
            render_pass_create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
            render_pass_create_info.pDependencies = subpass_dependencies;
            if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &composition_render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create composition render pass!");
            }
        }
        
        void destroy_render_passes() {
            vkDestroyRenderPass(device, shadow_render_pass, nullptr);
            vkDestroyRenderPass(device, geometry_render_pass, nullptr);
            vkDestroyRenderPass(device, composition_render_pass, nullptr);
        }
        
        void initialize_framebuffers() {
            initialize_shadow_map_framebuffer();
            initialize_geometry_framebuffer();
            initialize_composition_framebuffers();
        }
        
        void destroy_framebuffers() {
            vkDestroyFramebuffer(device, shadow_framebuffer, nullptr);
            vkDestroyFramebuffer(device, geometry_framebuffer, nullptr);
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                vkDestroyFramebuffer(device, present_framebuffers[i], nullptr);
            }
        }
        
        void initialize_shadow_map_framebuffer() {
            unsigned mip_levels = 1;
            unsigned layers = scene.lights.size();
            
            // Create a layered depth attachment for rendering light depth maps to, one for each light
            // Note: this will have to get recreated if the number of lights change
            create_image(physical_device, device,
                         swapchain_extent.width, swapchain_extent.height, mip_levels, layers,
                         VK_SAMPLE_COUNT_1_BIT,
                         depth_buffer_format, // Use the same format for the shadow map as what is used for the depth buffer
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         shadow_attachment.image, shadow_attachment.memory);
            create_image_view(device, shadow_attachment.image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, depth_buffer_format, VK_IMAGE_ASPECT_DEPTH_BIT, mip_levels, layers, shadow_attachment.image_view);
            
            VkFramebufferCreateInfo framebuffer_create_info { };
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = shadow_render_pass;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments = &shadow_attachment.image_view;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = layers;
            
            if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &shadow_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        void initialize_geometry_framebuffer() {
            // Initialize color attachments
            
            // Format for position and normal attachments (can be negative)
            VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
            unsigned mip_levels = 1;
            unsigned layers = 1;
            
            for (std::size_t i = 0u; i < 2; ++i) {
                create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, mip_levels, layers, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, geometry_framebuffer_attachments[i].image, geometry_framebuffer_attachments[i].memory);
                create_image_view(device, geometry_framebuffer_attachments[i].image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels, layers, geometry_framebuffer_attachments[i].image_view);
            }
            
            // Ambient, diffuse, and specular attachments
            // Use an unsigned format as color cannot be negative
            format = VK_FORMAT_R8G8B8A8_UNORM;
            
            for (std::size_t i = 2u; i < 5; ++i) {
                create_image(physical_device, device, swapchain_extent.width, swapchain_extent.height, mip_levels, layers, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, geometry_framebuffer_attachments[i].image, geometry_framebuffer_attachments[i].memory);
                create_image_view(device, geometry_framebuffer_attachments[i].image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels, layers, geometry_framebuffer_attachments[i].image_view);
            }
            
            VkImageView attachments[] {
                geometry_framebuffer_attachments[0].image_view, // Positions
                geometry_framebuffer_attachments[1].image_view, // Normals
                geometry_framebuffer_attachments[2].image_view, // Ambient
                geometry_framebuffer_attachments[3].image_view, // Diffuse
                geometry_framebuffer_attachments[4].image_view, // Specular
                depth_buffer_view // Depth
            };
            
            VkFramebufferCreateInfo framebuffer_create_info { };
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = geometry_render_pass;
            framebuffer_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            framebuffer_create_info.pAttachments = attachments;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = layers;
            
            if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &geometry_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        void initialize_composition_framebuffers() {
            present_framebuffers.resize(NUM_FRAMES_IN_FLIGHT);
            
            for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                VkImageView attachments[] = { swapchain_image_views[i] };
                
                VkFramebufferCreateInfo framebuffer_create_info { };
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = composition_render_pass;
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
        
        void initialize_global_descriptor_set() {
            // Descriptor set 0 is allocated for global uniforms that do not change between pipelines
            VkDescriptorSetLayoutBinding bindings[] {
                // Global camera information
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
                
                // Light data
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                
                // Positions
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                
                // Normals
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
                
                // Ambient
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
                
                // Diffuse
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
                
                // Specular
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
                
                // Depth
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7),
                
                // Shadow map
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8),
            };
            
            // Initialize the descriptor set layout
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &global_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
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
            
            VkWriteDescriptorSet descriptor_writes[9] { };
            VkDescriptorBufferInfo buffer_infos[2] { };
            
            unsigned binding = 0u;
            std::size_t offset = 0u;
            
            // Binding 0
            buffer_infos[binding].buffer = uniform_buffer;
            buffer_infos[binding].offset = offset;
            buffer_infos[binding].range = sizeof(GlobalUniforms);
            offset += align_to_device_boundary(physical_device, buffer_infos[binding].range);
            
            descriptor_writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding].dstSet = global_descriptor_set;
            descriptor_writes[binding].dstBinding = binding;
            descriptor_writes[binding].dstArrayElement = 0;
            descriptor_writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding].descriptorCount = 1;
            descriptor_writes[binding].pBufferInfo = &buffer_infos[binding];
            
            ++binding;
            
            // Binding 1
            buffer_infos[binding].buffer = uniform_buffer;
            buffer_infos[binding].offset = offset;
            buffer_infos[binding].range = sizeof(Scene::Light) * scene.lights.size();
            offset += align_to_device_boundary(physical_device, buffer_infos[binding].range);
            
            descriptor_writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[binding].dstSet = global_descriptor_set;
            descriptor_writes[binding].dstBinding = binding;
            descriptor_writes[binding].dstArrayElement = 0;
            descriptor_writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[binding].descriptorCount = 1;
            descriptor_writes[binding].pBufferInfo = &buffer_infos[binding];
            
            VkDescriptorImageInfo image_infos[7] { };
            
            // Bindings 2 - 8
            unsigned starting_binding = ++binding;
            for (; binding < 9; ++binding) {
                if (binding == 7) {
                    // Depth attachment
                    image_infos[binding - starting_binding].imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
                    image_infos[binding - starting_binding].imageView = depth_buffer_view;
                    image_infos[binding - starting_binding].sampler = depth_sampler;
                }
                else if (binding == 8) {
                    // Shadow map attachment
                    // Shadow map uses depth format
                    image_infos[binding - starting_binding].imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
                    
                    // Shadow map image view comes from the shadow map framebuffer
                    image_infos[binding - starting_binding].imageView = shadow_attachment.image_view;
                    image_infos[binding - starting_binding].sampler = depth_sampler;
                }
                else {
                    image_infos[binding - starting_binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_infos[binding - starting_binding].imageView = geometry_framebuffer_attachments[binding - starting_binding].image_view;
                    image_infos[binding - starting_binding].sampler = color_sampler;
                }
                
                descriptor_writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[binding].dstSet = global_descriptor_set;
                descriptor_writes[binding].dstBinding = binding;
                descriptor_writes[binding].dstArrayElement = 0;
                descriptor_writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_writes[binding].descriptorCount = 1;
                descriptor_writes[binding].pImageInfo = &image_infos[binding - starting_binding];
            }
            
            vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
        }
        
        void initialize_object_descriptor_sets() {
            // Allocate descriptor set 1 for per-object bindings
            VkDescriptorSetLayoutBinding bindings[] {
                // Object transforms (model, normal)
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
                
                // Object material settings (Phong)
                create_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            };
            
            // Initialize the descriptor set layout
            VkDescriptorSetLayoutCreateInfo layout_create_info { };
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
            layout_create_info.pBindings = bindings;
            if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &object_descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // Global camera uniforms + global light array
            std::size_t offset = align_to_device_boundary(physical_device, sizeof(GlobalUniforms)) + align_to_device_boundary(physical_device, sizeof(Scene::Light) * scene.lights.size());
            
            std::size_t num_objects = scene.objects.size();
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
                
                VkWriteDescriptorSet descriptor_writes[2] { };
                VkDescriptorBufferInfo buffer_infos[2] { };
                
                buffer_infos[0].buffer = uniform_buffer;
                buffer_infos[0].offset = offset;
                buffer_infos[0].range = sizeof(ObjectUniforms);
                
                descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[0].dstSet = object_descriptor_sets[i];
                descriptor_writes[0].dstBinding = 0;
                descriptor_writes[0].dstArrayElement = 0;
                descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[0].descriptorCount = 1;
                descriptor_writes[0].pBufferInfo = &buffer_infos[0];
                
                offset += align_to_device_boundary(physical_device, buffer_infos[0].range);
             
                buffer_infos[1].buffer = uniform_buffer;
                buffer_infos[1].offset = offset;
                buffer_infos[1].range = sizeof(PhongUniforms);
                
                descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[1].dstSet = object_descriptor_sets[i];
                descriptor_writes[1].dstBinding = 1;
                descriptor_writes[1].dstArrayElement = 0;
                descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[1].descriptorCount = 1;
                descriptor_writes[1].pBufferInfo = &buffer_infos[1];
                
                offset += align_to_device_boundary(physical_device, buffer_infos[1].range);
                
                vkUpdateDescriptorSets(device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
            }
        }

        void destroy_descriptor_sets() {
            // Destroy descriptor set layouts
            vkDestroyDescriptorSetLayout(device, global_descriptor_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(device, object_descriptor_set_layout, nullptr);
            
            // Descriptor sets get cleaned up alongside the descriptor pool
        }
        
        void initialize_buffers() {
            models.emplace_back(load_obj("assets/models/cube.obj"));
            models.emplace_back(load_obj("assets/models/knight.obj"));
            float box_size = 3.0f;
            float height = 2.0f;
            float thickness = 0.05f;
            
//            // Walls
//            Scene::Object& right = scene.objects.emplace_back();
//            right.model = 0;
//            right.ambient = glm::vec3(0.1f);
//            right.diffuse = glm::vec3(205,92,92) / glm::vec3(255); // red
//            right.specular = glm::vec3(0.0f);
//            right.specular_exponent = 0.0f;
//            right.transform = Transform(glm::vec3(box_size, height, 0), glm::vec3(thickness, box_size, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
//
//            Scene::Object& left = scene.objects.emplace_back();
//            left.model = 0;
//            left.ambient = glm::vec3(0.1f);
////            left.diffuse = glm::vec3(95, 158, 160) / glm::vec3(255); // green
//            left.diffuse = glm::vec3(46,139,87) / glm::vec3(255); // green
//            left.specular = glm::vec3(0.0f);
//            left.specular_exponent = 0.0f;
//            left.transform = Transform(glm::vec3(-box_size, height, 0), glm::vec3(thickness, box_size, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
//
//            Scene::Object& back = scene.objects.emplace_back();
//            back.model = 0;
//            back.ambient = glm::vec3(0.1f);
//            back.diffuse = glm::vec3(70,130,180) / glm::vec3(255); // blue
//            back.specular = glm::vec3(0.0f);
//            back.specular_exponent = 0.0f;
//            back.transform = Transform(glm::vec3(0, height, -box_size), glm::vec3(box_size, box_size, thickness), glm::vec3(0.0f, 0.0f, 0.0f));
//
//            Scene::Object& ceiling = scene.objects.emplace_back();
//            ceiling.model = 0;
//            ceiling.ambient = glm::vec3(0.1f);
//            ceiling.diffuse = glm::vec3(255,235,205) / glm::vec3(255); // offwhite
//            ceiling.specular = glm::vec3(0.0f);
//            ceiling.specular_exponent = 0.0f;
//            ceiling.transform = Transform(glm::vec3(0, box_size + height, 0), glm::vec3(box_size, thickness, box_size), glm::vec3(0.0f, 0.0f, 0.0f));

            Scene::Object& floor = scene.objects.emplace_back();
            floor.model = 0;
            floor.ambient = glm::vec3(0.1f);
            floor.diffuse = glm::vec3(255,235,205) / glm::vec3(255); // offwhite
            floor.specular = glm::vec3(0.0f);
            floor.specular_exponent = 0.0f;
            floor.transform = Transform(glm::vec3(0, -box_size + height, 0), glm::vec3(box_size, thickness, box_size), glm::vec3(0.0f, 0.0f, 0.0f));
            
            // Center model
            Scene::Object& knight = scene.objects.emplace_back();
            knight.model = 1;
            knight.ambient = glm::vec3(0.3f);
            knight.diffuse = glm::vec3(0.8f); // offwhite
            knight.specular = glm::vec3(0.0f);
            knight.specular_exponent = 0.0f;
            knight.transform = Transform(glm::vec3(0, 0.5f, 0), glm::vec3(1.5f), glm::vec3(0.0f, 50.0f, 0.0f));
            
            // This sample only uses vertex position and normal
            std::size_t vertex_buffer_size = 0u;
            std::size_t index_buffer_size = 0u;
            
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
        
        void initialize_lights() {
            glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.01f, 100.0f);
            projection[1][1] *= -1;
            
            {
                Scene::Light& light = scene.lights.emplace_back();
                light.position = glm::vec3(5.0f);
                glm::mat4 view = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                light.transform = projection * view;
            }
            
            {
                Scene::Light& light = scene.lights.emplace_back();
                light.position = glm::vec3(-5.0f, 5.0f, -5.0f);
                glm::mat4 view = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                light.transform = projection * view;
            }
        }
        
        void initialize_samplers() {
            VkSamplerCreateInfo color_sampler_create_info { };
            color_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            color_sampler_create_info.magFilter = VK_FILTER_LINEAR;
            color_sampler_create_info.minFilter = VK_FILTER_LINEAR;
            color_sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            color_sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            color_sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            
            if (enabled_physical_device_features.samplerAnisotropy) {
                color_sampler_create_info.anisotropyEnable = VK_TRUE;
                color_sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
            }
            else {
                color_sampler_create_info.anisotropyEnable = VK_FALSE;
                color_sampler_create_info.maxAnisotropy = 1.0f;
            }
        
            color_sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            color_sampler_create_info.unnormalizedCoordinates = VK_FALSE;
            color_sampler_create_info.compareEnable = VK_FALSE;
            color_sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
            color_sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            color_sampler_create_info.mipLodBias = 0.0f;
            color_sampler_create_info.minLod = 0.0f;
            color_sampler_create_info.maxLod = 1.0f;
            
            if (vkCreateSampler(device, &color_sampler_create_info, nullptr, &color_sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create color sampler!");
            }
            
            VkSamplerCreateInfo depth_sampler_create_info { };
            depth_sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            depth_sampler_create_info.magFilter = VK_FILTER_LINEAR;
            depth_sampler_create_info.minFilter = VK_FILTER_LINEAR;
            depth_sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            depth_sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            depth_sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            
            if (enabled_physical_device_features.samplerAnisotropy) {
                depth_sampler_create_info.anisotropyEnable = VK_TRUE;
                depth_sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
            }
            else {
                depth_sampler_create_info.anisotropyEnable = VK_FALSE;
                depth_sampler_create_info.maxAnisotropy = 1.0f;
            }
        
            depth_sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            depth_sampler_create_info.unnormalizedCoordinates = VK_FALSE;
            depth_sampler_create_info.compareEnable = VK_FALSE;
            depth_sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
            depth_sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            depth_sampler_create_info.mipLodBias = 0.0f;
            depth_sampler_create_info.minLod = 0.0f;
            depth_sampler_create_info.maxLod = 1.0f;
            
            if (vkCreateSampler(device, &depth_sampler_create_info, nullptr, &depth_sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create depth sampler!");
            }
        }
        
        void initialize_uniform_buffer() {
            // Globals (camera + lights) + per object (transform + material) * num objects
            std::size_t uniform_buffer_size = align_to_device_boundary(physical_device, sizeof(GlobalUniforms)) + align_to_device_boundary(physical_device, sizeof(Scene::Light) * scene.lights.size()) + (align_to_device_boundary(physical_device, sizeof(ObjectUniforms)) + align_to_device_boundary(physical_device, sizeof(PhongUniforms))) * scene.objects.size();
            
            create_buffer(physical_device, device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
            vkMapMemory(device, uniform_buffer_memory, 0, uniform_buffer_size, 0, &uniform_buffer_mapped);
        }
        
        void update_object_uniform_buffers(unsigned id) {
//            Scene::Object& object = scene.objects[id];
//            std::size_t offset = align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2 + sizeof(glm::vec4)) + (align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2) + align_to_device_boundary(physical_device, sizeof(glm::vec4) * 3 + 4)) * id;
//
//            struct ObjectData {
//                glm::mat4 model;
//                glm::mat4 normal;
//            };
//            ObjectData object_data { };
//            object_data.model = object.transform.get_matrix();
//            object_data.normal = glm::transpose(glm::inverse(object_data.model));
//
//            memcpy((void*)((const char*) (uniform_buffer_mapped) + offset), &object_data, sizeof(ObjectData));
//            offset += align_to_device_boundary(physical_device, sizeof(glm::mat4) * 2);
//
//            struct MaterialData {
//                // Must be aligned to vec4
//                glm::vec4 ambient;
//                glm::vec4 diffuse;
//                glm::vec4 specular;
//                float exponent;
//            };
//            MaterialData material_data { };
//            material_data.ambient = glm::vec4(object.ambient, 1.0f);
//            material_data.diffuse = glm::vec4(object.diffuse, 1.0f);
//            material_data.specular = glm::vec4(object.specular, 1.0f);
//            material_data.exponent = object.specular_exponent;
//
//            memcpy((void*)((const char*) (uniform_buffer_mapped) + offset), &material_data, sizeof(MaterialData));
//            offset += align_to_device_boundary(physical_device, sizeof(glm::vec4) * 3 + 4);
        }
        
        void update_uniform_buffers() {
            std::size_t offset = 0u;
            
            // set 0 binding 0
            {
                GlobalUniforms uniforms { };
                uniforms.view = camera.get_view_matrix();
                uniforms.projection = camera.get_projection_matrix();
                uniforms.camera_position = camera.get_position();
                uniforms.debug_view = 0;
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &uniforms, sizeof(uniforms));
                offset += align_to_device_boundary(physical_device, sizeof(GlobalUniforms));
            }
            
            // set 0 binding 1
            {
                std::size_t light_uniform_block_size = sizeof(Scene::Light);
                
                // Only set lighting data for active lights
                // LIGHT_COUNT represents the maximum supported number of lights
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), scene.lights.data(), light_uniform_block_size * scene.lights.size());
                offset += align_to_device_boundary(physical_device, light_uniform_block_size * scene.lights.size());
            }
            
            for (Scene::Object& object : scene.objects) {
                Transform& transform = object.transform;
                
                // Vertex
                // set 1 binding 0
                ObjectUniforms vertex { };
                vertex.model = transform.get_matrix();
                vertex.normal = glm::transpose(glm::inverse(vertex.model));
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &vertex, sizeof(ObjectUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(ObjectUniforms));
                
                // Fragment
                // set 1 binding 1
                PhongUniforms fragment { };
                fragment.ambient = glm::vec4(object.ambient, 1.0f);
                fragment.diffuse = glm::vec4(object.diffuse, 1.0f);
                fragment.specular = glm::vec4(object.specular, 1.0f);
                fragment.specular_exponent = object.specular_exponent;
                fragment.flat_shaded = (int) object.flat_shaded;
                
                memcpy((void*)(((const char*) uniform_buffer_mapped) + offset), &fragment, sizeof(PhongUniforms));
                offset += align_to_device_boundary(physical_device, sizeof(PhongUniforms));
            }
        }
        
        void destroy_uniform_buffer() {
            vkFreeMemory(device, uniform_buffer_memory, nullptr);
            vkDestroyBuffer(device, uniform_buffer, nullptr);
        }
        
        void on_key_pressed(int key) override {
//            if (key == GLFW_KEY_1) {
//                debug_view = OUTPUT;
//            }
//            else if (key == GLFW_KEY_2) {
//                debug_view = POSITION;
//            }
//            else if (key == GLFW_KEY_3) {
//                debug_view = NORMAL;
//            }
//            else if (key == GLFW_KEY_4) {
//                debug_view = AMBIENT;
//            }
//            else if (key == GLFW_KEY_5) {
//                debug_view = DIFFUSE;
//            }
//            else if (key == GLFW_KEY_6) {
//                debug_view = SPECULAR;
//            }
//            else if (key == GLFW_KEY_7) {
//                debug_view = DEPTH;
//            }
        }
        
};

DEFINE_SAMPLE_MAIN(ShadowMapping);
