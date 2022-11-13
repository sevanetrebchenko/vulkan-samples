
#pragma once

#ifndef VULKAN_SAMPLES_PHYSICAL_DEVICE_HPP
#define VULKAN_SAMPLES_PHYSICAL_DEVICE_HPP

#include "core/defines.hpp"
#include "vulkan/types.hpp"
#include "vulkan/logical_device.hpp"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <functional>
#include <vector>

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    
    class VulkanPhysicalDevice {
        public:
            class Selector;
            
            struct Properties {
                Properties(const VkPhysicalDeviceProperties& properties);
                ~Properties();
                
                enum class Type : std::uint8_t {
                    DISCRETE_GPU   = 1u << 0u,
                    INTEGRATED_GPU = 1u << 1u,
                    VIRTUAL_GPU    = 1u << 2u,
                    CPU            = 1u << 3u,
                    OTHER          = 1u << 4u,
                } type;
                
                struct Limits {
                    Limits(const VkPhysicalDeviceLimits& limits);
                    ~Limits();
                    
                    std::uint32_t max_image_dimension_1d;
                    std::uint32_t max_image_dimension_2d;
                    std::uint32_t max_image_dimension_3d;
                    std::uint32_t max_image_dimension_cube;
                    std::uint32_t max_image_array_layers;
                    std::uint32_t max_texel_buffer_elements;
                    std::uint32_t max_uniform_buffer_range;
                    std::uint32_t max_storage_buffer_range;
                    std::uint32_t max_push_constants_size;
                    std::uint32_t max_memory_allocation_count;
                    std::uint32_t max_sampler_allocation_count;
                    std::uint32_t buffer_image_granularity;
                    std::uint32_t sparse_address_space_size;
                    std::uint32_t max_bound_descriptor_sets;
                    std::uint32_t max_per_stage_descriptor_samplers;
                    std::uint32_t max_per_stage_descriptor_uniform_buffers;
                    std::uint32_t max_per_stage_descriptor_storage_buffers;
                    std::uint32_t max_per_stage_descriptor_sampled_images;
                    std::uint32_t max_per_stage_descriptor_storage_images;
                    std::uint32_t max_per_stage_descriptor_input_attachments;
                    std::uint32_t max_per_stage_resources;
                    std::uint32_t max_descriptor_set_samplers;
                    std::uint32_t max_descriptor_set_uniform_buffers;
                    std::uint32_t max_descriptor_set_uniform_buffers_dynamic;
                    std::uint32_t max_descriptor_set_storage_buffers;
                    std::uint32_t max_descriptor_set_storage_buffers_dynamic;
                    std::uint32_t max_descriptor_set_sampled_images;
                    std::uint32_t max_descriptor_set_storage_images;
                    std::uint32_t max_descriptor_set_input_attachments;
                    std::uint32_t max_vertex_input_attributes;
                    std::uint32_t max_vertex_input_bindings;
                    std::uint32_t max_vertex_input_attribute_offset;
                    std::uint32_t max_vertex_input_binding_stride;
                    std::uint32_t max_vertex_output_components;
                    std::uint32_t max_tesselation_generation_level;
                    std::uint32_t max_tesselation_patch_size;
                    std::uint32_t max_tesselation_control_per_vertex_input_components;
                    std::uint32_t max_tesselation_control_per_vertex_output_components;
                    std::uint32_t max_tesselation_control_per_patch_output_components;
                    std::uint32_t max_tesselation_control_total_output_components;
                    std::uint32_t max_tesselation_evaluation_input_components;
                    std::uint32_t max_tesselation_evaluation_output_components;
                    std::uint32_t max_geometry_shader_invocations;
                    std::uint32_t max_geometry_input_components;
                    std::uint32_t max_geometry_output_components;
                    std::uint32_t max_geometry_output_vertices;
                    std::uint32_t max_geometry_total_output_components;
                    std::uint32_t max_fragment_input_components;
                    std::uint32_t max_fragment_output_attachments;
                    std::uint32_t max_fragment_dual_src_attachments;
                    std::uint32_t max_fragment_combined_output_resources;
                    std::uint32_t max_compute_shared_memory_size;
                    std::uint32_t max_compute_work_group_count[3];
                    std::uint32_t max_compute_work_group_invocations;
                    std::uint32_t max_compute_work_group_size[3];
                    std::uint32_t subpixel_precision_bits;
                    std::uint32_t subtexel_precision_bits;
                    std::uint32_t mipmap_precision_bits;
                    std::uint32_t max_draw_indexed_index_value;
                    std::uint32_t max_draw_indirect_count;
                    float max_sampler_lod_bias;
                    float max_sampler_anisotropy;
                    std::uint32_t max_viewports;
                    std::uint32_t max_viewport_dimensions[2];
                    float viewport_bounds_range[2];
                    std::uint32_t viewport_subpixel_bits;
                    std::size_t min_memory_map_alignment;
                    std::uint64_t min_texel_buffer_offset_alignment;
                    std::uint64_t min_uniform_buffer_offset_alignment;
                    std::uint64_t min_storage_buffer_offset_alignment;
                    std::int32_t min_texel_offset;
                    std::uint32_t max_texel_offset;
                    std::int32_t min_texel_gather_offset;
                    std::uint32_t max_texel_gather_offset;
                    float min_interpolation_offset;
                    float max_interpolation_offset;
                    std::uint32_t subpixel_interpolation_offset_bits;
                    std::uint32_t max_framebuffer_width;
                    std::uint32_t max_framebuffer_height;
                    std::uint32_t max_framebuffer_layers;
                    VkSampleCountFlags framebuffer_color_sample_counts;
                    VkSampleCountFlags framebuffer_depth_sample_counts;
                    VkSampleCountFlags framebuffer_stencil_sample_counts;
                    VkSampleCountFlags framebuffer_no_attachments_sample_counts;
                    std::uint32_t max_color_attachments;
                    VkSampleCountFlags sampled_image_color_sample_counts;
                    VkSampleCountFlags sampled_image_integer_sample_counts;
                    VkSampleCountFlags sample_image_depth_sample_counts;
                    VkSampleCountFlags sample_image_stencil_sample_counts;
                    VkSampleCountFlags storage_image_sample_counts;
                    std::uint32_t max_sample_mask_words;
                    bool timestamp_compute_and_graphics;
                    float timestamp_period;
                    std::uint32_t max_clip_distances;
                    std::uint32_t max_cull_distances;
                    std::uint32_t max_combined_clip_and_cull_distances;
                    std::uint32_t discrete_queue_priorities;
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
                std::uint32_t driver_version;
                std::string device_name;
            } properties;
            
            struct Features {
                Features(const VkPhysicalDeviceFeatures& features);
                ~Features();
                
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
        
            class QueueFamily {
                public:
                    enum class Operation : std::uint8_t {

                    };
                    
                    QueueFamily();
                    ~QueueFamily();
                    
                private:
                    VkQueueFamilyProperties properties_;
                    std::uint8_t index_;
            };
            
            class QueueFamilies {
                public:
                    enum class Operation : std::uint8_t {

                    };
                
                    QueueFamilies(const std::vector<VkQueueFamilyProperties>& queue_families);
                    ~QueueFamilies();
                
                    // Query whether this device has a queue dedicated to a given operation.
                    NODISCARD bool has_dedicated_queue(Operation op) const;
                
                    // Query whether this device has a queue that supports a given operation.
                    NODISCARD bool supports_operation(Operation op) const;
            
                    NODISCARD VulkanDevice::Queue
                    
                private:
                    std::vector<VkQueueFamilyProperties> families_;
                    std::unordered_map<Operation, std::uint8_t> indices_;
            } queue_families;
            
            ~VulkanPhysicalDevice();
            
            VulkanDevice::Builder create_device() const;
        
        private:
            VulkanPhysicalDevice();
            
            VkPhysicalDevice handle_;
            
            std::vector<QueueFamily> queue_families_;
    };
    
    class VulkanPhysicalDevice::Selector {
        public:
            typedef std::function<int (const Properties&, const Features&, const QueueFamilies&)> SelectorFn;
            
            Selector(const VulkanInstance& instance);
            ~Selector();
            
            NODISCARD VulkanPhysicalDevice select();
            
            Selector& with_selector_function(const SelectorFn& selector);
            
            Selector& with_debug_name(const char* name);
            
            // Device extensions.
            Selector& with_enabled_extension(const char* name);
            Selector& with_disabled_extension(const char* name);
            
            // Device validation layers.
            Selector& with_enabled_validation_layer(const char* name);
            Selector& with_disabled_validation_layer(const char* name);
            
            Selector& with_dedicated_queue();
            
        
        private:
            const VulkanInstance& instance_;
            

            SelectorFn selector_;
            const char* name_;
            
            std::vector<const char*> extensions_;
            std::vector<const char*> validation_layers_;
    };
    
}

#endif //VULKAN_SAMPLES_PHYSICAL_DEVICE_HPP
