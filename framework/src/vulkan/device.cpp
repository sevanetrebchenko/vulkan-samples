
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

#include <cstring>
#include <iostream>
#include <unordered_set>

namespace vks {

    struct VulkanDevice::Data : public VulkanDevice {
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
    
        struct QueueFamilies {
            static constexpr i32 INVALID_QUEUE_FAMILY_INDEX = -1;
            
            NODISCARD bool is_operation_supported(VulkanQueue::Operation operation) const;
            NODISCARD bool has_dedicated_queue_family(VulkanQueue::Operation operation) const;
            
            GraphicsQueueFamily graphics_queue_family;
            PresentationQueueFamily presentation_queue_family;
            ComputeQueueFamily compute_queue_family;
            TransferQueueFamily transfer_queue_family;
        };
        
        Data(std::shared_ptr<VulkanInstance> instance);
        ~Data() override;

        NODISCARD bool build();

        NODISCARD bool verify_device_extension_support(VkPhysicalDevice device_handle) const;
        NODISCARD bool verify_device_validation_layer_support(VkPhysicalDevice device_handle) const;
        NODISCARD bool verify_device_properties(const VkPhysicalDeviceProperties& device_properties) const;
        NODISCARD bool verify_device_feature_support(const VkPhysicalDeviceFeatures& device_features) const;
        
        NODISCARD QueueFamilies select_device_queue_families(VkPhysicalDevice device_handle) const;
        NODISCARD GraphicsQueueFamily select_graphics_queue_family(const std::vector<VkQueueFamilyProperties>& queue_families) const;
        
        NODISCARD bool verify_device_queue_family_support(const QueueFamilies& queue_families) const;
        
        std::shared_ptr<VulkanInstance> m_instance;
    
        VkPhysicalDevice m_device;
        VkDevice m_handle;

        VkPhysicalDeviceFeatures m_requested_features;
    
        QueueFamilies m_queue_families;

        // Bitmasks.
        VulkanQueue::Operation m_supported_queues;
        VulkanQueue::Operation m_dedicated_queues;

        std::shared_ptr<VulkanQueue> m_graphics_queue;
        std::shared_ptr<VulkanQueue> m_presentation_queue;
        std::shared_ptr<VulkanQueue> m_compute_queue;
        std::shared_ptr<VulkanQueue> m_async_compute_queue;
        std::shared_ptr<VulkanQueue> m_transfer_queue;
        std::shared_ptr<VulkanQueue> m_async_transfer_queue;
        
        const char* m_name;

        std::vector<const char*> m_extensions;
        std::vector<const char*> m_validation_layers;
    };
    
    bool VulkanDevice::Data::QueueFamilies::is_operation_supported(VulkanQueue::Operation operation) const {
        ASSERT(operation == VulkanQueue::Operation::GRAPHICS || operation == VulkanQueue::Operation::PRESENTATION ||
               operation == VulkanQueue::Operation::COMPUTE || operation == VulkanQueue::Operation::TRANSFER,
               "Operation provided to VulkanDevice::Data::QueueFamilies::is_operation_supported(...) must be a single operation");
        
        switch (operation) {
            case VulkanQueue::Operation::GRAPHICS:
                return graphics_queue_family.index != INVALID_QUEUE_FAMILY_INDEX;
            case VulkanQueue::Operation::PRESENTATION:
                return presentation_queue_family.index != INVALID_QUEUE_FAMILY_INDEX;
            case VulkanQueue::Operation::COMPUTE:
                return compute_queue_family.index != INVALID_QUEUE_FAMILY_INDEX;
            case VulkanQueue::Operation::TRANSFER:
                return transfer_queue_family.index != INVALID_QUEUE_FAMILY_INDEX;
        }
    }
    
    bool VulkanDevice::Data::QueueFamilies::has_dedicated_queue_family(VulkanQueue::Operation operation) const {
        ASSERT(operation == VulkanQueue::Operation::GRAPHICS || operation == VulkanQueue::Operation::PRESENTATION ||
               operation == VulkanQueue::Operation::COMPUTE || operation == VulkanQueue::Operation::TRANSFER,
               "Operation provided to VulkanDevice::Data::QueueFamilies::has_dedicated_queue_family(...) must be a single operation");
        
        if (!is_operation_supported(operation)) {
            return false;
        }
        
        // A dedicated queue family only supports the specified operation.
        
        switch (operation) {
            // If graphics operations are supported, the graphics queue family is considered dedicated.
            // Other operations (compute, presentation, transfer) that share the graphics queue family are not considered dedicated.
            case VulkanQueue::Operation::GRAPHICS:
                return true;
            
            case VulkanQueue::Operation::PRESENTATION:
                return presentation_queue_family.index != graphics_queue_family.index &&
                       presentation_queue_family.index != compute_queue_family.index &&
                       presentation_queue_family.index != transfer_queue_family.index;
            
            case VulkanQueue::Operation::COMPUTE:
                return compute_queue_family.index != graphics_queue_family.index &&
                       compute_queue_family.index != presentation_queue_family.index &&
                       compute_queue_family.index != transfer_queue_family.index;
            
            case VulkanQueue::Operation::TRANSFER:
                return transfer_queue_family.index != graphics_queue_family.index &&
                       transfer_queue_family.index != presentation_queue_family.index &&
                       transfer_queue_family.index != compute_queue_family.index;
        }
    }
    
    VulkanDevice::Data::Data(std::shared_ptr<VulkanInstance> instance) : m_instance(std::move(instance)),
                                                                         m_device(nullptr),
                                                                         m_handle(nullptr),
                                                                         m_requested_features({ }),
                                                                         m_queue_families({ }),
                                                                         m_supported_queues(static_cast<VulkanQueue::Operation>(0u)),
                                                                         m_dedicated_queues(static_cast<VulkanQueue::Operation>(0u)),
                                                                         m_graphics_queue(nullptr),
                                                                         m_presentation_queue(nullptr),
                                                                         m_compute_queue(nullptr),
                                                                         m_async_compute_queue(nullptr),
                                                                         m_transfer_queue(nullptr),
                                                                         m_async_transfer_queue(nullptr),
                                                                         m_name("My Vulkan Device")
                                                                         {
    }

    VulkanDevice::Data::~Data() {
    }

    bool VulkanDevice::Data::build() {
        VkResult result;
    
        // Select physical device. --------------------------------------------------------------------------------------------------------
        // Get available physical devices.
        static VkResult (VKAPI_PTR* fp_vk_enumerate_physical_devices)(VkInstance, u32*, VkPhysicalDevice*) = reinterpret_cast<decltype(fp_vk_enumerate_physical_devices)>(detail::vk_get_instance_proc_addr(*m_instance, "vkEnumeratePhysicalDevices"));
        if (!fp_vk_enumerate_physical_devices) {
            return false;
        }
        
        u32 device_count = 0u;
        result = fp_vk_enumerate_physical_devices(*m_instance, &device_count, nullptr);
        if (result != VK_SUCCESS) {
            // Failed to get physical device count.
            return false;
        }
    
        if (device_count == 0u) {
            // Failed to find GPU with Vulkan support.
            return false;
        }
    
        std::vector<VkPhysicalDevice> devices(device_count);
        result = fp_vk_enumerate_physical_devices(*m_instance, &device_count, devices.data());
        if (result != VK_SUCCESS) {
            // Failed to enumerate physical devices.
            return false;
        }
    
        // vkGetPhysicalDeviceProperties
        static void (VKAPI_PTR* fp_vk_get_physical_device_properties)(VkPhysicalDevice, VkPhysicalDeviceProperties*) = reinterpret_cast<decltype(fp_vk_get_physical_device_properties)>(detail::vk_get_instance_proc_addr(*m_instance, "vkGetPhysicalDeviceProperties"));
        if (!fp_vk_get_physical_device_properties) {
            return false;
        }
        
        // vkGetPhysicalDeviceFeatures
        static void (VKAPI_PTR* fp_vk_get_physical_device_features)(VkPhysicalDevice, VkPhysicalDeviceFeatures*) = reinterpret_cast<decltype(fp_vk_get_physical_device_features)>(detail::vk_get_instance_proc_addr(*m_instance, "vkGetPhysicalDeviceFeatures"));
        if (!fp_vk_get_physical_device_features) {
            return false;
        }
        
        VkPhysicalDevice device = nullptr;
        VkPhysicalDeviceProperties device_properties { };
        VkPhysicalDeviceFeatures device_features { };
        QueueFamilies device_queue_families { };
        
        i32 highest_device_score = -1;
    
        for (u32 i = 0u; i < device_count; ++i) {
            VkPhysicalDevice current_device = devices[i];
        
            // Verify that given device supports all requested device validation layers.
            if (!verify_device_validation_layer_support(current_device)) {
                continue;
            }
        
            // Verify that given device supports all requested device extensions.
            if (!verify_device_extension_support(current_device)) {
                continue;
            }
        
            // Verify that given device properties meet requested requirements.
            VkPhysicalDeviceProperties current_device_properties { };
            fp_vk_get_physical_device_properties(current_device, &current_device_properties);
            if (!verify_device_properties(current_device_properties)) {
                continue;
            }
        
            // Verify that requested device features are supported by the given device.
            VkPhysicalDeviceFeatures current_device_features { };
            fp_vk_get_physical_device_features(current_device, &current_device_features);
            if (!verify_device_feature_support(current_device_features)) {
                continue;
            }
        
            // Verify that a valid queue family configuration exists on this device.
            QueueFamilies current_device_queue_families = select_device_queue_families(current_device);
            if (!verify_device_queue_family_support(current_device_queue_families)) {
                continue;
            }
        
            // Prioritize dedicated graphics hardware.
            int current_device_score = 0;
            
            switch (current_device_properties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    current_device_score = 1000;
                    break;
                
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    current_device_score = 100;
                    break;
                
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    current_device_score = 10;
                    break;
    
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                default:
                    break;
            }
            
            if (current_device_score > highest_device_score) {
                device = current_device;
                device_properties = current_device_properties;
                device_features = current_device_features;
                device_queue_families = current_device_queue_families;
                
                highest_device_score = current_device_score;
            }
        }
    
        if (highest_device_score <= 0) {
            // Failed to find suitable device given selection criteria.
            return false;
        }
        
        m_device = device;
        m_queue_families = device_queue_families;
        
        VulkanDevice::Properties::Type device_type;
        switch (device_properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                device_type = VulkanDevice::Properties::Type::INTEGRATED_GPU;
                break;
        
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                device_type = VulkanDevice::Properties::Type::DISCRETE_GPU;
                break;
        
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                device_type = VulkanDevice::Properties::Type::VIRTUAL_GPU;
                break;
        
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                device_type = VulkanDevice::Properties::Type::CPU;
                break;
        
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                device_type = VulkanDevice::Properties::Type::OTHER;
                break;
        }
    
        properties = VulkanDevice::Properties {
            .type = device_type,
            .limits = VulkanDevice::Properties::Limits {
                .max_image_dimension_1d = device_properties.limits.maxImageDimension1D,
                .max_image_dimension_2d = device_properties.limits.maxImageDimension2D,
                .max_image_dimension_3d = device_properties.limits.maxImageDimension3D,
                .max_image_dimension_cube = device_properties.limits.maxImageDimensionCube,
                .max_image_array_layers = device_properties.limits.maxImageArrayLayers,
                .max_texel_buffer_elements = device_properties.limits.maxTexelBufferElements,
                .max_uniform_buffer_range = device_properties.limits.maxUniformBufferRange,
                .max_storage_buffer_range = device_properties.limits.maxStorageBufferRange,
                .max_push_constants_size = device_properties.limits.maxPushConstantsSize,
                .max_memory_allocation_count = device_properties.limits.maxMemoryAllocationCount,
                .max_sampler_allocation_count = device_properties.limits.maxSamplerAllocationCount,
                .buffer_image_granularity = static_cast<u32>(device_properties.limits.bufferImageGranularity),
                .sparse_address_space_size = static_cast<u32>(device_properties.limits.sparseAddressSpaceSize),
                .max_bound_descriptor_sets = device_properties.limits.maxBoundDescriptorSets,
                .max_per_stage_descriptor_samplers = device_properties.limits.maxPerStageDescriptorSamplers,
                .max_per_stage_descriptor_uniform_buffers = device_properties.limits.maxPerStageDescriptorUniformBuffers,
                .max_per_stage_descriptor_storage_buffers = device_properties.limits.maxPerStageDescriptorStorageBuffers,
                .max_per_stage_descriptor_sampled_images = device_properties.limits.maxPerStageDescriptorSampledImages,
                .max_per_stage_descriptor_storage_images = device_properties.limits.maxPerStageDescriptorStorageImages,
                .max_per_stage_descriptor_input_attachments = device_properties.limits.maxPerStageDescriptorInputAttachments,
                .max_per_stage_resources = device_properties.limits.maxPerStageResources,
                .max_descriptor_set_samplers = device_properties.limits.maxDescriptorSetSamplers,
                .max_descriptor_set_uniform_buffers = device_properties.limits.maxDescriptorSetUniformBuffers,
                .max_descriptor_set_uniform_buffers_dynamic = device_properties.limits.maxDescriptorSetUniformBuffersDynamic,
                .max_descriptor_set_storage_buffers = device_properties.limits.maxDescriptorSetStorageBuffers,
                .max_descriptor_set_storage_buffers_dynamic = device_properties.limits.maxDescriptorSetStorageBuffersDynamic,
                .max_descriptor_set_sampled_images = device_properties.limits.maxDescriptorSetSampledImages,
                .max_descriptor_set_storage_images = device_properties.limits.maxDescriptorSetStorageBuffers,
                .max_descriptor_set_input_attachments = device_properties.limits.maxDescriptorSetInputAttachments,
                .max_vertex_input_attributes = device_properties.limits.maxVertexInputAttributes,
                .max_vertex_input_bindings = device_properties.limits.maxVertexInputBindings,
                .max_vertex_input_attribute_offset = device_properties.limits.maxVertexInputAttributeOffset,
                .max_vertex_input_binding_stride = device_properties.limits.maxVertexInputBindingStride,
                .max_vertex_output_components = device_properties.limits.maxVertexOutputComponents,
                .max_tesselation_generation_level = device_properties.limits.maxTessellationGenerationLevel,
                .max_tesselation_patch_size = device_properties.limits.maxTessellationPatchSize,
                .max_tesselation_control_per_vertex_input_components = device_properties.limits.maxTessellationControlPerVertexInputComponents,
                .max_tesselation_control_per_vertex_output_components = device_properties.limits.maxTessellationControlPerVertexOutputComponents,
                .max_tesselation_control_per_patch_output_components = device_properties.limits.maxTessellationControlPerPatchOutputComponents,
                .max_tesselation_control_total_output_components = device_properties.limits.maxTessellationControlTotalOutputComponents,
                .max_tesselation_evaluation_input_components = device_properties.limits.maxTessellationEvaluationInputComponents,
                .max_tesselation_evaluation_output_components = device_properties.limits.maxTessellationEvaluationOutputComponents,
                .max_geometry_shader_invocations = device_properties.limits.maxGeometryShaderInvocations,
                .max_geometry_input_components = device_properties.limits.maxGeometryInputComponents,
                .max_geometry_output_components = device_properties.limits.maxGeometryOutputComponents,
                .max_geometry_output_vertices = device_properties.limits.maxGeometryOutputVertices,
                .max_geometry_total_output_components = device_properties.limits.maxGeometryTotalOutputComponents,
                .max_fragment_input_components = device_properties.limits.maxFragmentInputComponents,
                .max_fragment_output_attachments = device_properties.limits.maxFragmentOutputAttachments,
                .max_fragment_dual_src_attachments = device_properties.limits.maxFragmentDualSrcAttachments,
                .max_fragment_combined_output_resources = device_properties.limits.maxFragmentCombinedOutputResources,
                .max_compute_shared_memory_size = device_properties.limits.maxComputeSharedMemorySize,
                .max_compute_work_group_count = { device_properties.limits.maxComputeWorkGroupCount[0], device_properties.limits.maxComputeWorkGroupCount[1], device_properties.limits.maxComputeWorkGroupCount[2] },
                .max_compute_work_group_invocations = device_properties.limits.maxComputeWorkGroupInvocations,
                .max_compute_work_group_size = { device_properties.limits.maxComputeWorkGroupSize[0], device_properties.limits.maxComputeWorkGroupSize[1], device_properties.limits.maxComputeWorkGroupSize[2] },
                .subpixel_precision_bits = device_properties.limits.subPixelPrecisionBits,
                .subtexel_precision_bits = device_properties.limits.subTexelPrecisionBits,
                .mipmap_precision_bits = device_properties.limits.mipmapPrecisionBits,
                .max_draw_indexed_index_value = device_properties.limits.maxDrawIndexedIndexValue,
                .max_draw_indirect_count = device_properties.limits.maxDrawIndirectCount,
                .max_sampler_lod_bias = device_properties.limits.maxSamplerLodBias,
                .max_sampler_anisotropy = device_properties.limits.maxSamplerAnisotropy,
                .max_viewports = device_properties.limits.maxViewports,
                .max_viewport_dimensions = { device_properties.limits.maxViewportDimensions[0], device_properties.limits.maxViewportDimensions[1] },
                .viewport_bounds_range = { device_properties.limits.viewportBoundsRange[0], device_properties.limits.viewportBoundsRange[1] },
                .viewport_subpixel_bits = device_properties.limits.viewportSubPixelBits,
                .min_memory_map_alignment = device_properties.limits.minMemoryMapAlignment,
                .min_texel_buffer_offset_alignment = device_properties.limits.minTexelBufferOffsetAlignment,
                .min_uniform_buffer_offset_alignment = device_properties.limits.minUniformBufferOffsetAlignment,
                .min_storage_buffer_offset_alignment = device_properties.limits.minStorageBufferOffsetAlignment,
                .min_texel_offset = device_properties.limits.minTexelOffset,
                .max_texel_offset = device_properties.limits.maxTexelOffset,
                .min_texel_gather_offset = device_properties.limits.minTexelGatherOffset,
                .max_texel_gather_offset = device_properties.limits.maxTexelGatherOffset,
                .min_interpolation_offset = device_properties.limits.minInterpolationOffset,
                .max_interpolation_offset = device_properties.limits.maxInterpolationOffset,
                .subpixel_interpolation_offset_bits = device_properties.limits.subPixelInterpolationOffsetBits,
                .max_framebuffer_width = device_properties.limits.maxFramebufferWidth,
                .max_framebuffer_height = device_properties.limits.maxFramebufferHeight,
                .max_framebuffer_layers = device_properties.limits.maxFramebufferLayers,
                .framebuffer_color_sample_counts = device_properties.limits.framebufferColorSampleCounts,
                .framebuffer_depth_sample_counts = device_properties.limits.framebufferDepthSampleCounts,
                .framebuffer_stencil_sample_counts = device_properties.limits.framebufferStencilSampleCounts,
                .framebuffer_no_attachments_sample_counts = device_properties.limits.framebufferNoAttachmentsSampleCounts,
                .max_color_attachments = device_properties.limits.maxColorAttachments,
                .sampled_image_color_sample_counts = device_properties.limits.sampledImageColorSampleCounts,
                .sampled_image_integer_sample_counts = device_properties.limits.sampledImageIntegerSampleCounts,
                .sample_image_depth_sample_counts = device_properties.limits.sampledImageDepthSampleCounts,
                .sample_image_stencil_sample_counts = device_properties.limits.sampledImageStencilSampleCounts,
                .storage_image_sample_counts = device_properties.limits.storageImageSampleCounts,
                .max_sample_mask_words = device_properties.limits.maxSampleMaskWords,
                .timestamp_compute_and_graphics = static_cast<bool>(device_properties.limits.timestampComputeAndGraphics),
                .timestamp_period = device_properties.limits.timestampPeriod,
                .max_clip_distances = device_properties.limits.maxClipDistances,
                .max_cull_distances = device_properties.limits.maxCullDistances,
                .max_combined_clip_and_cull_distances = device_properties.limits.maxCombinedClipAndCullDistances,
                .discrete_queue_priorities = device_properties.limits.discreteQueuePriorities,
                .point_size_range = { device_properties.limits.pointSizeRange[0], device_properties.limits.pointSizeRange[1] },
                .line_width_range = { device_properties.limits.lineWidthRange[0], device_properties.limits.lineWidthRange[1] },
                .point_size_granularity = device_properties.limits.pointSizeGranularity,
                .line_width_granularity = device_properties.limits.lineWidthGranularity,
                .strict_lines = static_cast<bool>(device_properties.limits.strictLines),
                .standard_sample_locations = static_cast<bool>(device_properties.limits.standardSampleLocations),
                .optimal_buffer_copy_offset_alignment = device_properties.limits.optimalBufferCopyOffsetAlignment,
                .optimal_buffer_copy_row_pitch_alignment = device_properties.limits.optimalBufferCopyRowPitchAlignment,
                .non_coherent_atom_size = device_properties.limits.nonCoherentAtomSize
            },
            .runtime = {
                .major = VK_API_VERSION_MAJOR(device_properties.apiVersion),
                .minor = VK_API_VERSION_MINOR(device_properties.apiVersion),
                .patch = VK_API_VERSION_PATCH(device_properties.apiVersion)
            },
            .driver_version = device_properties.driverVersion,
            .device_name = std::string(device_properties.deviceName)
        };
    
        // Safe to static_cast to bool: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBool32.html
        features = VulkanDevice::Features {
            .robust_buffer_access = static_cast<bool>(device_features.robustBufferAccess),
            .full_draw_index_uint32 = static_cast<bool>(device_features.fullDrawIndexUint32),
            .image_cube_array = static_cast<bool>(device_features.imageCubeArray),
            .independent_blend = static_cast<bool>(device_features.independentBlend),
            .geometry_shader = static_cast<bool>(device_features.geometryShader),
            .tesselation_shader = static_cast<bool>(device_features.tessellationShader),
            .sample_rate_shading = static_cast<bool>(device_features.sampleRateShading),
            .dual_src_blend = static_cast<bool>(device_features.dualSrcBlend),
            .logic_operations = static_cast<bool>(device_features.logicOp),
            .multi_draw_indirect = static_cast<bool>(device_features.multiDrawIndirect),
            .draw_indirect_first_instance = static_cast<bool>(device_features.drawIndirectFirstInstance),
            .depth_clamp = static_cast<bool>(device_features.depthClamp),
            .depth_bias_clamp = static_cast<bool>(device_features.depthBiasClamp),
            .fill_mode_non_solid = static_cast<bool>(device_features.fillModeNonSolid),
            .depth_bounds = static_cast<bool>(device_features.depthBounds),
            .wide_lines = static_cast<bool>(device_features.wideLines),
            .large_points = static_cast<bool>(device_features.largePoints),
            .alpha_to_one = static_cast<bool>(device_features.alphaToOne),
            .multi_viewport = static_cast<bool>(device_features.multiViewport),
            .sampler_anisotropy = static_cast<bool>(device_features.samplerAnisotropy),
            .texture_compression_etc2 = static_cast<bool>(device_features.textureCompressionETC2),
            .texture_compression_astc_ldr = static_cast<bool>(device_features.textureCompressionASTC_LDR),
            .texture_compression_bc = static_cast<bool>(device_features.textureCompressionBC),
            .occlusion_query_precise = static_cast<bool>(device_features.occlusionQueryPrecise),
            .pipeline_statistics_query = static_cast<bool>(device_features.pipelineStatisticsQuery),
            .vertex_pipeline_stores_and_atomics = static_cast<bool>(device_features.vertexPipelineStoresAndAtomics),
            .fragment_stores_and_atomics = static_cast<bool>(device_features.fragmentStoresAndAtomics),
            .shader_tesselation_and_geometry_point_size = static_cast<bool>(device_features.shaderTessellationAndGeometryPointSize),
            .shader_image_gather_extended = static_cast<bool>(device_features.shaderImageGatherExtended),
            .shader_storage_image_extended_formats = static_cast<bool>(device_features.shaderStorageImageExtendedFormats),
            .shader_storage_multisample = static_cast<bool>(device_features.shaderStorageImageMultisample),
            .shader_storage_image_read_without_format = static_cast<bool>(device_features.shaderStorageImageReadWithoutFormat),
            .shader_storage_image_write_without_format = static_cast<bool>(device_features.shaderStorageImageWriteWithoutFormat),
            .shader_uniform_buffer_array_dynamic_indexing = static_cast<bool>(device_features.shaderUniformBufferArrayDynamicIndexing),
            .shader_sampled_image_array_dynamic_indexing = static_cast<bool>(device_features.shaderSampledImageArrayDynamicIndexing),
            .shader_storage_buffer_array_dynamic_indexing = static_cast<bool>(device_features.shaderStorageBufferArrayDynamicIndexing),
            .shader_storage_image_array_dynamic_indexing = static_cast<bool>(device_features.shaderStorageImageArrayDynamicIndexing),
            .shader_clip_distance = static_cast<bool>(device_features.shaderClipDistance),
            .shader_cull_distance = static_cast<bool>(device_features.shaderCullDistance),
            .shader_float64 = static_cast<bool>(device_features.shaderFloat64),
            .shader_int64 = static_cast<bool>(device_features.shaderInt64),
            .shader_int16 = static_cast<bool>(device_features.shaderInt16),
            .shader_resource_residency = static_cast<bool>(device_features.shaderResourceResidency),
            .shader_resource_min_lod = static_cast<bool>(device_features.shaderResourceMinLod),
            .sparse_binding = static_cast<bool>(device_features.sparseBinding),
            .sparse_residency_buffer = static_cast<bool>(device_features.sparseResidencyBuffer),
            .sparse_residency_image2d = static_cast<bool>(device_features.sparseResidencyImage2D),
            .sparse_residency_image3d = static_cast<bool>(device_features.sparseResidencyImage3D),
            .sparse_residency_2_samples = static_cast<bool>(device_features.sparseResidency2Samples),
            .sparse_residency_4_samples = static_cast<bool>(device_features.sparseResidency4Samples),
            .sparse_residency_8_samples = static_cast<bool>(device_features.sparseResidency8Samples),
            .sparse_residency_16_samples = static_cast<bool>(device_features.sparseResidency16Samples),
            .sparse_residency_aliased = static_cast<bool>(device_features.sparseResidencyAliased),
            .variable_multisample_rate = static_cast<bool>(device_features.variableMultisampleRate),
            .inherited_queries = static_cast<bool>(device_features.inheritedQueries),
        };
    
        // Construct logical device. ----------------------------------------------------------------------------------
        float queue_priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    
        // Append all unique queue family indices.
        std::unordered_set<i32> unique_queue_family_indices {
            device_queue_families.graphics_queue_family.index,
            device_queue_families.presentation_queue_family.index,
            device_queue_families.compute_queue_family.index,
            device_queue_families.transfer_queue_family.index
        };
    
        queue_create_infos.reserve(unique_queue_family_indices.size());
        for (u32 queue_family_index : unique_queue_family_indices) {
            queue_create_infos.emplace_back(VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // Reserved by the Vulkan API for future use.
                .queueFamilyIndex = static_cast<u32>(queue_family_index),
                .queueCount = 1u, // TODO: configurable amount of queues?
                .pQueuePriorities = &queue_priority
            });
        }
    
        // Create logical device.
        VkDeviceCreateInfo device_create_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0, // Reserved by the Vulkan API for future use.
            .queueCreateInfoCount = static_cast<u32>(queue_create_infos.size()),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledLayerCount = static_cast<u32>(m_validation_layers.size()),
            .ppEnabledLayerNames = m_validation_layers.data(),
            .enabledExtensionCount = static_cast<u32>(m_extensions.size()),
            .ppEnabledExtensionNames = m_extensions.data(),
            .pEnabledFeatures = &m_requested_features
        };
    
        static VkResult (VKAPI_PTR* fp_vk_create_device)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*) = reinterpret_cast<decltype(fp_vk_create_device)>(detail::vk_get_instance_proc_addr(*m_instance, "vkCreateDevice"));
        if (!fp_vk_create_device) {
            return false;
        }
        
        result = fp_vk_create_device(device, &device_create_info, nullptr, &m_handle);
        if (result != VK_SUCCESS) {
            return false;
        }
        
        // Retrieve device queues.
        VulkanQueue::Builder queue_builder(m_instance, shared_from_this());
        
        if (m_queue_families.is_operation_supported(VulkanQueue::Operation::GRAPHICS)) {
            
            // Register all operations within the graphics family.
            VulkanQueue::Operation supported_queue_operations = VulkanQueue::Operation::GRAPHICS;
    
            if (m_queue_families.graphics_queue_family.has_presentation_support) {
                supported_queue_operations |= VulkanQueue::Operation::PRESENTATION;
            }
    
            if (m_queue_families.graphics_queue_family.has_compute_support) {
                supported_queue_operations |= VulkanQueue::Operation::COMPUTE;
            }
    
            if (m_queue_families.graphics_queue_family.has_transfer_support) {
                supported_queue_operations |= VulkanQueue::Operation::TRANSFER;
            }
            
            m_graphics_queue = queue_builder.with_operation(supported_queue_operations, m_queue_families.graphics_queue_family.index).build();
            
            if (m_queue_families.graphics_queue_family.has_presentation_support) {
                // A queue family that supports presentation operations was requested by the user.
                m_presentation_queue = m_graphics_queue;
            }
    
            // Synchronous compute / transfer queues submit operations to the graphics queue.
    
            if (test(m_supported_queues, VulkanQueue::Operation::COMPUTE) && m_queue_families.graphics_queue_family.has_compute_support) {
                // A queue family that supports compute operations was requested by the user.
                m_compute_queue = m_graphics_queue;
            }
            
            if (test(m_supported_queues, VulkanQueue::Operation::TRANSFER) && m_queue_families.graphics_queue_family.has_transfer_support) {
                // A queue family that supports transfer operations was requested by the user.
                m_transfer_queue = m_graphics_queue;
            }
        }
        
        if (test(m_dedicated_queues, VulkanQueue::Operation::PRESENTATION) && m_queue_families.has_dedicated_queue_family(VulkanQueue::Operation::PRESENTATION)) {
            // A queue family that is dedicated to presentation operations was requested by the user.
            m_presentation_queue = queue_builder.with_operation(VulkanQueue::Operation::PRESENTATION, m_queue_families.presentation_queue_family.index).build();
        }
        
        if (test(m_supported_queues, VulkanQueue::Operation::COMPUTE))
        
        if (!headless) {
            if (!m_presentation_queue) {
                ASSERT(m_queue_families.presentation_queue_family.index != m_queue_families.graphics_queue_family.index, "Misconfigured presentation / graphics queue families");
                ASSERT(!m_queue_families.graphics_queue_family.has_presentation_support, "Misconfigured presentation / graphics queue families");
                
                // Presentation queue is separate from the graphics queue.
                VulkanQueue::Operation supported_queue_operations = VulkanQueue::Operation::PRESENTATION;
                
                if (m_queue_families.presentation_queue_family.index == m_queue_families.compute_queue_family.index) {
                    supported_queue_operations |= VulkanQueue::Operation::COMPUTE;
                }
                
                if (m_queue_families.presentation_queue_family.index == m_queue_families.transfer_queue_family.index) {
                    supported_queue_operations |= VulkanQueue::Operation::TRANSFER;
                }

                m_presentation_queue = queue_builder.with_operation(supported_queue_operations, m_queue_families.presentation_queue_family.index).build();
    
                if (m_queue_families.presentation_queue_family.index == m_queue_families.compute_queue_family.index) {
                    m_async_compute_queue = m_presentation_queue;
                }
    
                if (m_queue_families.presentation_queue_family.index == m_queue_families.compute_queue_family.index) {
                    m_async_transfer_queue = m_presentation_queue;
                }
            }
        }
        
        if (!)
        
        return true;
    }
    
    bool VulkanDevice::Data::verify_device_extension_support(VkPhysicalDevice device_handle) const {
        static VkResult (VKAPI_PTR* fp_vk_enumerate_device_extension_properties)(VkPhysicalDevice, const char*, u32*, VkExtensionProperties*) = reinterpret_cast<decltype(fp_vk_enumerate_device_extension_properties)>(detail::vk_get_instance_proc_addr(*m_instance, "vkEnumerateDeviceExtensionProperties"));
        VkResult result;
    
        u32 device_extension_count = 0u;
        result = fp_vk_enumerate_device_extension_properties(device_handle, nullptr, &device_extension_count, nullptr);
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
    
        std::vector<VkExtensionProperties> device_extensions(device_extension_count);
        result = fp_vk_enumerate_device_extension_properties(device_handle, nullptr, &device_extension_count, device_extensions.data());
    
        for (const char* layer : m_validation_layers) {
            u32 layer_extension_count = 0u;
            fp_vk_enumerate_device_extension_properties(device_handle, layer, &layer_extension_count, nullptr);
        
            std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
            fp_vk_enumerate_device_extension_properties(device_handle, layer, &layer_extension_count, layer_extensions.data());
        
            device_extension_count += layer_extension_count;
            device_extensions.insert(device_extensions.end(), layer_extensions.begin(), layer_extensions.end());
        }
    
        std::vector<const char*> unsupported_extensions;
        unsupported_extensions.reserve(m_extensions.size());
    
        for (const char* requested : m_extensions) {
            bool found = false;
        
            for (u32 j = 0u; j < device_extension_count; ++j) {
                const char* available = device_extensions[j].extensionName;
            
                if (strcmp(requested, available) == 0) {
                    found = true;
                    break;
                }
            }
        
            if (!found) {
                unsupported_extensions.emplace_back(requested);
            }
        }
    
        if (!unsupported_extensions.empty()) {
            // TODO: log message
            return false;
        }
    
        return true;
    }
    
    bool VulkanDevice::Data::verify_device_validation_layer_support(VkPhysicalDevice device_handle) const {
        VkResult (VKAPI_PTR* fp_vk_enumerate_device_layer_properties)(VkPhysicalDevice, u32*, VkLayerProperties*) = reinterpret_cast<decltype(fp_vk_enumerate_device_layer_properties)>(detail::vk_get_instance_proc_addr(*m_instance, "vkEnumerateDeviceLayerProperties"));
        VkResult result;
    
        u32 device_validation_layer_count = 0u;
        result = fp_vk_enumerate_device_layer_properties(device_handle, &device_validation_layer_count, nullptr);
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
    
        std::vector<VkLayerProperties> device_validation_layers(device_validation_layer_count);
        result = fp_vk_enumerate_device_layer_properties(device_handle, &device_validation_layer_count, device_validation_layers.data());
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
    
        std::vector<const char*> unsupported_layers;
        unsupported_layers.reserve(m_validation_layers.size()); // Maximum number of unsupported layers.
    
        // Verify that all requested validation layers are available.
        for (const char* requested : m_validation_layers) {
            bool found = false;
        
            for (u32 j = 0u; j < device_validation_layer_count; ++j) {
                const char* available = device_validation_layers[j].layerName;
            
                if (strcmp(requested, available) == 0) {
                    found = true;
                    break;
                }
            }
        
            if (!found) {
                unsupported_layers.emplace_back(requested);
            }
        }
    
        if (!unsupported_layers.empty()) {
            // TODO: log message
            return false;
        }
    
        return true;
    }
    
    bool VulkanDevice::Data::verify_device_properties(const VkPhysicalDeviceProperties& device_properties) const {
        return device_properties.apiVersion >= m_instance->get_runtime_version();
    }
    
    bool VulkanDevice::Data::verify_device_feature_support(const VkPhysicalDeviceFeatures& device_features) const {
        if (m_requested_features.robustBufferAccess && !static_cast<bool>(device_features.robustBufferAccess)) {
            return false;
        }
    
        if (m_requested_features.fullDrawIndexUint32 && !static_cast<bool>(device_features.fullDrawIndexUint32)) {
            return false;
        }
    
        if (m_requested_features.imageCubeArray && !static_cast<bool>(device_features.imageCubeArray)) {
            return false;
        }
    
        if (m_requested_features.independentBlend && !static_cast<bool>(device_features.independentBlend)) {
            return false;
        }
    
        if (m_requested_features.geometryShader && !static_cast<bool>(device_features.geometryShader)) {
            return false;
        }
    
        if (m_requested_features.tessellationShader && !static_cast<bool>(device_features.tessellationShader)) {
            return false;
        }
    
        if (m_requested_features.sampleRateShading && !static_cast<bool>(device_features.sampleRateShading)) {
            return false;
        }
    
        if (m_requested_features.dualSrcBlend && !static_cast<bool>(device_features.dualSrcBlend)) {
            return false;
        }
    
        if (m_requested_features.logicOp && !static_cast<bool>(device_features.logicOp)) {
            return false;
        }
    
        if (m_requested_features.multiDrawIndirect && !static_cast<bool>(device_features.multiDrawIndirect)) {
            return false;
        }
    
        if (m_requested_features.drawIndirectFirstInstance && !static_cast<bool>(device_features.drawIndirectFirstInstance)) {
            return false;
        }
    
        if (m_requested_features.depthClamp && !static_cast<bool>(device_features.depthClamp)) {
            return false;
        }
    
        if (m_requested_features.depthBiasClamp && !static_cast<bool>(device_features.depthBiasClamp)) {
            return false;
        }
    
        if (m_requested_features.fillModeNonSolid && !static_cast<bool>(device_features.fillModeNonSolid)) {
            return false;
        }
    
        if (m_requested_features.depthBounds && !static_cast<bool>(device_features.depthBounds)) {
            return false;
        }
    
        if (m_requested_features.wideLines && !static_cast<bool>(device_features.wideLines)) {
            return false;
        }
    
        if (m_requested_features.largePoints && !static_cast<bool>(device_features.largePoints)) {
            return false;
        }
    
        if (m_requested_features.alphaToOne && !static_cast<bool>(device_features.alphaToOne)) {
            return false;
        }
    
        if (m_requested_features.multiViewport && !static_cast<bool>(device_features.multiViewport)) {
            return false;
        }
    
        if (m_requested_features.samplerAnisotropy && !static_cast<bool>(device_features.samplerAnisotropy)) {
            return false;
        }
    
        if (m_requested_features.textureCompressionETC2 && !static_cast<bool>(device_features.textureCompressionETC2)) {
            return false;
        }
    
        if (m_requested_features.textureCompressionASTC_LDR && !static_cast<bool>(device_features.textureCompressionASTC_LDR)) {
            return false;
        }
    
        if (m_requested_features.textureCompressionBC && !static_cast<bool>(device_features.textureCompressionBC)) {
            return false;
        }
    
        if (m_requested_features.occlusionQueryPrecise && !static_cast<bool>(device_features.occlusionQueryPrecise)) {
            return false;
        }
    
        if (m_requested_features.pipelineStatisticsQuery && !static_cast<bool>(device_features.pipelineStatisticsQuery)) {
            return false;
        }
    
        if (m_requested_features.vertexPipelineStoresAndAtomics && !static_cast<bool>(device_features.vertexPipelineStoresAndAtomics)) {
            return false;
        }
    
        if (m_requested_features.fragmentStoresAndAtomics && !static_cast<bool>(device_features.fragmentStoresAndAtomics)) {
            return false;
        }
    
        if (m_requested_features.shaderTessellationAndGeometryPointSize && !static_cast<bool>(device_features.shaderTessellationAndGeometryPointSize)) {
            return false;
        }
    
        if (m_requested_features.shaderImageGatherExtended && !static_cast<bool>(device_features.shaderImageGatherExtended)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageImageExtendedFormats && !static_cast<bool>(device_features.shaderStorageImageExtendedFormats)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageImageMultisample && !static_cast<bool>(device_features.shaderStorageImageMultisample)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageImageReadWithoutFormat && !static_cast<bool>(device_features.shaderStorageImageReadWithoutFormat)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageImageWriteWithoutFormat && !static_cast<bool>(device_features.shaderStorageImageWriteWithoutFormat)) {
            return false;
        }
    
        if (m_requested_features.shaderUniformBufferArrayDynamicIndexing && !static_cast<bool>(device_features.shaderUniformBufferArrayDynamicIndexing)) {
            return false;
        }
    
        if (m_requested_features.shaderSampledImageArrayDynamicIndexing && !static_cast<bool>(device_features.shaderSampledImageArrayDynamicIndexing)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageBufferArrayDynamicIndexing && !static_cast<bool>(device_features.shaderStorageBufferArrayDynamicIndexing)) {
            return false;
        }
    
        if (m_requested_features.shaderStorageImageArrayDynamicIndexing && !static_cast<bool>(device_features.shaderStorageImageArrayDynamicIndexing)) {
            return false;
        }
    
        if (m_requested_features.shaderClipDistance && !static_cast<bool>(device_features.shaderClipDistance)) {
            return false;
        }
    
        if (m_requested_features.shaderCullDistance && !static_cast<bool>(device_features.shaderCullDistance)) {
            return false;
        }
    
        if (m_requested_features.shaderFloat64 && !static_cast<bool>(device_features.shaderFloat64)) {
            return false;
        }
    
        if (m_requested_features.shaderInt64 && !static_cast<bool>(device_features.shaderInt64)) {
            return false;
        }
    
        if (m_requested_features.shaderInt16 && !static_cast<bool>(device_features.shaderInt16)) {
            return false;
        }
    
        if (m_requested_features.shaderResourceResidency && !static_cast<bool>(device_features.shaderResourceResidency)) {
            return false;
        }
    
        if (m_requested_features.shaderResourceMinLod && !static_cast<bool>(device_features.shaderResourceMinLod)) {
            return false;
        }
    
        if (m_requested_features.sparseBinding && !static_cast<bool>(device_features.sparseBinding)) {
            return false;
        }
    
        if (m_requested_features.sparseResidencyBuffer && !static_cast<bool>(device_features.sparseResidencyBuffer)) {
            return false;
        }
    
        if (m_requested_features.sparseResidencyImage2D && !static_cast<bool>(device_features.sparseResidencyImage2D)) {
            return false;
        }
    
        if (m_requested_features.sparseResidencyImage3D && !static_cast<bool>(device_features.sparseResidencyImage3D)) {
            return false;
        }
    
        if (m_requested_features.sparseResidency2Samples && !static_cast<bool>(device_features.sparseResidency2Samples)) {
            return false;
        }
    
        if (m_requested_features.sparseResidency4Samples && !static_cast<bool>(device_features.sparseResidency4Samples)) {
            return false;
        }
    
        if (m_requested_features.sparseResidency8Samples && !static_cast<bool>(device_features.sparseResidency8Samples)) {
            return false;
        }
    
        if (m_requested_features.sparseResidency16Samples && !static_cast<bool>(device_features.sparseResidency16Samples)) {
            return false;
        }
    
        if (m_requested_features.sparseResidencyAliased && !static_cast<bool>(device_features.sparseResidencyAliased)) {
            return false;
        }
    
        if (m_requested_features.variableMultisampleRate && !static_cast<bool>(device_features.variableMultisampleRate)) {
            return false;
        }
    
        if (m_requested_features.inheritedQueries && !static_cast<bool>(device_features.inheritedQueries)) {
            return false;
        }
    
        return true;
    }
    
    VulkanDevice::Data::QueueFamilies VulkanDevice::Data::select_device_queue_families(VkPhysicalDevice device_handle) const {
        // vkGetPhysicalDeviceQueueFamilyProperties
        static void (VKAPI_PTR* fp_vk_get_physical_device_queue_family_properties)(VkPhysicalDevice, u32*, VkQueueFamilyProperties*) = reinterpret_cast<decltype(fp_vk_get_physical_device_queue_family_properties)>(detail::vk_get_instance_proc_addr(*m_instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
        ASSERT(fp_vk_get_physical_device_queue_family_properties != nullptr, "Failed to load vkGetPhysicalDeviceQueueFamilyProperties");
    
        #if defined(VKS_PLATFORM_WINDOWS)
            static VkBool32 (VKAPI_PTR* fp_vk_get_physical_device_presentation_support)(VkPhysicalDevice, u32) = reinterpret_cast<decltype(fp_vk_get_physical_device_presentation_support)>(detail::vk_get_instance_proc_addr(*m_instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
            ASSERT(fp_vk_get_physical_device_presentation_support != nullptr, "Failed to load vkGetPhysicalDeviceWin32PresentationSupportKHR");
        #endif
        
        u32 queue_family_count = 0u;
        fp_vk_get_physical_device_queue_family_properties(device_handle, &queue_family_count, nullptr);
    
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        fp_vk_get_physical_device_queue_family_properties(device_handle, &queue_family_count, queue_families.data());
        
        u32 highest_score = 0u;
    
        for (u32 i = 0u; i < queue_families.size(); ++i) {
            const VkQueueFamilyProperties& family = queue_families[i];
            bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
            bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
        
            std::cout << "Queue family: " << i << ", queue count: " << family.queueCount << std::endl;
            std::cout << "graphics support? " << (has_graphics_support ? "yes " : "no") << std::endl;
            std::cout << "presentation support? " << (has_presentation_support ? "yes" : "no") << std::endl;
            std::cout << "compute support? " << (has_compute_support ? "yes" : "no") << std::endl;
            std::cout << "transfer support? " << (has_transfer_support ? "yes" : "no") << std::endl;
            std::cout << std::endl;
        }
    
        GraphicsQueueFamily graphics_queue_family {
            .index = QueueFamilies::INVALID_QUEUE_FAMILY_INDEX,
            .num_available_queues = 0u,
            .has_presentation_support = false,
            .has_compute_support = false,
            .has_transfer_support = false
        };
    
        bool graphics_support_requested = test(m_supported_queues, VulkanQueue::Operation::GRAPHICS);
        bool dedicated_graphics_queue_requested = test(m_dedicated_queues, VulkanQueue::Operation::GRAPHICS);
        
        bool compute_support_requested = test(m_supported_queues, VulkanQueue::Operation::COMPUTE);
        bool dedicated_compute_queue_requested = test(m_dedicated_queues, VulkanQueue::Operation::COMPUTE);
        
        bool transfer_support_requested = test(m_supported_queues, VulkanQueue::Operation::TRANSFER);
        bool dedicated_transfer_queue_requested = test(m_dedicated_queues, VulkanQueue::Operation::TRANSFER);
        
        bool presentation_support_requested = test(m_supported_queues, VulkanQueue::Operation::PRESENTATION);
        bool dedicated_presentation_queue_requested = test(m_dedicated_queues, VulkanQueue::Operation::PRESENTATION);
        
        for (u32 i = 0u; i < queue_family_count; ++i) {
            const VkQueueFamilyProperties& family = queue_families[i];
            bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;

            #if defined(VKS_PLATFORM_WINDOWS)
                bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
            #endif

            if (graphics_support_requested) {
                if (presentation_support_requested && !dedicated_presentation_queue_requested) {
                    if (compute_support_requested) {
                    
                    }
                    else if (transfer_support_requested) {
                    
                    }
                }
                else {
                    // Presentation support is optional.
                }
            }
            
            if (graphics_support_requested) {
                if (presentation_support_requested) {
                }
                else {
                }
            }
            
            if (graphics_support_requested) {
            }
            
            
            if (graphics_support_requested && !has_graphics_support) {
                continue;
            }
            
            if ((dedicated_presentation_queue_requested) || (presentation_support_requested && !has_presentation_support)) {
                continue;
            }
            
            if (compute_support_requested && !has_compute_support) {
                continue;
            }
            
            if (transfer_support_requested && (!has_transfer_support || !(has_graphics_support && has_compute_support))) {
                continue;
            }

            // Scoring of queue family based on supported operations:
            // 6. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
            // 5. GRAPHICS | PRESENTATION | TRANSFER
            // 4. GRAPHICS | PRESENTATION
            // 3. GRAPHICS | COMPUTE | TRANSFER
            // 2. GRAPHICS | TRANSFER
            // 1. GRAPHICS
            u32 current_score = 0u;
            
                    
                    
                    if (!has_graphics_support) {
                        continue;
                    }
                    

                    
                    // Notes:
                    //  - Since a dedicated presentation queue was not requested, prefer a queue family that supports both graphics and presentation operations for improved performance.
                    //  - Synchronous compute / transfer operations (requested by compute / transfer operation support) will be submitted to the graphics family. Asynchronous compute / transfer
                    //    operations (requested by a dedicated compute / transfer queue) will submit operations to a separate queue family.
                    
                    if (has_presentation_support) {
                        if (has_compute_support) {
                            // Supported operations: GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
                            current_score = 6u;
                        }
                        else if (has_transfer_support) {
                            // Supported operations: GRAPHICS | PRESENTATION | TRANSFER
                            current_score = 5u;
                        }
                        else {
                            // Supported operations: GRAPHICS | PRESENTATION
                            current_score = 4u;
                        }
                    }
                    else if (has_compute_support) {
                        // Supported operations: GRAPHICS | COMPUTE | TRANSFER
                        current_score = 3u;
                    }
                    else if (has_transfer_support) {
                        // Supported operations: GRAPHICS | TRANSFER
                        current_score = 2u;
                    }
                    else {
                        // Supported operations: GRAPHICS
                        current_score = 0u;
                    }
                    
                    if (current_score > highest_score) {
                        // Found a more suitable graphics queue family.
                        graphics_queue_family.index = static_cast<i32>(i);
                        graphics_queue_family.has_presentation_support = has_presentation_support;
                        graphics_queue_family.has_compute_support = has_compute_support;
                        graphics_queue_family.has_transfer_support = has_transfer_support;
                        graphics_queue_family.num_available_queues = family.queueCount;
                    }
                }
//            }
//        }
//            else if (compute_support_requested) {
//                // A dedicated presentation queue was requested (will be handled later in the selection process), so presentation support is optional for the graphics family.
//                if (transfer_support_requested) {
//                    configuration = 1u;
//
//                    for (u32 i = 0u; i < queue_family_count; ++i) {
//                        const VkQueueFamilyProperties& family = queue_families[i];
//                        bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
//                        bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
//                        bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
//
//                        #if defined(VKS_PLATFORM_WINDOWS)
//                        bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
//                        #endif
//
//                        // Scoring of queue family based on supported operations:
//                        // 3. GRAPHICS | COMPUTE | TRANSFER
//                        // 2. GRAPHICS | TRANSFER
//                        // 1. GRAPHICS
//                        u32 current_score = 0u;
//
//                        if (!has_graphics_support) {
//                            continue;
//                        }
//
//                        if (has_compute_support) {
//                            current_score = 3u;
//                        }
//                        else if (has_transfer_support) {
//                            current_score = 2u;
//                        }
//                        else {
//                            current_score = 1u;
//                        }
//
//                        if (current_score > highest_score) {
//                            // Found a more suitable graphics queue family.
//                            graphics_queue_family.index = static_cast<i32>(i);
//                            graphics_queue_family.has_presentation_support = has_presentation_support; // Optional.
//                            graphics_queue_family.has_compute_support = has_compute_support;
//                            graphics_queue_family.has_transfer_support = has_transfer_support;
//                            graphics_queue_family.num_available_queues = family.queueCount;
//                        }
//                    }
//                }
//                else {
//                    // Requested: GRAPHICS | COMPUTE
//                    configuration = 2u;
//
//                    for (u32 i = 0u; i < queue_family_count; ++i) {
//                        const VkQueueFamilyProperties& family = queue_families[i];
//                        bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
//                        bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
//                        bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
//
//                        #if defined(VKS_PLATFORM_WINDOWS)
//                        bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
//                        #endif
//
//                        // Scoring of queue family based on supported operations:
//                        // 3. GRAPHICS | COMPUTE | TRANSFER
//                        // 2. GRAPHICS | TRANSFER
//                        // 1. GRAPHICS
//                        u32 current_score = 0u;
//
//                        if (!has_graphics_support) {
//                            continue;
//                        }
//
//                        if (has_compute_support) {
//                            current_score = 3u;
//                        }
//                        else if (has_transfer_support) {
//                            current_score = 2u;
//                        }
//                        else {
//                            current_score = 1u;
//                        }
//
//                        if (current_score > highest_score) {
//                            // Found a more suitable graphics queue family.
//                            graphics_queue_family.index = static_cast<i32>(i);
//                            graphics_queue_family.has_presentation_support = has_presentation_support; // Optional.
//                            graphics_queue_family.has_compute_support = has_compute_support;
//                            graphics_queue_family.has_transfer_support = has_transfer_support;
//                            graphics_queue_family.num_available_queues = family.queueCount;
//                        }
//                    }
//                }
//            }
//            else if (transfer_support_requested) {
//                // Requested: GRAPHICS | TRANSFER
//            }
//            else {
//                // Requested: GRAPHICS
//            }
//        }
//
//
//        if (graphics_support_requested) {
//            // Select a queue family for graphics-related operations.
//            for (u32 i = 0u; i < queue_families.size(); ++i) {
//                const VkQueueFamilyProperties& family = queue_families[i];
//                bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
//                bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
//                bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
//
//                #if defined(VKS_PLATFORM_WINDOWS)
//                    bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
//                #endif
//
//                if (!has_graphics_support) {
//                    continue;
//                }
//
//                if (presentation_support_requested && !dedicated_presentation_queue_requested) {
//                    // If a dedicated presentation queue is requested, support for presentation operations is optional for the graphics family.
//
//                    if (compute_support_requested) {
//                        // Transfer operations are implicitly supported on any family that supports graphics and compute operations.
//
//                        if (transfer_support_requested) {
//                            // Scoring of graphics queue family (based on requested operations):
//                            // 6. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
//                            // 5. GRAPHICS | PRESENTATION | TRANSFER
//                            // 4. GRAPHICS | PRESENTATION
//                            // 3. GRAPHICS | COMPUTE | TRANSFER
//                            // 2. GRAPHICS | TRANSFER
//                            // 1. GRAPHICS
//                            u32 current_score = 0u;
//
//                            // Note: TRANSFER operations are implicitly allowed on a queue that supports both GRAPHICS and COMPUTE operations.
//                            if (has_graphics_support) {
//                                if (has_presentation_support) {
//                                    if (has_compute_support) {
//                                        // GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
//                                        current_score = 6u;
//                                    }
//                                    else if (has_transfer_support) {
//                                        // GRAPHICS | PRESENTATION | TRANSFER
//                                        // Assume there exists another queue family that supports compute operations.
//                                        current_score = 5u;
//                                    }
//                                    else {
//                                        // GRAPHICS | PRESENTATION
//                                        // Assume there exist other queue families that support both compute and transfer operations.
//                                        current_score = 4u;
//                                    }
//                                }
//                                else if (has_compute_support) {
//                                    // GRAPHICS | COMPUTE | TRANSFER
//                                    // Assume there exists a
//                                    current_score = 3u;
//                                }
//                                else if (has_transfer_support) {
//                                    // GRAPHICS | TRANSFER
//                                    current_score = 2u;
//                                }
//                                else {
//                                    // GRAPHICS
//                                    current_score = 1u;
//                                }
//                            }
//                        }
//                        else {
//
//                        }
//
//                        if (has_presentation_support) {
//                            if (has_compute_support) {
//
//                            }
//                            else if (has_transfer_support) {
//
//                            }
//                            else {
//
//                            }
//                        }
//
//
//
//                    }
//                    else if (transfer_support_requested) {
//
//                    }
//                    else {
//
//                    }
//                }
//
//                if (compute_support_requested) {
//                    // Request for compute operation support implies synchronization between compute and graphics families, as they will be submitted to the same queue.
//                    // Request for dedicated compute queue does not pertain to this family and will be handled later.
//                }
//
//                if (transfer_support_requested) {
//                    // Request for transfer operation support implies synchronization between the transfer and graphics families, as they will be submitted to the same queue.
//                }
//
//
//
//                if (current_score > highest_score) {
//                    highest_score = current_score;
//                    graphics_queue_family.index = static_cast<i32>(i);
//                    graphics_queue_family.num_available_queues = family.queueCount;
//                }
//            }
//
//            switch (highest_score) {
//                case 0u:
//                case 1u:
//                    break;
//
//                case 2u:
//                    graphics_queue_family.has_transfer_support = true;
//                    break;
//
//                case 3u:
//                    graphics_queue_family.has_compute_support = true;
//                    graphics_queue_family.has_transfer_support = true;
//                    break;
//
//                case 4u:
//                    graphics_queue_family.has_presentation_support = true;
//                    break;
//
//                case 5u:
//                    graphics_queue_family.has_presentation_support = true;
//                    graphics_queue_family.has_transfer_support = true;
//                    break;
//
//                case 6u:
//                    graphics_queue_family.has_presentation_support = true;
//                    graphics_queue_family.has_compute_support = true;
//                    graphics_queue_family.has_transfer_support = true;
//                    break;
//            }
//        }
 
    
        ComputeQueueFamily compute_queue_family {
            .index = QueueFamilies::INVALID_QUEUE_FAMILY_INDEX,
            .num_available_queues = 0u,
            .async = false
        };
    
        TransferQueueFamily transfer_queue_family {
            .index = QueueFamilies::INVALID_QUEUE_FAMILY_INDEX,
            .num_available_queues = 0u,
            .async = false
        };
    
        u32 highest_compute_score = 0u;
        u32 highest_transfer_score = 0u;
    
        // Select queue families for compute and transfer operations.
        for (u32 i = 0u; i < queue_families.size(); ++i) {
            const VkQueueFamilyProperties& family = queue_families[i];
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
            bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
        
            if (i == graphics_queue_family.index) {
                // Do not share compute or transfer queue family with the graphics family - if available, this queue family will instead be used for synchronous compute / transfer operations
                // and will be selected as the default if there are no other families available.
                continue;
            }
        
            // The ideal asynchronous compute / transfer queue families come from different queue families and do not share bandwidth with graphics / presentation queue families.
            // Otherwise, prefer sharing queue bandwidth with other asynchronous operations.
        
            // Scoring of asynchronous compute queue family (based on supported operations):
            // 4. COMPUTE
            // 3. COMPUTE | TRANSFER
            // 2. PRESENTATION | COMPUTE
            // 1. PRESENTATION | COMPUTE | TRANSFER
            u32 current_compute_score = 0u;
        
            // Scoring for asynchronous transfer queue family (based on supported operations):
            // 4. TRANSFER
            // 3. COMPUTE | TRANSFER
            // 2. PRESENTATION | TRANSFER
            // 1. PRESENTATION | COMPUTE | TRANSFER
            u32 current_transfer_score = 0u;
        
            if (has_presentation_support) {
                if (has_compute_support) {
                    if (has_transfer_support) {
                        // PRESENTATION | COMPUTE | TRANSFER
                        current_compute_score = 1u;
                        current_transfer_score = 1u;
                    }
                    else {
                        // PRESENTATION | COMPUTE
                        current_compute_score = 2u;
                    }
                }
                else if (has_transfer_support) {
                    // PRESENTATION | TRANSFER
                    current_transfer_score = 2u;
                }
            }
            else if (has_compute_support) {
                if (has_transfer_support) {
                    // COMPUTE | TRANSFER
                    current_compute_score = 3u;
                    current_transfer_score = 3u;
                }
                else {
                    // COMPUTE
                    current_compute_score = 4u;
                }
            }
            else if (has_transfer_support) {
                // TRANSFER
                current_transfer_score = 4u;
            }
        
            if (current_compute_score > highest_compute_score) {
                highest_compute_score = current_compute_score;
                compute_queue_family.index = static_cast<i32>(i);
                compute_queue_family.num_available_queues = family.queueCount;
            }
        
            if (current_transfer_score > highest_transfer_score) {
                highest_transfer_score = current_transfer_score;
                transfer_queue_family.index = static_cast<i32>(i);
                transfer_queue_family.num_available_queues = family.queueCount;
            }
        }
    
        if (compute_queue_family.index == QueueFamilies::INVALID_QUEUE_FAMILY_INDEX) {
            // Found no queue families that support asynchronous compute operations - default to graphics queue family and synchronized compute operations, if available.
            if (graphics_queue_family.has_compute_support) {
                compute_queue_family.index = graphics_queue_family.index;
                compute_queue_family.async = false;
                compute_queue_family.num_available_queues = graphics_queue_family.num_available_queues;
            }
        }
        else {
            compute_queue_family.async = true;
        }
    
        if (transfer_queue_family.index == QueueFamilies::INVALID_QUEUE_FAMILY_INDEX) {
            // Found no queue families that support asynchronous transfer operations - default to graphics queue family and synchronized transfer operations, if available.
            if (graphics_queue_family.has_transfer_support) {
                transfer_queue_family.index = graphics_queue_family.index;
                transfer_queue_family.async = false;
                transfer_queue_family.num_available_queues = graphics_queue_family.num_available_queues;
            }
        }
        else {
            transfer_queue_family.async = true;
        }
    
        PresentationQueueFamily presentation_queue_family {
            .index = QueueFamilies::INVALID_QUEUE_FAMILY_INDEX,
            .num_available_queues = 0u
        };
    
        if (graphics_queue_family.has_presentation_support) {
            // Prefer a presentation queue family that also supports graphics operations for performance reasons.
            presentation_queue_family.index = graphics_queue_family.index;
            presentation_queue_family.num_available_queues = graphics_queue_family.num_available_queues;
        }
        else {
            highest_score = 0u;
        
            // Select a queue family for presentation operations.
            for (u32 i = 0u; i < queue_families.size(); ++i) {
                const VkQueueFamilyProperties& family = queue_families[i];
                bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
                bool shares_compute_support = ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) && (compute_queue_family.index == i);
                bool shares_transfer_support = ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) && (transfer_queue_family.index == i);
            
                // The ideal presentation queue family, if not supported by the graphics queue family, is standalone and independent of the selected compute / transfer queue families.
                // Otherwise, prefer sharing queue bandwidth with other asynchronous operations on the selected compute / transfer queue families.
                // Scoring of presentation queue family (based on supported operations):
                // 4. PRESENTATION
                // 3. PRESENTATION | TRANSFER
                // 2. PRESENTATION | COMPUTE
                // 1. PRESENTATION | COMPUTE | TRANSFER
            
                u32 current_score = 0u;
            
                if (has_presentation_support) {
                    if (shares_compute_support) {
                        if (shares_transfer_support) {
                            // PRESENTATION | COMPUTE | TRANSFER
                            current_score = 1u;
                        }
                        else {
                            // PRESENTATION | COMPUTE
                            current_score = 2u;
                        }
                    }
                    else if (shares_transfer_support) {
                        // PRESENTATION | TRANSFER
                        current_score = 3u;
                    }
                    else {
                        // PRESENTATION
                        current_score = 4u;
                    }
                }
            
                if (current_score > highest_score) {
                    highest_score = current_score;
                    presentation_queue_family.index = static_cast<i32>(i);
                    presentation_queue_family.num_available_queues = family.queueCount;
                }
            }
        }
    
        return QueueFamilies {
            .graphics_queue_family = graphics_queue_family,
            .presentation_queue_family = presentation_queue_family,
            .compute_queue_family = compute_queue_family,
            .transfer_queue_family = transfer_queue_family
        };
    }
    
    VulkanDevice::Data::GraphicsQueueFamily VulkanDevice::Data::select_graphics_queue_family(const std::vector<VkQueueFamilyProperties>& queue_families) const {
        GraphicsQueueFamily graphics_queue_family {
            .index = QueueFamilies::INVALID_QUEUE_FAMILY_INDEX,
            .num_available_queues = 0u,
            .has_presentation_support = false,
            .has_compute_support = false,
            .has_transfer_support = false
        };
    
        // Graphics queue will always be dedicated.
        bool graphics_support_requested = test(m_supported_queues, VulkanQueue::Operation::GRAPHICS);

        // It would be better for application performance to combine the graphics and presentation queue if a dedicated presentation queue is not requested.
        bool presentation_support_requested = test(m_supported_queues, VulkanQueue::Operation::PRESENTATION) && test(m_dedicated_queues, VulkanQueue::Operation::PRESENTATION);

        // Compute / transfer operations will be submitted synchronously to the graphics queue if a dedicated compute / transfer queue is not requested. Asynchronous operations require a dedicated compute / transfer queue.
        bool compute_support_requested = test(m_supported_queues, VulkanQueue::Operation::COMPUTE);
        bool transfer_support_requested = test(m_supported_queues, VulkanQueue::Operation::TRANSFER);
        
        if (!graphics_support_requested) {
            return graphics_queue_family;
        }
        
        u32 highest_score = 0u;
        
        for (u32 i = 0u; i < queue_families.size(); ++i) {
            const VkQueueFamilyProperties& family = queue_families[i];
            bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
    
            #if defined(VKS_PLATFORM_WINDOWS)
                bool has_presentation_support = static_cast<bool>(fp_vk_get_physical_device_presentation_support(device_handle, i));
            #endif
    
            if (!has_graphics_support) {
                continue;
            }
    
            u32 current_score = 0u;
            
            // Note: transfer operations are implicitly supported on queue families where graphics and compute operations are supported.
    
            if (presentation_support_requested) {
                if (compute_support_requested) {
                    if (transfer_support_requested) {
                        // Requested: GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
                        
                        // Scoring of queue family based on supported operations:
                        // 6. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
                        // 5. GRAPHICS | PRESENTATION | TRANSFER
                        // 4. GRAPHICS | PRESENTATION
                        // 3. GRAPHICS | COMPUTE | TRANSFER
                        // 2. GRAPHICS | TRANSFER
                        // 1. GRAPHICS
                        if (has_presentation_support) {
                            if (has_compute_support) {
                                current_score = 6u;
                            }
                            else if (has_transfer_support) {
                                current_score = 5u;
                            }
                            else {
                                current_score = 4u;
                            }
                        }
                        else if (has_compute_support) {
                            current_score = 3u;
                        }
                        else if (has_transfer_support) {
                            current_score = 2u;
                        }
                        else {
                            current_score = 1u;
                        }
                    }
                    else {
                        // Requested: GRAPHICS | PRESENTATION | COMPUTE
                        
                        // Scoring of queue family based on supported operations:
                        // 4. GRAPHICS | PRESENTATION | COMPUTE | (TRANSFER)
                        // 3. GRAPHICS | PRESENTATION or GRAPHICS | PRESENTATION | TRANSFER
                        // 2. GRAPHICS | COMPUTE | (TRANSFER)
                        // 1. GRAPHICS or GRAPHICS | TRANSFER
                        if (has_presentation_support) {
                            if (has_compute_support) {
                                current_score = 4u;
                            }
                            else {
                                current_score = 3u;
                            }
                        }
                        else if (has_compute_support) {
                            current_score = 2u;
                        }
                        else {
                            current_score = 1u;
                        }
                    }
                }
                else if (transfer_support_requested) {
                    // Requested: GRAPHICS | PRESENTATION | TRANSFER
    
                    // Scoring of queue family based on supported operations:
                    // 4. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER or GRAPHICS | PRESENTATION | TRANSFER
                    // 3. GRAPHICS | PRESENTATION
                    // 2. GRAPHICS | COMPUTE | TRANSFER or GRAPHICS | TRANSFER
                    // 1. GRAPHICS
                    if (has_presentation_support) {
                        if (has_compute_support || has_transfer_support) {
                            current_score = 4u;
                        }
                        else {
                            current_score = 3u;
                        }
                    }
                    else if (has_compute_support || has_transfer_support) {
                        current_score = 2u;
                    }
                    else {
                        current_score = 1u;
                    }
                }
                else {
                    // Requested: GRAPHICS | PRESENTATION
    
                    // Scoring of queue family based on supported operations:
                    // 2. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER or GRAPHICS | PRESENTATION | TRANSFER or GRAPHICS | PRESENTATION
                    // 1. GRAPHICS | COMPUTE | TRANSFER or GRAPHICS | TRANSFER or GRAPHICS
                    if (has_presentation_support) {
                        current_score = 2u;
                    }
                    else {
                        current_score = 1u;
                    }
                }
            }
            else if (compute_support_requested) {
                if (transfer_support_requested) {
                    // Requested: GRAPHICS | COMPUTE | TRANSFER
    
                    // Scoring of queue family based on supported operations:
                    // 3. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER or GRAPHICS | COMPUTE | TRANSFER
                    // 2. GRAPHICS | PRESENTATION | TRANSFER or GRAPHICS | TRANSFER
                    // 1. GRAPHICS | PRESENTATION or GRAPHICS
                    if (has_compute_support) {
                        if (has_transfer_support) {
                            current_score = 3u;
                        }
                    }
                    else if (has_transfer_support) {
                        current_score = 2u;
                    }
                    else {
                        current_score = 1u;
                    }
                }
                else {
                    // Requested: GRAPHICS | COMPUTE
    
                    // Scoring of queue family based on supported operations:
                    // 2. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER or GRAPHICS | COMPUTE | TRANSFER
                    // 1. GRAPHICS | PRESENTATION | TRANSFER or GRAPHICS | PRESENTATION or GRAPHICS | TRANSFER or GRAPHICS
                    if (has_compute_support) {
                        current_score = 2u;
                    }
                    else {
                        current_score = 1u;
                    }
                }
            }
            else if (transfer_support_requested) {
                // Requested: GRAPHICS | TRANSFER
    
                // Scoring of queue family based on supported operations:
                // 2. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER or GRAPHICS | PRESENTATION | TRANSFER or GRAPHICS | COMPUTE | TRANSFER or GRAPHICS | TRANSFER
                // 1. GRAPHICS | PRESENTATION or GRAPHICS
                if (has_transfer_support) {
                    current_score = 2u;
                }
                else {
                    current_score = 1u;
                }
            }
            else {
                // Requested: GRAPHICS
                current_score = 1u;
            }
            
            if (current_score > highest_score) {
                highest_score = current_score;
                
                graphics_queue_family.index = static_cast<i32>(i);
                graphics_queue_family.has_presentation_support = has_presentation_support;
                graphics_queue_family.has_compute_support = has_compute_support;
                graphics_queue_family.has_transfer_support = has_transfer_support;
                graphics_queue_family.num_available_queues = family.queueCount;
            }
        }
        
        return graphics_queue_family;
    }
    
    bool VulkanDevice::Data::verify_device_queue_family_support(const VulkanDevice::Data::QueueFamilies& queue_families) const {
        if ((m_supported_queues & VulkanQueue::Operation::GRAPHICS) == VulkanQueue::Operation::GRAPHICS) {
            if (!queue_families.is_operation_supported(VulkanQueue::Operation::GRAPHICS)) {
                // Unable to find a queue family that supports graphics operations.
                return false;
            }
        }
        
        // The existence of a graphics queue family implies the existence of a dedicated graphics queue.
        // if ((m_dedicated_queues & VulkanQueue::Operation::GRAPHICS) == VulkanQueue::Operation::GRAPHICS) {
        //     ...
        // }
    
        if ((m_supported_queues & VulkanQueue::Operation::PRESENTATION) == VulkanQueue::Operation::PRESENTATION) {
            if (!queue_families.is_operation_supported(VulkanQueue::Operation::PRESENTATION)) {
                // Unable to find a queue family that supports presentation operations.
                return false;
            }
        }

        if ((m_dedicated_queues & VulkanQueue::Operation::PRESENTATION) == VulkanQueue::Operation::PRESENTATION) {
            if (!queue_families.has_dedicated_queue_family(VulkanQueue::Operation::PRESENTATION)) {
                // Unable to find a dedicated queue family for presentation operations.
                return false;
            }
        }
    
        if ((m_supported_queues & VulkanQueue::Operation::COMPUTE) == VulkanQueue::Operation::COMPUTE) {
            if (!queue_families.is_operation_supported(VulkanQueue::Operation::COMPUTE)) {
                // Unable to find a queue family that supports compute operations.
                return false;
            }
        }
        
        if ((m_dedicated_queues & VulkanQueue::Operation::COMPUTE) == VulkanQueue::Operation::COMPUTE) {
            if (!queue_families.has_dedicated_queue_family(VulkanQueue::Operation::COMPUTE)) {
                // Unable to find a dedicated queue family for async compute operations.
                return false;
            }
        }
    
        if ((m_supported_queues & VulkanQueue::Operation::TRANSFER) == VulkanQueue::Operation::TRANSFER) {
            if (!queue_families.is_operation_supported(VulkanQueue::Operation::TRANSFER)) {
                // Unable to find a queue family that supports transfer operations.
                return false;
            }
        }
    
        if ((m_dedicated_queues & VulkanQueue::Operation::TRANSFER) == VulkanQueue::Operation::TRANSFER) {
            if (!queue_families.has_dedicated_queue_family(VulkanQueue::Operation::TRANSFER)) {
                // Unable to find a dedicated queue family for presentation operations.
                return false;
            }
        }
        
        return true;
    }

    VulkanDevice::VulkanDevice() {
    }
    
    VulkanDevice::~VulkanDevice() {
    }
    
    VulkanDevice::operator VkPhysicalDevice() const {
        std::shared_ptr<const VulkanDevice::Data> data = std::reinterpret_pointer_cast<const VulkanDevice::Data>(shared_from_this());
        return data->m_device;
    }
    
    VulkanDevice::operator VkDevice() const {
        std::shared_ptr<const VulkanDevice::Data> data = std::reinterpret_pointer_cast<const VulkanDevice::Data>(shared_from_this());
        return data->m_handle;
    }
    
    VulkanDevice::Builder::Builder(std::shared_ptr<VulkanInstance> instance) : m_device(std::make_shared<VulkanDevice::Data>(std::move(instance))) {
    }
    
    VulkanDevice::Builder::~Builder() {
    }

    std::shared_ptr<VulkanDevice> VulkanDevice::Builder::build() {
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
        if (!data->build()) {
            return std::shared_ptr<VulkanDevice>(); // nullptr
        }
    
        std::shared_ptr<VulkanInstance> instance = data->m_instance;
    
        // Builder should not maintain ownership over created devices, and allow for subsequent calls to 'build' to generate valid standalone devices.
        std::shared_ptr<VulkanDevice> device = std::move(m_device);
        m_device = std::make_shared<VulkanDevice::Data>(instance); // Ensure instance validity after transferring ownership.
        return std::move(device);
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_features(const Features& requested) {
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
        data->m_requested_features = VkPhysicalDeviceFeatures {
            .robustBufferAccess = requested.robust_buffer_access,
            .fullDrawIndexUint32 = requested.full_draw_index_uint32,
            .imageCubeArray = requested.image_cube_array,
            .independentBlend = requested.independent_blend,
            .geometryShader = requested.geometry_shader,
            .tessellationShader = requested.tesselation_shader,
            .sampleRateShading = requested.sample_rate_shading,
            .dualSrcBlend = requested.dual_src_blend,
            .logicOp = requested.logic_operations,
            .multiDrawIndirect = requested.multi_draw_indirect,
            .drawIndirectFirstInstance = requested.draw_indirect_first_instance,
            .depthClamp = requested.depth_clamp,
            .depthBiasClamp = requested.depth_bias_clamp,
            .fillModeNonSolid = requested.fill_mode_non_solid,
            .depthBounds = requested.depth_bounds,
            .wideLines = requested.wide_lines,
            .largePoints = requested.large_points,
            .alphaToOne = requested.alpha_to_one,
            .multiViewport = requested.multi_viewport,
            .samplerAnisotropy = requested.sampler_anisotropy,
            .textureCompressionETC2 = requested.texture_compression_etc2,
            .textureCompressionASTC_LDR = requested.texture_compression_astc_ldr,
            .textureCompressionBC = requested.texture_compression_bc,
            .occlusionQueryPrecise = requested.occlusion_query_precise,
            .pipelineStatisticsQuery = requested.pipeline_statistics_query,
            .vertexPipelineStoresAndAtomics = requested.vertex_pipeline_stores_and_atomics,
            .fragmentStoresAndAtomics = requested.fragment_stores_and_atomics,
            .shaderTessellationAndGeometryPointSize = requested.shader_tesselation_and_geometry_point_size,
            .shaderImageGatherExtended = requested.shader_image_gather_extended,
            .shaderStorageImageExtendedFormats = requested.shader_storage_image_extended_formats,
            .shaderStorageImageMultisample = requested.shader_storage_multisample,
            .shaderStorageImageReadWithoutFormat = requested.shader_storage_image_read_without_format,
            .shaderStorageImageWriteWithoutFormat = requested.shader_storage_image_write_without_format,
            .shaderUniformBufferArrayDynamicIndexing = requested.shader_uniform_buffer_array_dynamic_indexing,
            .shaderSampledImageArrayDynamicIndexing = requested.shader_sampled_image_array_dynamic_indexing,
            .shaderStorageBufferArrayDynamicIndexing = requested.shader_storage_buffer_array_dynamic_indexing,
            .shaderStorageImageArrayDynamicIndexing = requested.shader_storage_image_array_dynamic_indexing,
            .shaderClipDistance = requested.shader_clip_distance,
            .shaderCullDistance = requested.shader_cull_distance,
            .shaderFloat64 = requested.shader_float64,
            .shaderInt64 = requested.shader_int64,
            .shaderInt16 = requested.shader_int16,
            .shaderResourceResidency = requested.shader_resource_residency,
            .shaderResourceMinLod = requested.shader_resource_min_lod,
            .sparseBinding = requested.sparse_binding,
            .sparseResidencyBuffer = requested.sparse_residency_buffer,
            .sparseResidencyImage2D = requested.sparse_residency_image2d,
            .sparseResidencyImage3D = requested.sparse_residency_image3d,
            .sparseResidency2Samples = requested.sparse_residency_2_samples,
            .sparseResidency4Samples = requested.sparse_residency_4_samples,
            .sparseResidency8Samples = requested.sparse_residency_8_samples,
            .sparseResidency16Samples = requested.sparse_residency_16_samples,
            .sparseResidencyAliased = requested.sparse_residency_aliased,
            .variableMultisampleRate = requested.variable_multisample_rate,
            .inheritedQueries = requested.inherited_queries
        };
        return *this;
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_supported_queue(VulkanQueue::Operation operation) {
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
        data->m_supported_queues |= operation;
        
        return *this;
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_dedicated_queue(VulkanQueue::Operation operation) {
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
        
        // Dedicated operations imply the operation is supported.
        data->m_supported_queues |= operation;
        data->m_dedicated_queues |= operation;
        
        return *this;
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }

        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
        
        // Ensure requested extension does not already exist.
        bool found = false;

        for (const char* extension : data->m_extensions) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            // Only add the first instance of the extension.
            data->m_extensions.emplace_back(requested);
        }

        return *this;
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
    
        for (std::size_t i = 0u; i < data->m_extensions.size(); ++i) {
            const char* extension = data->m_extensions[i];

            if (strcmp(requested, extension) == 0) {
                // Found extension.
                std::swap(data->m_extensions[i], data->m_extensions[data->m_extensions.size() - 1u]); // Swap idiom.
                data->m_extensions.pop_back();

                // Extension is guaranteed to be present in the list only once.
                break;
            }
        }

        return *this;
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
    
        // Ensure requested validation layer does not already exist.
        bool found = false;
    
        for (const char* layer : data->m_validation_layers) {
            if (strcmp(requested, layer) == 0) {
                found = true;
                break;
            }
        }
    
        if (!found) {
            data->m_validation_layers.emplace_back(requested);
        }
    
        return *this;
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
    
        for (u32 i = 0u; i < data->m_validation_layers.size(); ++i) {
            const char* layer = data->m_validation_layers[i];
        
            if (strcmp(requested, layer) == 0) {
                // Found validation layer.
                std::swap(data->m_validation_layers[i], data->m_validation_layers[data->m_validation_layers.size() - 1u]); // Swap idiom.
                data->m_validation_layers.pop_back();
            
                // Validation layer is guaranteed to be present in the list only once.
                break;
            }
        }
    
        return *this;
    }

    VulkanDevice::Builder& VulkanDevice::Builder::with_debug_name(const char* name) {
        if (strlen(name) > 0u) {
            std::shared_ptr<VulkanDevice::Data> data = std::reinterpret_pointer_cast<VulkanDevice::Data>(m_device);
            data->m_name = name;
        }

        return *this;
    }

}
