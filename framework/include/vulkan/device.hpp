
#pragma once

#include "core/defines.hpp"
#include "vulkan/loader.hpp"
#include "vulkan/types.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <utility>
#include <string>
#include <iostream>

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;

    
    class VulkanDevice {
        public:
            class Builder;
        
        private:
            VkDevice handle_;
    };
    
    class VulkanDevice::Builder {
        public:
        
        private:
        
    };
    
    
    
    class VulkanPhysicalDevice {
        public:
            class Selector;
            
            class Properties {
                public:
                    Properties();
                    ~Properties();
                    
                    // TODO: physical-device-properties-2 branch for VkPhysicalDeviceProperties2(KHR).
                    
                    std::uint32_t api_version;
                    std::uint32_t driver_version;
                    
                    enum class Type : std::uint8_t {
                        INTEGRATED_GPU,
                        DISCRETE_GPU,
                        VIRTUAL_GPU,
                        CPU,
                        OTHER
                    } type;
        
                    struct Limits {
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
                        std::int32_t max_texel_offset;
                        std::int32_t min_texel_gather_offset;
                        std::int32_t max_texel_gather_offset;
                        float min_interpolation_offset;
                        float max_interpolation_offset;
                        std::uint32_t subpixel_interpolation_offset_bits;
                        std::uint32_t max_framebuffer_width;
                        std::uint32_t max_framebuffer_height;
                        std::uint32_t max_framebuffer_layers;
                        VkSampleCountFlagBits framebuffer_color_sample_counts;
                        VkSampleCountFlagBits framebuffer_depth_sample_counts;
                        VkSampleCountFlagBits framebuffer_stencil_sample_counts;
                        VkSampleCountFlagBits framebuffer_no_attachments_sample_counts;
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
                    
                    // Vendor-supplied information.
                    std::uint32_t vendor_id;
                    std::uint32_t device_id;
                    std::string device_name;
            } properties;
            
            class Features {
            
            } features;
        
            VulkanPhysicalDevice() {
            
            }
            ~VulkanPhysicalDevice() {
            
            }
            
            VulkanDevice::Builder create_device() const;
            
        private:
            VkPhysicalDevice handle_;
    };
    
    class VulkanPhysicalDevice::Selector {
        public:
            Selector(VulkanInstance* instance);
            ~Selector();
            
            NODISCARD VulkanPhysicalDevice select();
            
            Selector& with_selector_function(const std::function<int(const Properties&, const Features&)>& selector);
            
            Selector& with_debug_name(const char* name);
            
        private:
            VulkanInstance* instance_;
        
            // vkEnumeratePhysicalDevices
            VkResult (VKAPI_PTR *fp_vk_enumerate_physical_devices_)(VkInstance, std::uint32_t*, VkPhysicalDevice*);
            
            // vkGetPhysicalDeviceProperties
            void (VKAPI_PTR *fp_vk_get_physical_device_properties_)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
            
            // vkGetPhysicalDeviceFeatures
            void (VKAPI_PTR *fp_vk_get_physical_device_features_)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);

            // vkGetPhysicalDeviceQueueFamilyProperties
            void (VKAPI_PTR *fp_vk_get_physical_device_queue_family_properties_)(VkPhysicalDevice, std::uint32_t*, VkQueueFamilyProperties*);
    
            // vkEnumerateDeviceExtensionProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_device_extension_properties_)(VkPhysicalDevice, const char*, std::uint32_t*, VkExtensionProperties*);
    
            // vkEnumerateDeviceLayerProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_device_layer_properties_)(VkPhysicalDevice, std::uint32_t*, VkLayerProperties*);
    };
    
}
