
#pragma once

#include "core/defines.hpp"
#include "vulkan/types.hpp"
#include "vulkan/queue.hpp"

#include <functional>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    
    class VulkanDevice : public ManagedObject<VulkanDevice> {
        public:
            class Builder;
            
            struct Properties {
                enum class Type : u8 {
                    DISCRETE_GPU   = 1u << 0u,
                    INTEGRATED_GPU = 1u << 1u,
                    VIRTUAL_GPU    = 1u << 2u,
                    CPU            = 1u << 3u,
                    OTHER          = 1u << 4u,
                } type;
    
                struct Limits {
                    u32 max_image_dimension_1d;
                    u32 max_image_dimension_2d;
                    u32 max_image_dimension_3d;
                    u32 max_image_dimension_cube;
                    u32 max_image_array_layers;
                    u32 max_texel_buffer_elements;
                    u32 max_uniform_buffer_range;
                    u32 max_storage_buffer_range;
                    u32 max_push_constants_size;
                    u32 max_memory_allocation_count;
                    u32 max_sampler_allocation_count;
                    u32 buffer_image_granularity;
                    u32 sparse_address_space_size;
                    u32 max_bound_descriptor_sets;
                    u32 max_per_stage_descriptor_samplers;
                    u32 max_per_stage_descriptor_uniform_buffers;
                    u32 max_per_stage_descriptor_storage_buffers;
                    u32 max_per_stage_descriptor_sampled_images;
                    u32 max_per_stage_descriptor_storage_images;
                    u32 max_per_stage_descriptor_input_attachments;
                    u32 max_per_stage_resources;
                    u32 max_descriptor_set_samplers;
                    u32 max_descriptor_set_uniform_buffers;
                    u32 max_descriptor_set_uniform_buffers_dynamic;
                    u32 max_descriptor_set_storage_buffers;
                    u32 max_descriptor_set_storage_buffers_dynamic;
                    u32 max_descriptor_set_sampled_images;
                    u32 max_descriptor_set_storage_images;
                    u32 max_descriptor_set_input_attachments;
                    u32 max_vertex_input_attributes;
                    u32 max_vertex_input_bindings;
                    u32 max_vertex_input_attribute_offset;
                    u32 max_vertex_input_binding_stride;
                    u32 max_vertex_output_components;
                    u32 max_tesselation_generation_level;
                    u32 max_tesselation_patch_size;
                    u32 max_tesselation_control_per_vertex_input_components;
                    u32 max_tesselation_control_per_vertex_output_components;
                    u32 max_tesselation_control_per_patch_output_components;
                    u32 max_tesselation_control_total_output_components;
                    u32 max_tesselation_evaluation_input_components;
                    u32 max_tesselation_evaluation_output_components;
                    u32 max_geometry_shader_invocations;
                    u32 max_geometry_input_components;
                    u32 max_geometry_output_components;
                    u32 max_geometry_output_vertices;
                    u32 max_geometry_total_output_components;
                    u32 max_fragment_input_components;
                    u32 max_fragment_output_attachments;
                    u32 max_fragment_dual_src_attachments;
                    u32 max_fragment_combined_output_resources;
                    u32 max_compute_shared_memory_size;
                    u32 max_compute_work_group_count[3];
                    u32 max_compute_work_group_invocations;
                    u32 max_compute_work_group_size[3];
                    u32 subpixel_precision_bits;
                    u32 subtexel_precision_bits;
                    u32 mipmap_precision_bits;
                    u32 max_draw_indexed_index_value;
                    u32 max_draw_indirect_count;
                    float max_sampler_lod_bias;
                    float max_sampler_anisotropy;
                    u32 max_viewports;
                    u32 max_viewport_dimensions[2];
                    float viewport_bounds_range[2];
                    u32 viewport_subpixel_bits;
                    u64 min_memory_map_alignment;
                    u64 min_texel_buffer_offset_alignment;
                    u64 min_uniform_buffer_offset_alignment;
                    u64 min_storage_buffer_offset_alignment;
                    i32 min_texel_offset;
                    u32 max_texel_offset;
                    i32 min_texel_gather_offset;
                    u32 max_texel_gather_offset;
                    float min_interpolation_offset;
                    float max_interpolation_offset;
                    u32 subpixel_interpolation_offset_bits;
                    u32 max_framebuffer_width;
                    u32 max_framebuffer_height;
                    u32 max_framebuffer_layers;
                    VkSampleCountFlags framebuffer_color_sample_counts;
                    VkSampleCountFlags framebuffer_depth_sample_counts;
                    VkSampleCountFlags framebuffer_stencil_sample_counts;
                    VkSampleCountFlags framebuffer_no_attachments_sample_counts;
                    u32 max_color_attachments;
                    VkSampleCountFlags sampled_image_color_sample_counts;
                    VkSampleCountFlags sampled_image_integer_sample_counts;
                    VkSampleCountFlags sample_image_depth_sample_counts;
                    VkSampleCountFlags sample_image_stencil_sample_counts;
                    VkSampleCountFlags storage_image_sample_counts;
                    u32 max_sample_mask_words;
                    bool timestamp_compute_and_graphics;
                    float timestamp_period;
                    u32 max_clip_distances;
                    u32 max_cull_distances;
                    u32 max_combined_clip_and_cull_distances;
                    u32 discrete_queue_priorities;
                    float point_size_range[2];
                    float line_width_range[2];
                    float point_size_granularity;
                    float line_width_granularity;
                    bool strict_lines;
                    bool standard_sample_locations;
                    VkDeviceSize optimal_buffer_copy_offset_alignment;
                    VkDeviceSize optimal_buffer_copy_row_pitch_alignment;
                    VkDeviceSize non_coherent_atom_size;
                } limits;
    
                // Supported Vulkan runtime.
                Version runtime;
    
                // Vendor-supplied information.
                u32 driver_version;
                std::string device_name;
            } properties;
            
            struct Features {
                bool robust_buffer_access;
                bool full_draw_index_uint32;
                bool image_cube_array;
                bool independent_blend;
                bool geometry_shader;
                bool tesselation_shader;
                bool sample_rate_shading;
                bool dual_src_blend;
                bool logic_operations;
                bool multi_draw_indirect;
                bool draw_indirect_first_instance;
                bool depth_clamp;
                bool depth_bias_clamp;
                bool fill_mode_non_solid;
                bool depth_bounds;
                bool wide_lines;
                bool large_points;
                bool alpha_to_one;
                bool multi_viewport;
                bool sampler_anisotropy;
                bool texture_compression_etc2;
                bool texture_compression_astc_ldr;
                bool texture_compression_bc;
                bool occlusion_query_precise;
                bool pipeline_statistics_query;
                bool vertex_pipeline_stores_and_atomics;
                bool fragment_stores_and_atomics;
                bool shader_tesselation_and_geometry_point_size;
                bool shader_image_gather_extended;
                bool shader_storage_image_extended_formats;
                bool shader_storage_multisample;
                bool shader_storage_image_read_without_format;
                bool shader_storage_image_write_without_format;
                bool shader_uniform_buffer_array_dynamic_indexing;
                bool shader_sampled_image_array_dynamic_indexing;
                bool shader_storage_buffer_array_dynamic_indexing;
                bool shader_storage_image_array_dynamic_indexing;
                bool shader_clip_distance;
                bool shader_cull_distance;
                bool shader_float64;
                bool shader_int64;
                bool shader_int16;
                bool shader_resource_residency;
                bool shader_resource_min_lod;
                bool sparse_binding;
                bool sparse_residency_buffer;
                bool sparse_residency_image2d;
                bool sparse_residency_image3d;
                bool sparse_residency_2_samples;
                bool sparse_residency_4_samples;
                bool sparse_residency_8_samples;
                bool sparse_residency_16_samples;
                bool sparse_residency_aliased;
                bool variable_multisample_rate;
                bool inherited_queries;
            } features;
            
            ~VulkanDevice() override;
        
            NODISCARD operator VkPhysicalDevice() const;
            NODISCARD operator VkDevice() const;
        
        private:
            class Data;
        
            // Devices should only be created using VulkanDevice::Builder.
            VulkanDevice();
    };
    
    class VulkanDevice::Builder {
        public:
            explicit Builder(std::shared_ptr<VulkanInstance> instance);
            ~Builder();
            
            NODISCARD std::shared_ptr<VulkanDevice> build();
            
            // Note: any enabled features must be supported by the device.
            Builder& with_features(const Features& features);
            
            // By default, only GRAPHICS operations are required to be supported by the selected device.
            Builder& with_supported_queue(VulkanQueue::Operation operation);
            Builder& with_dedicated_queue(VulkanQueue::Operation operation);
            
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
            
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
            
            Builder& with_debug_name(const char* name);
        
        private:
            std::shared_ptr<VulkanDevice> m_device;
    };
    
}
