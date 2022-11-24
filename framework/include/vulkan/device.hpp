
#pragma once

#include "core/defines.hpp"
#include "vulkan/types.hpp"

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
                operator VkPhysicalDeviceProperties() const;
                
                enum class Type : u8 {
                    DISCRETE_GPU   = 1u << 0u,
                    INTEGRATED_GPU = 1u << 1u,
                    VIRTUAL_GPU    = 1u << 2u,
                    CPU            = 1u << 3u,
                    OTHER          = 1u << 4u,
                } type;
    
                struct Limits {
                    operator VkPhysicalDeviceLimits() const;
                    
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
                operator VkPhysicalDeviceFeatures() const;
    
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
            
            class Queue {
                public:
                    enum class Operation : u8 {
                        GRAPHICS = 1u << 0u,
                        COMPUTE  = 1u << 1u,
                        TRANSFER = 1u << 2u
                    };
                
                    NODISCARD bool supports_operation(Operation op) const;
                    NODISCARD bool is_dedicated() const;
                    
                private:
                    friend class VulkanDevice;
                    Queue() {
                    
                    }
                    
                    VkQueue handle_;
                    Operation operations_;
            };
            
            ~VulkanDevice() {
            
            }
        
        private:
            friend class Builder;
            
            VulkanDevice();
            
            VkPhysicalDevice m_device;
            VkDevice m_handle;
            
            std::string m_name;
            
            // Device queues.
//            Queue graphics_queue_; // Graphics and presentation operations.
//            Queue m_presentation_queue;
//            Queue compute_queue_; // Compute operations only.
//            Queue async_compute_queue_;
//            Queue transfer_queue_; // GPU-internal data transfers.
//            Queue async_transfer_queue_; // Data transfers between CPU and GPU.
            
            std::vector<const char*> m_extensions;
            std::vector<const char*> m_validation_layers;
    };
    
    class VulkanDevice::Builder {
        public:
            Builder(std::shared_ptr<VulkanInstance> instance);
            ~Builder();
            
            NODISCARD std::shared_ptr<VulkanDevice> build() const;
            
            // Provides a read-only view into the capabilities of an available physical device on the system.
            // Builder& with_physical_device_selector_function(const SelectorFn& selector);
             Builder& with_physical_device_selector_function(i32 (*selector)(const Properties&, const Features&));
            //Builder& with_physical_device_selector_function(const std::function<int(const Properties&, const Features&)>& selector);
            
            // Enable / disable specific device features.
            // Note: any enabled features must be supported by the device.
            Builder& with_features(const Features& features);
            
            // Device extensions.
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
            
            // Device validation layers.
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
            
            Builder& with_debug_name(const char* name);
        
        private:
            struct GraphicsQueueFamily {
                i32 index;
                u32 num_available_queues;
                bool has_presentation_support;
                bool has_compute_support; // For synchronous compute operations.
                bool has_transfer_support; // For synchronous (inside-GPU) transfer operations.
            };
        
            // Optional for headless devices.
            struct PresentationQueueFamily {
                i32 index;
                u32 num_available_queues;
            };
        
            // Queue family for asynchronous compute operations.
            struct ComputeQueueFamily {
                i32 index;
                u32 num_available_queues;
                bool async;
            };
        
            // Queue family for asynchronous (CPU-GPU) transfer operations.
            struct TransferQueueFamily {
                i32 index;
                u32 num_available_queues;
                bool async;
            };
            
            struct DeviceContext {
                // Physical device.
                VkPhysicalDevice device;
                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;
                
                // Queue families.
                std::vector<VkQueueFamilyProperties> queue_family_properties;
                std::tuple<GraphicsQueueFamily, PresentationQueueFamily, ComputeQueueFamily, TransferQueueFamily> queue_families;
    
                // Logical device.
                VkDevice handle;
                
                std::vector<std::string> messages;
            };
            

            
            NODISCARD bool verify_device_extension_support(VkPhysicalDevice device_handle) const;
            NODISCARD bool verify_device_validation_layer_support(VkPhysicalDevice device_handle) const;
            NODISCARD bool verify_device_properties(const VkPhysicalDeviceProperties& device_properties) const;
            NODISCARD bool verify_device_feature_support(const VkPhysicalDeviceFeatures& device_features) const;
        
            NODISCARD std::tuple<GraphicsQueueFamily, PresentationQueueFamily, ComputeQueueFamily, TransferQueueFamily> select_device_queue_families(VkPhysicalDevice device_handle, const std::vector<VkQueueFamilyProperties>& queue_families) const;
            NODISCARD bool verify_device_queue_family_support(const std::tuple<GraphicsQueueFamily, PresentationQueueFamily, ComputeQueueFamily, TransferQueueFamily>& queue_families) const;
            
            
            // SECTION: Physical device Vulkan functions.
            
            // vkEnumeratePhysicalDevices
            VkResult (VKAPI_PTR* fp_vk_enumerate_physical_devices_)(VkInstance, u32*, VkPhysicalDevice*);
            
            // vkGetPhysicalDeviceProperties
            void (VKAPI_PTR* fp_vk_get_physical_device_properties_)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
            
            // vkGetPhysicalDeviceFeatures
            void (VKAPI_PTR* fp_vk_get_physical_device_features_)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
            
            // vkGetPhysicalDeviceQueueFamilyProperties
            void (VKAPI_PTR* fp_vk_get_physical_device_queue_family_properties_)(VkPhysicalDevice, u32*, VkQueueFamilyProperties*);
            
            // vkEnumerateDeviceExtensionProperties
            VkResult (VKAPI_PTR* fp_vk_enumerate_device_extension_properties_)(VkPhysicalDevice, const char*, u32*, VkExtensionProperties*);
            
            // vkEnumerateDeviceLayerProperties
            VkResult (VKAPI_PTR* fp_vk_enumerate_device_layer_properties_)(VkPhysicalDevice, u32*, VkLayerProperties*);
        
            #if defined(VKS_PLATFORM_WINDOWS)
        
                // vkGetPhysicalDeviceWin32PresentationSupportKHR
                VkBool32 (VKAPI_PTR* m_fp_vk_get_physical_device_win32_presentation_support)(VkPhysicalDevice, u32);
        
            #elif defined(VKS_PLATFORM_LINUX)
        
                // vkGetPhysicalDeviceXcbPresentationSupportKHR
                VkBool32 (VKAPI_PTR* m_fp_vk_get_physical_device_xcb_presentation_support)(VkPhysicalDevice, u32, xcb_connection_t*, xcb_visualid_t);
                
            #endif
            // TODO: Android + Apple
        
        
            // SECTION: Logical device Vulkan functions.
            
            // vkCreateDevice
            VkResult (VKAPI_PTR* fp_vk_create_device_)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*);

            
            bool m_configuration_started;
            
            std::shared_ptr<VulkanInstance> m_instance;
            VulkanDevice m_device;
            
            i32 (*selector_)(const Properties&, const Features&);
            Features requested_features_;
    };
    
}
