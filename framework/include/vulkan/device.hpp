
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
        
                    #if defined(VK_KHR_acceleration_structure)
                        struct AccelerationStructureProperties {
                            operator VkPhysicalDeviceAccelerationStructurePropertiesKHR() const;
                
                            std::uint64_t max_geometry_count;
                            std::uint64_t max_instance_count;
                            std::uint64_t max_primitive_count;
                
                            std::uint32_t max_per_stage_descriptor_acceleration_structures;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_acceleration_structures;
                
                            std::uint32_t max_descriptor_set_acceleration_structures;
                            std::uint32_t max_descriptor_set_update_after_bind_acceleration_structures;
                
                            std::uint32_t min_acceleration_structure_scratch_offset_alignment;
                        } acceleration_structure_properties;
                    #endif
        
                    #if defined(VK_EXT_blend_operation_advanced)
                        struct BlendOperationAdvancedProperties {
                            operator VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT() const;
                
                            std::uint32_t advanced_blend_max_color_attachments;
                            bool advanced_blend_independent_blend;
                            bool advanced_blend_non_premultiplied_src_color;
                            bool advanced_blend_non_premultiplied_dst_color;
                            bool advanced_blend_correlated_overlap;
                            bool advanced_blend_all_operations;
                        } blend_operation_advanced_properties;
                    #endif
        
                    #if defined(VK_EXT_conservative_rasterization)
                        struct ConservativeRasterizationProperties {
                            operator VkPhysicalDeviceConservativeRasterizationPropertiesEXT() const;
                
                            float primitive_overestimation_size;
                            float max_extra_primitive_overestimation_size;
                            float extra_primitive_overestimation_size_granularity;
                
                            bool primitive_underestimation;
                            bool conservative_point_and_line_rasterization;
                            bool degenerate_triangles_rasterized;
                            bool degenerate_lines_rasterized;
                            bool fully_covered_fragment_shader_input_variable;
                            bool conservative_rasterization_post_depth_coverage;
                        } conservative_rasterization_properties;
                    #endif
        
                    #if defined(VK_NV_cooperative_matrix)
                        struct CooperativeMatrixProperties {
                            operator VkPhysicalDeviceCooperativeMatrixPropertiesNV() const;
                            
                            VkShaderStageFlags cooperative_matrix_supported_stages;
                        } cooperative_matrix_properties;
                    #endif
        
                    #if defined(VK_NV_copy_memory_indirect)
                        struct CopyMemoryIndirectProperties {
                            operator VkPhysicalDeviceCopyMemoryIndirectPropertiesNV() const;
                            
                            VkQueueFlags supported_queues;
                        } copy_memory_indirect_properties;
                    #endif
        
                    #if defined(VK_EXT_custom_border_color)
                        struct CustomBorderColorProperties {
                            operator VkPhysicalDeviceCustomBorderColorPropertiesEXT() const;
                            
                            std::uint32_t max_custom_border_color_samplers;
                        } custom_border_color_properties;
                    #endif
                    
                    #if defined(VK_KHR_depth_stencil_resolve)
                        struct DepthStencilResolveProperties {
                            operator VkPhysicalDeviceDepthStencilResolvePropertiesKHR() const;
                        
                            VkResolveModeFlags supported_depth_resolve_modes;
                            VkResolveModeFlags supported_stencil_resolve_modes;
                            bool independent_resolve_none;
                            bool independent_resolve;
                        } depth_stencil_resolve_properties;
                    #endif
        
                    #if defined(VK_EXT_descriptor_indexing)
                        struct DescriptorIndexingProperties {
                            operator VkPhysicalDeviceDescriptorIndexingPropertiesEXT() const;
                
                            std::uint32_t max_update_after_bind_descriptors_in_all_pools;
                
                            bool shader_uniform_buffer_array_non_uniform_indexing_native;
                            bool shader_sampled_image_array_non_uniform_indexing_native;
                            bool shader_storage_buffer_array_non_uniform_indexing_native;
                            bool shader_storage_image_array_non_uniform_indexing_native;
                            bool shader_input_attachment_array_non_uniform_indexing_native;
                
                            bool robust_buffer_access_update_after_bind;
                            bool quad_divergent_implicit_lod;
                
                            std::uint32_t max_per_stage_descriptor_update_after_bind_samplers;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_uniform_buffers;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_storage_buffers;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_sampled_images;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_storage_images;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_input_attachments;
                
                            std::uint32_t max_per_stage_update_after_bind_resources;
                
                            std::uint32_t max_descriptor_set_update_after_bind_samplers;
                            std::uint32_t max_descriptor_set_update_after_bind_uniform_buffers;
                            std::uint32_t max_descriptor_set_update_after_bind_uniform_buffers_dynamic;
                            std::uint32_t max_descriptor_set_update_after_bind_storage_buffers;
                            std::uint32_t max_descriptor_set_update_after_bind_storage_buffers_dynamic;
                            std::uint32_t max_descriptor_set_update_after_bind_sampled_images;
                            std::uint32_t max_descriptor_set_update_after_bind_storage_images;
                            std::uint32_t max_descriptor_set_update_after_bind_input_attachments;
                        } descriptor_indexing_properties;
                    #endif
        
                    #if defined(VK_NV_device_generated_commands)
                        struct DeviceGeneratedCommandProperties {
                            operator VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV() const;
    
                            std::uint32_t max_graphics_shader_group_count;
    
                            std::uint32_t max_indirect_sequence_count;
                            std::uint32_t max_indirect_commands_token_count;
                            std::uint32_t max_indirect_commands_stream_count;
                            std::uint32_t max_indirect_commands_stream_stride;
                            std::uint32_t min_indirect_commands_buffer_offset_alignment;
    
                            std::uint32_t min_sequences_count_buffer_offset_alignment;
                            std::uint32_t min_sequences_index_buffer_offset_alignment;
                        } device_generated_command_properties;
                    #endif
        
                    #if defined(VK_EXT_discard_rectangles)
                        struct DiscardRectangleProperties {
                            operator VkPhysicalDeviceDiscardRectanglePropertiesEXT() const;
                            
                            std::uint32_t max_discard_rectangles;
                        } discard_rectangle_properties;
                    #endif
        
                    #if defined(VK_KHR_driver_properties)
                        struct DriverProperties {
                            operator VkPhysicalDeviceDriverPropertiesKHR() const;
    
                            VkDriverId driver_id;
                            std::string driver_name;
                            std::string driver_info;
                            VkConformanceVersion conformance_version;
                        } driver_properties;
                    #endif
        
                    #if defined(VK_EXT_physical_device_drm)
                        struct DeviceDRMProperties {
                            operator VkPhysicalDeviceDrmPropertiesEXT() const;
                            
                            bool has_primary;
                            std::uint64_t primary_major;
                            std::uint64_t primary_minor;
                            
                            bool has_render;
                            std::uint64_t render_major;
                            std::uint64_t render_minor;
                        } device_drm_properties;
                    #endif
        
                    #if defined(VK_EXT_extended_dynamic_state3)
                        struct ExtendedDynamicStateProperties3 {
                            operator VkPhysicalDeviceExtendedDynamicState3PropertiesEXT() const;
                            
                            bool dynamic_primitive_topology_unrestricted;
                        } extended_dynamic_state_properties3;
                    #endif
        
                    #if defined(VK_EXT_external_memory_host)
                        struct ExternalMemoryHostProperties {
                            operator VkPhysicalDeviceExternalMemoryHostPropertiesEXT() const;
                            
                            bool min_imported_host_pointer_alignment;
                        } external_memory_host_properties;
                    #endif
        
                    #if defined(VK_KHR_shader_float_controls)
                        struct FloatControlsProperties {
                            operator VkPhysicalDeviceFloatControlsPropertiesKHR() const;
    
                            VkShaderFloatControlsIndependence denorm_behavior_independence;
                            VkShaderFloatControlsIndependence rounding_mode_independence;
                            
                            bool shader_signed_zero_inf_nan_preserve_float16;
                            bool shader_signed_zero_inf_nan_preserve_float32;
                            bool shader_signed_zero_inf_nan_preserve_float64;
                            
                            bool shader_denorm_preserve_float16;
                            bool shader_denorm_preserve_float32;
                            bool shader_denorm_preserve_float64;
                            
                            bool shader_denorm_flush_to_zero_float16;
                            bool shader_denorm_flush_to_zero_float32;
                            bool shader_denorm_flush_to_zero_float64;
                            
                            bool shader_rounding_mode_rtef_float16;
                            bool shader_rounding_mode_rtef_float32;
                            bool shader_rounding_mode_rtef_float64;
                            
                            bool shader_rounding_mode_rtzf_float16;
                            bool shader_rounding_mode_rtzf_float32;
                            bool shader_rounding_mode_rtzf_float64;
                        } float_controls_properties;
                    #endif
        
                    #if defined(VK_EXT_fragment_density_map)
                        struct FragmentDensityMapProperties {
                            operator VkPhysicalDeviceFragmentDensityMapPropertiesEXT() const;
    
                            VkExtent2D min_fragment_density_texel_size;
                            VkExtent2D max_fragment_density_texel_size;
                            bool fragment_density_invocations;
                        } fragment_density_map_properties;
                    #endif
                    
                    #if defined(VK_EXT_fragment_density_map2)
                        struct FragmentDensityMapProperties2 {
                            operator VkPhysicalDeviceFragmentDensityMap2PropertiesEXT() const;
    
                            bool subsampled_loads;
                            bool subsampled_coarse_reconstruction_early_access;
                            std::uint32_t max_subsampled_array_layers;
                            std::uint32_t max_descriptor_set_subsampled_samplers;
                        } fragment_density_map_properties2;
                    #endif
        
                    #if defined(VK_QCOM_fragment_density_map_offset)
                        struct FragmentDensityMapOffsetProperties {
                            operator VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM() const;
                            
                            VkExtent2D fragment_density_offset_granularity;
                        } fragment_density_map_offset_properties;
                    #endif
        
                    #if defined(VK_KHR_fragment_shader_barycentric)
                        struct FragmentShaderBarycentricProperties {
                            operator VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR() const;
                            
                            bool tri_strip_vertex_order_independent_of_provoking_vertex;
                        } fragment_shader_barycentric_properties;
                    #endif
        
                    #if defined(VK_NV_fragment_shading_rate_enums)
                        struct FragmentShadingRateEnumsProperties {
                            operator VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV() const;
                            
                            VkSampleCountFlagBits max_fragment_shader_rate_invocation_count;
                        } fragment_shading_rate_enums_properties;
                    #endif
        
                    #if defined(VK_KHR_fragment_shading_rate)
                        struct FragmentShadingRateProperties {
                            operator VkPhysicalDeviceFragmentShadingRatePropertiesKHR() const;
                            
                            VkExtent2D min_fragment_shading_rate_attachment_texel_size;
                            VkExtent2D max_fragment_shading_rate_attachment_texel_size;
                            std::uint32_t max_fragment_shading_rate_attachment_texel_size_aspect_ratio;
                        } fragment_shading_rate_properties;
                    #endif
        
                    #if defined(VK_EXT_graphics_pipeline_library)
                        struct GraphicsPipelineLibraryProperties {
                            operator VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT() const;
                            
                            bool graphics_pipeline_library_fast_linking;
                            bool graphics_pipeline_library_independent_interpolation_decoration;
                        } graphics_pipeline_library_properties;
                    #endif
        
                    #if defined(VK_KHR_external_memory_capabilities)
                        struct DeviceIDProperties {
                            operator VkPhysicalDeviceIDPropertiesKHR() const;
                            
                            std::uint8_t device_uuid[VK_UUID_SIZE];
                            std::uint8_t driver_uuid[VK_UUID_SIZE];
                            std::uint8_t device_luid[VK_LUID_SIZE];
                            std::uint32_t node_mask;
                            bool device_luid_valid;
                        } device_id_properties;
                    #endif
        
                    #if defined(VK_QCOM_image_processing)
                        struct ImageProcessingProperties {
                            operator VkPhysicalDeviceImageProcessingPropertiesQCOM() const;
                            
                            std::uint32_t max_weight_filter_phases;
                            VkExtent2D max_weight_filter_dimension;
                            VkExtent2D max_block_match_region;
                            VkExtent2D max_block_filter_size;
                        } image_processing_properties;
                    #endif
        
                    #if defined(VK_EXT_inline_uniform_block)
                        struct InlineUniformBlockProperties {
                            operator VkPhysicalDeviceInlineUniformBlockPropertiesEXT() const;
                            
                            std::uint32_t max_inline_uniform_block_size;
                            std::uint32_t max_per_stage_descriptor_inline_uniform_blocks;
                            std::uint32_t max_per_stage_descriptor_update_after_bind_inline_uniform_blocks;
                            std::uint32_t max_descriptor_set_inline_uniform_blocks;
                            std::uint32_t max_descriptor_set_update_after_bind_inline_uniform_blocks;
                            
                        } inline_uniform_block_properties;
                    #endif
        
                    #if defined(VK_EXT_line_rasterization)
                        struct LineRasterizationProperties {
                            operator VkPhysicalDeviceLineRasterizationPropertiesEXT() const;
                            
                            std::uint32_t line_sub_pixel_precision_bits;
                        } line_rasterization_properties;
                    #endif
        
                    #if defined(VK_KHR_maintenance3)
                        struct MaintenanceProperties3 {
                            operator VkPhysicalDeviceMaintenance3PropertiesKHR() const;
                            
                            std::uint32_t max_per_set_descriptors;
                            VkDeviceSize max_memory_allocation_size;
                        } maintenance_properties3;
                    #endif
        
                    #if defined(VK_KHR_maintenance4)
                        struct MaintenanceProperties4 {
                            operator VkPhysicalDeviceMaintenance4PropertiesKHR() const;
                            
                            VkDeviceSize max_buffer_size;
                        } maintenance_properties4;
                    #endif
        
                    #if defined(VK_NV_memory_decompression)
                        struct MemoryDecompressionProperties {
                            operator VkPhysicalDeviceMemoryDecompressionPropertiesNV() const;
                            
                            VkMemoryDecompressionMethodFlagsNV decompression_methods;
                            std::uint64_t max_decompression_indirect_count;
                        } memory_decompression_properties;
                    #endif
                        
                        // TODO:
                        // VkPhysicalDeviceMeshShaderPropertiesEXT
                        // VkPhysicalDeviceMeshShaderPropertiesNV
                        // VkPhysicalDeviceMultiDrawPropertiesEXT
                        // VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX
                        // VkPhysicalDeviceMultiviewProperties
                        // VkPhysicalDeviceOpacityMicromapPropertiesEXT
                        // VkPhysicalDeviceOpticalFlowPropertiesNV
                        // VkPhysicalDevicePCIBusInfoPropertiesEXT
                        // VkPhysicalDevicePerformanceQueryPropertiesKHR
                        // VkPhysicalDevicePipelineRobustnessPropertiesEXT
                        // VkPhysicalDevicePointClippingProperties
                        // VkPhysicalDevicePortabilitySubsetPropertiesKHR
                        // VkPhysicalDeviceProtectedMemoryProperties
                        // VkPhysicalDeviceProvokingVertexPropertiesEXT
                        // VkPhysicalDevicePushDescriptorPropertiesKHR
                        // VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV
                        // VkPhysicalDeviceRayTracingPipelinePropertiesKHR
                        // VkPhysicalDeviceRayTracingPropertiesNV
                        // VkPhysicalDeviceRobustness2PropertiesEXT
                        // VkPhysicalDeviceSampleLocationsPropertiesEXT
                        // VkPhysicalDeviceSamplerFilterMinmaxProperties
                        // VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM
                        // VkPhysicalDeviceShaderCoreProperties2AMD
                        // VkPhysicalDeviceShaderCorePropertiesAMD
                        // VkPhysicalDeviceShaderIntegerDotProductProperties
                        // VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT
                        // VkPhysicalDeviceShaderSMBuiltinsPropertiesNV
                        // VkPhysicalDeviceShadingRateImagePropertiesNV
                        // VkPhysicalDeviceSubgroupProperties
                        // VkPhysicalDeviceSubgroupSizeControlProperties
                        // VkPhysicalDeviceSubpassShadingPropertiesHUAWEI
                        // VkPhysicalDeviceTexelBufferAlignmentProperties
                        // VkPhysicalDeviceTimelineSemaphoreProperties
                        // VkPhysicalDeviceTransformFeedbackPropertiesEXT
                        // VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT
                        // VkPhysicalDeviceVulkan11Properties
                        // VkPhysicalDeviceVulkan12Properties
                        // VkPhysicalDeviceVulkan13Properties
                    
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
                        std::uint32_t              maxImageDimension1D;
                        std::uint32_t              maxImageDimension2D;
                        std::uint32_t              maxImageDimension3D;
                        std::uint32_t              maxImageDimensionCube;
                        std::uint32_t              maxImageArrayLayers;
                        std::uint32_t              maxTexelBufferElements;
                        std::uint32_t              maxUniformBufferRange;
                        std::uint32_t              maxStorageBufferRange;
                        std::uint32_t              maxPushConstantsSize;
                        std::uint32_t              maxMemoryAllocationCount;
                        std::uint32_t              maxSamplerAllocationCount;
                        std:: uint64_t          bufferImageGranularity;
                        std::uint64_t          sparseAddressSpaceSize;
                        std::uint32_t              maxBoundDescriptorSets;
                        std::uint32_t              maxPerStageDescriptorSamplers;
                        uint32_t              maxPerStageDescriptorUniformBuffers;
                        uint32_t              maxPerStageDescriptorStorageBuffers;
                        uint32_t              maxPerStageDescriptorSampledImages;
                        uint32_t              maxPerStageDescriptorStorageImages;
                        uint32_t              maxPerStageDescriptorInputAttachments;
                        uint32_t              maxPerStageResources;
                        uint32_t              maxDescriptorSetSamplers;
                        uint32_t              maxDescriptorSetUniformBuffers;
                        uint32_t              maxDescriptorSetUniformBuffersDynamic;
                        uint32_t              maxDescriptorSetStorageBuffers;
                        uint32_t              maxDescriptorSetStorageBuffersDynamic;
                        uint32_t              maxDescriptorSetSampledImages;
                        uint32_t              maxDescriptorSetStorageImages;
                        uint32_t              maxDescriptorSetInputAttachments;
                        uint32_t              maxVertexInputAttributes;
                        uint32_t              maxVertexInputBindings;
                        uint32_t              maxVertexInputAttributeOffset;
                        uint32_t              maxVertexInputBindingStride;
                        uint32_t              maxVertexOutputComponents;
                        uint32_t              maxTessellationGenerationLevel;
                        uint32_t              maxTessellationPatchSize;
                        uint32_t              maxTessellationControlPerVertexInputComponents;
                        uint32_t              maxTessellationControlPerVertexOutputComponents;
                        uint32_t              maxTessellationControlPerPatchOutputComponents;
                        uint32_t              maxTessellationControlTotalOutputComponents;
                        uint32_t              maxTessellationEvaluationInputComponents;
                        uint32_t              maxTessellationEvaluationOutputComponents;
                        uint32_t              maxGeometryShaderInvocations;
                        uint32_t              maxGeometryInputComponents;
                        uint32_t              maxGeometryOutputComponents;
                        uint32_t              maxGeometryOutputVertices;
                        uint32_t              maxGeometryTotalOutputComponents;
                        uint32_t              maxFragmentInputComponents;
                        uint32_t              maxFragmentOutputAttachments;
                        uint32_t              maxFragmentDualSrcAttachments;
                        uint32_t              maxFragmentCombinedOutputResources;
                        uint32_t              maxComputeSharedMemorySize;
                        uint32_t              maxComputeWorkGroupCount[3];
                        uint32_t              maxComputeWorkGroupInvocations;
                        uint32_t              maxComputeWorkGroupSize[3];
                        uint32_t              subPixelPrecisionBits;
                        uint32_t              subTexelPrecisionBits;
                        uint32_t              mipmapPrecisionBits;
                        uint32_t              maxDrawIndexedIndexValue;
                        uint32_t              maxDrawIndirectCount;
                        float                 maxSamplerLodBias;
                        float                 maxSamplerAnisotropy;
                        uint32_t              maxViewports;
                        uint32_t              maxViewportDimensions[2];
                        float                 viewportBoundsRange[2];
                        uint32_t              viewportSubPixelBits;
                        size_t                minMemoryMapAlignment;
                        uint64_t          minTexelBufferOffsetAlignment;
                        uint64_t          minUniformBufferOffsetAlignment;
                        uint64_t          minStorageBufferOffsetAlignment;
                        int32_t               minTexelOffset;
                        uint32_t              maxTexelOffset;
                        int32_t               minTexelGatherOffset;
                        uint32_t              maxTexelGatherOffset;
                        float                 minInterpolationOffset;
                        float                 maxInterpolationOffset;
                        uint32_t              subPixelInterpolationOffsetBits;
                        uint32_t              maxFramebufferWidth;
                        uint32_t              maxFramebufferHeight;
                        uint32_t              maxFramebufferLayers;
                        VkSampleCountFlags    framebufferColorSampleCounts;
                        VkSampleCountFlags    framebufferDepthSampleCounts;
                        VkSampleCountFlags    framebufferStencilSampleCounts;
                        VkSampleCountFlags    framebufferNoAttachmentsSampleCounts;
                        uint32_t              maxColorAttachments;
                        VkSampleCountFlags    sampledImageColorSampleCounts;
                        VkSampleCountFlags    sampledImageIntegerSampleCounts;
                        VkSampleCountFlags    sampledImageDepthSampleCounts;
                        VkSampleCountFlags    sampledImageStencilSampleCounts;
                        VkSampleCountFlags    storageImageSampleCounts;
                        uint32_t              maxSampleMaskWords;
                        VkBool32              timestampComputeAndGraphics;
                        float                 timestampPeriod;
                        uint32_t              maxClipDistances;
                        uint32_t              maxCullDistances;
                        uint32_t              maxCombinedClipAndCullDistances;
                        uint32_t              discreteQueuePriorities;
                        float                 pointSizeRange[2];
                        float                 lineWidthRange[2];
                        float                 pointSizeGranularity;
                        float                 lineWidthGranularity;
                        VkBool32              strictLines;
                        VkBool32              standardSampleLocations;
                        VkDeviceSize          optimalBufferCopyOffsetAlignment;
                        VkDeviceSize          optimalBufferCopyRowPitchAlignment;
                        VkDeviceSize          nonCoherentAtomSize;
                    } limits;
                    
                    // Vendor-supplied information.
                    std::uint32_t vendor_id;
                    std::uint32_t device_id;
                    std::array<std::uint8_t, VK_UUID_SIZE> device_uuid;
                    std::array<char, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE> device_name;
                    
                    // Missing:
                    //  - Sparse properties (VkPhysicalDeviceSparseProperties)
                private:
                
                
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
        
            #if defined(VK_VERSION_1_0)
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
        
                #if defined(VK_VERSION_1_1)
                    // vkGetPhysicalDeviceProperties2
                    void (VKAPI_PTR *fp_vk_get_physical_device_properties_2_)(VkPhysicalDevice, VkPhysicalDeviceProperties2*);
                #endif
                
                #if defined(VK_KHR_get_display_properties2)
                    // vkGetPhysicalDeviceProperties2KHR
                    void (VKAPI_PTR *fp_vk_get_physical_device_properties_2_khr_)(VkPhysicalDevice, VkPhysicalDeviceProperties2KHR*);
                #endif
            #endif
    
    };
    
}
