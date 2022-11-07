
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

namespace vks {

    VulkanPhysicalDevice::Selector::Selector(const VulkanInstance* instance) : instance_(instance),
                                                                               fp_vk_enumerate_physical_devices_(reinterpret_cast<typeof(fp_vk_enumerate_physical_devices_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumeratePhysicalDevices"))),
                                                                               fp_vk_get_physical_device_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties"))),
                                                                               fp_vk_get_physical_device_features_(reinterpret_cast<typeof(fp_vk_get_physical_device_features_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceFeatures"))),
                                                                               fp_vk_get_physical_device_queue_family_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_queue_family_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceQueueFamilyProperties"))),
                                                                               fp_vk_enumerate_device_extension_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_extension_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumerateDeviceExtensionProperties"))),
                                                                               fp_vk_enumerate_device_layer_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_layer_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumerateDeviceLayerProperties"))),
                                                                               selector_(nullptr)
                                                                               {
        ASSERT(fp_vk_enumerate_physical_devices_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_properties_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_features_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_queue_family_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_extension_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_layer_properties_ != nullptr, "");
    }
    
    VulkanPhysicalDevice::Selector::~Selector() {
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_selector_function(int (*selector)(const Properties&, const Features&)) {
        if (selector) {
            selector_ = selector;
        }
        
        return *this;
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_debug_name(const char* name) {
    }
    
    VulkanPhysicalDevice VulkanPhysicalDevice::Selector::select() {
        // Enumerate physical devices.
        std::uint32_t physical_device_count = 0u;
        VkResult result = fp_vk_enumerate_physical_devices_(*instance_, &physical_device_count, nullptr);

        if (physical_device_count == 0u) {
            throw VulkanException("ERROR: Failed to find GPU with Vulkan support.");
        }

        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        result = fp_vk_enumerate_physical_devices_(*instance_, &physical_device_count, physical_devices.data());

        for (const VkPhysicalDevice& physical_device : physical_devices) {
            Properties physical_device_properties = query_properties(physical_device);
            Features physical_device_features = query_features(physical_device);
            
            int score = selector_(physical_device_properties, physical_device_features);
        }
    }
    
    VulkanPhysicalDevice::Properties VulkanPhysicalDevice::Selector::query_properties(VkPhysicalDevice handle) const {
        VkPhysicalDeviceProperties physical_device_properties { };
        fp_vk_get_physical_device_properties_(handle, &physical_device_properties);
        return { physical_device_properties };
    }
    
    VulkanPhysicalDevice::Features VulkanPhysicalDevice::Selector::query_features(VkPhysicalDevice handle) const {
        VkPhysicalDeviceFeatures physical_device_features { };
        fp_vk_get_physical_device_features_(handle, &physical_device_features);
        return { physical_device_features };
    }
    
    VulkanPhysicalDevice::Properties::Properties(const VkPhysicalDeviceProperties& properties) : api_version({
                                                                                                     .major = VK_API_VERSION_MAJOR(properties.apiVersion),
                                                                                                     .minor = VK_API_VERSION_MINOR(properties.apiVersion),
                                                                                                     .patch = VK_API_VERSION_PATCH(properties.apiVersion)
                                                                                                 }),
                                                                                                 type(Type::OTHER),
                                                                                                 limits(properties.limits),
                                                                                                 driver_version(properties.driverVersion),
                                                                                                 device_name(std::string(properties.deviceName))
                                                                                                 {
        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                type = VulkanPhysicalDevice::Properties::Type::INTEGRATED_GPU;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                type = VulkanPhysicalDevice::Properties::Type::DISCRETE_GPU;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                type = VulkanPhysicalDevice::Properties::Type::VIRTUAL_GPU;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                type = VulkanPhysicalDevice::Properties::Type::CPU;
        }
    }
    
    VulkanPhysicalDevice::Properties::~Properties() = default;
    
    VulkanPhysicalDevice::Properties::Limits::Limits(const VkPhysicalDeviceLimits& limits) : max_image_dimension_1d(limits.maxImageDimension1D),
                                                                                             max_image_dimension_2d(limits.maxImageDimension2D),
                                                                                             max_image_dimension_3d(limits.maxImageDimension3D),
                                                                                             max_image_dimension_cube(limits.maxImageDimensionCube),
                                                                                             max_image_array_layers(limits.maxImageArrayLayers),
                                                                                             max_texel_buffer_elements(limits.maxTexelBufferElements),
                                                                                             max_uniform_buffer_range(limits.maxUniformBufferRange),
                                                                                             max_storage_buffer_range(limits.maxStorageBufferRange),
                                                                                             max_push_constants_size(limits.maxPushConstantsSize),
                                                                                             max_memory_allocation_count(limits.maxMemoryAllocationCount),
                                                                                             max_sampler_allocation_count(limits.maxSamplerAllocationCount),
                                                                                             buffer_image_granularity(limits.bufferImageGranularity),
                                                                                             sparse_address_space_size(limits.sparseAddressSpaceSize),
                                                                                             max_bound_descriptor_sets(limits.maxBoundDescriptorSets),
                                                                                             max_per_stage_descriptor_samplers(limits.maxPerStageDescriptorSamplers),
                                                                                             max_per_stage_descriptor_uniform_buffers(limits.maxPerStageDescriptorUniformBuffers),
                                                                                             max_per_stage_descriptor_storage_buffers(limits.maxPerStageDescriptorStorageBuffers),
                                                                                             max_per_stage_descriptor_sampled_images(limits.maxPerStageDescriptorSampledImages),
                                                                                             max_per_stage_descriptor_storage_images(limits.maxPerStageDescriptorStorageImages),
                                                                                             max_per_stage_descriptor_input_attachments(limits.maxPerStageDescriptorInputAttachments),
                                                                                             max_per_stage_resources(limits.maxPerStageResources),
                                                                                             max_descriptor_set_samplers(limits.maxDescriptorSetSamplers),
                                                                                             max_descriptor_set_uniform_buffers(limits.maxDescriptorSetUniformBuffers),
                                                                                             max_descriptor_set_uniform_buffers_dynamic(limits.maxDescriptorSetUniformBuffersDynamic),
                                                                                             max_descriptor_set_storage_buffers(limits.maxDescriptorSetStorageBuffers),
                                                                                             max_descriptor_set_storage_buffers_dynamic(limits.maxDescriptorSetStorageBuffersDynamic),
                                                                                             max_descriptor_set_sampled_images(limits.maxDescriptorSetSampledImages),
                                                                                             max_descriptor_set_storage_images(limits.maxDescriptorSetStorageBuffers),
                                                                                             max_descriptor_set_input_attachments(limits.maxDescriptorSetInputAttachments),
                                                                                             max_vertex_input_attributes(limits.maxVertexInputAttributes),
                                                                                             max_vertex_input_bindings(limits.maxVertexInputBindings),
                                                                                             max_vertex_input_attribute_offset(limits.maxVertexInputAttributeOffset),
                                                                                             max_vertex_input_binding_stride(limits.maxVertexInputBindingStride),
                                                                                             max_vertex_output_components(limits.maxVertexOutputComponents),
                                                                                             max_tesselation_generation_level(limits.maxTessellationGenerationLevel),
                                                                                             max_tesselation_patch_size(limits.maxTessellationPatchSize),
                                                                                             max_tesselation_control_per_vertex_input_components(limits.maxTessellationControlPerVertexInputComponents),
                                                                                             max_tesselation_control_per_vertex_output_components(limits.maxTessellationControlPerVertexOutputComponents),
                                                                                             max_tesselation_control_per_patch_output_components(limits.maxTessellationControlPerPatchOutputComponents),
                                                                                             max_tesselation_control_total_output_components(limits.maxTessellationControlTotalOutputComponents),
                                                                                             max_tesselation_evaluation_input_components(limits.maxTessellationEvaluationInputComponents),
                                                                                             max_tesselation_evaluation_output_components(limits.maxTessellationEvaluationOutputComponents),
                                                                                             max_geometry_shader_invocations(limits.maxGeometryShaderInvocations),
                                                                                             max_geometry_input_components(limits.maxGeometryInputComponents),
                                                                                             max_geometry_output_components(limits.maxGeometryOutputComponents),
                                                                                             max_geometry_output_vertices(limits.maxGeometryOutputVertices),
                                                                                             max_geometry_total_output_components(limits.maxGeometryTotalOutputComponents),
                                                                                             max_fragment_input_components(limits.maxFragmentInputComponents),
                                                                                             max_fragment_output_attachments(limits.maxFragmentOutputAttachments),
                                                                                             max_fragment_dual_src_attachments(limits.maxFragmentDualSrcAttachments),
                                                                                             max_fragment_combined_output_resources(limits.maxFragmentCombinedOutputResources),
                                                                                             max_compute_shared_memory_size(limits.maxComputeSharedMemorySize),
                                                                                             max_compute_work_group_count { limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2] },
                                                                                             max_compute_work_group_invocations(limits.maxComputeWorkGroupInvocations),
                                                                                             max_compute_work_group_size { limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], limits.maxComputeWorkGroupSize[2] },
                                                                                             subpixel_precision_bits(limits.subPixelPrecisionBits),
                                                                                             subtexel_precision_bits(limits.subTexelPrecisionBits),
                                                                                             mipmap_precision_bits(limits.mipmapPrecisionBits),
                                                                                             max_draw_indexed_index_value(limits.maxDrawIndexedIndexValue),
                                                                                             max_draw_indirect_count(limits.maxDrawIndirectCount),
                                                                                             max_sampler_lod_bias(limits.maxSamplerLodBias),
                                                                                             max_sampler_anisotropy(limits.maxSamplerAnisotropy),
                                                                                             max_viewports(limits.maxViewports),
                                                                                             max_viewport_dimensions { limits.maxViewportDimensions[0], limits.maxViewportDimensions[1] },
                                                                                             viewport_bounds_range { limits.viewportBoundsRange[0], limits.viewportBoundsRange[1] },
                                                                                             viewport_subpixel_bits(limits.viewportSubPixelBits),
                                                                                             min_memory_map_alignment(limits.minMemoryMapAlignment),
                                                                                             min_texel_buffer_offset_alignment(limits.minTexelBufferOffsetAlignment),
                                                                                             min_uniform_buffer_offset_alignment(limits.minUniformBufferOffsetAlignment),
                                                                                             min_storage_buffer_offset_alignment(limits.minStorageBufferOffsetAlignment),
                                                                                             min_texel_offset(limits.minTexelOffset),
                                                                                             max_texel_offset(limits.maxTexelOffset),
                                                                                             min_texel_gather_offset(limits.minTexelGatherOffset),
                                                                                             max_texel_gather_offset(limits.maxTexelGatherOffset),
                                                                                             min_interpolation_offset(limits.minInterpolationOffset),
                                                                                             max_interpolation_offset(limits.maxInterpolationOffset),
                                                                                             subpixel_interpolation_offset_bits(limits.subPixelInterpolationOffsetBits),
                                                                                             max_framebuffer_width(limits.maxFramebufferWidth),
                                                                                             max_framebuffer_height(limits.maxFramebufferHeight),
                                                                                             max_framebuffer_layers(limits.maxFramebufferLayers),
                                                                                             framebuffer_color_sample_counts(limits.framebufferColorSampleCounts),
                                                                                             framebuffer_depth_sample_counts(limits.framebufferDepthSampleCounts),
                                                                                             framebuffer_stencil_sample_counts(limits.framebufferStencilSampleCounts),
                                                                                             framebuffer_no_attachments_sample_counts(limits.framebufferNoAttachmentsSampleCounts),
                                                                                             max_color_attachments(limits.maxColorAttachments),
                                                                                             sampled_image_color_sample_counts(limits.sampledImageColorSampleCounts),
                                                                                             sampled_image_integer_sample_counts(limits.sampledImageIntegerSampleCounts),
                                                                                             sample_image_depth_sample_counts(limits.sampledImageDepthSampleCounts),
                                                                                             sample_image_stencil_sample_counts(limits.sampledImageStencilSampleCounts),
                                                                                             storage_image_sample_counts(limits.storageImageSampleCounts),
                                                                                             max_sample_mask_words(limits.maxSampleMaskWords),
                                                                                             timestamp_compute_and_graphics(limits.timestampComputeAndGraphics),
                                                                                             timestamp_period(limits.timestampPeriod),
                                                                                             max_clip_distances(limits.maxClipDistances),
                                                                                             max_cull_distances(limits.maxCullDistances),
                                                                                             max_combined_clip_and_cull_distances(limits.maxCombinedClipAndCullDistances),
                                                                                             discrete_queue_priorities(limits.discreteQueuePriorities),
                                                                                             point_size_range { limits.pointSizeRange[0], limits.pointSizeRange[1] },
                                                                                             line_width_range { limits.lineWidthRange[0], limits.lineWidthRange[1] },
                                                                                             point_size_granularity(limits.pointSizeGranularity),
                                                                                             line_width_granularity(limits.lineWidthGranularity),
                                                                                             strict_lines(limits.strictLines),
                                                                                             standard_sample_locations(limits.standardSampleLocations),
                                                                                             optimal_buffer_copy_offset_alignment(limits.optimalBufferCopyOffsetAlignment),
                                                                                             optimal_buffer_copy_row_pitch_alignment(limits.optimalBufferCopyRowPitchAlignment),
                                                                                             non_coherent_atom_size(limits.nonCoherentAtomSize)
                                                                                             {
    }
    
    VulkanPhysicalDevice::Properties::Limits::~Limits() = default;
    
    VulkanPhysicalDevice::Features::Features(const VkPhysicalDeviceFeatures& features) : robust_buffer_access(features.robustBufferAccess),
                                                                                         full_draw_index_uint32(features.fullDrawIndexUint32),
                                                                                         image_cube_array(features.imageCubeArray),
                                                                                         independent_blend(features.independentBlend),
                                                                                         geometry_shader(features.geometryShader),
                                                                                         {
    }
    
    VulkanPhysicalDevice::Features::~Features() = default;
    
}