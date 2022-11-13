
#include "vulkan/physical_device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

#include <cstring>

namespace vks {
    
    VulkanPhysicalDevice::Selector::Selector(const VulkanInstance& instance) : instance_(instance),
                                                                               fp_vk_enumerate_physical_devices_(reinterpret_cast<typeof(fp_vk_enumerate_physical_devices_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"))),
                                                                               fp_vk_get_physical_device_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties"))),
                                                                               fp_vk_get_physical_device_features_(reinterpret_cast<typeof(fp_vk_get_physical_device_features_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceFeatures"))),
                                                                               fp_vk_get_physical_device_queue_family_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_queue_family_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"))),
                                                                               fp_vk_enumerate_device_extension_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_extension_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumerateDeviceExtensionProperties"))),
                                                                               fp_vk_enumerate_device_layer_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_layer_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumerateDeviceLayerProperties"))),
                                                                               selector_(nullptr)
                                                                               {
        // TODO
        ASSERT(fp_vk_enumerate_physical_devices_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_properties_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_features_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_queue_family_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_extension_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_layer_properties_ != nullptr, "");
    }
    
    VulkanPhysicalDevice::Selector::~Selector() {
    }
    
    VulkanPhysicalDevice VulkanPhysicalDevice::Selector::select() {
        VkResult result;
        
        // Enumerate physical devices.
        std::uint32_t device_count = 0u;
        result = fp_vk_enumerate_physical_devices_(instance_, &device_count, nullptr);
        if (result != VK_SUCCESS) {
            // TODO:
        }
        
        if (device_count == 0u) {
            throw VulkanException("ERROR: Failed to find GPU with Vulkan support.");
        }
        
        std::vector<VkPhysicalDevice> devices(device_count);
        result = fp_vk_enumerate_physical_devices_(instance_, &device_count, devices.data());
        if (result != VK_SUCCESS) {
            // TODO:
        }
        
        // Configure default selector function.
        if (!selector_) {
            selector_ = [this](const Properties& device_properties, const Features& device_features, const QueueFamilies& device_queue_families) -> int {
                int score = 0;
                
                // Bias towards dedicated graphics hardware.
                switch (device_properties.type) {
                    case Properties::Type::DISCRETE_GPU:
                        score += 1000;
                        break;
                        
                    case Properties::Type::INTEGRATED_GPU:
                        score += 100;
                        break;
                        
                    case Properties::Type::VIRTUAL_GPU:
                    case Properties::Type::CPU:
                        score += 10;
                        break;
                        
                    default:
                    case Properties::Type::OTHER:
                        break;
                }
                
                // Bias towards devices that support the requested Vulkan API version.
                if (device_properties.runtime < instance_.runtime) {
                    score *= -1;
                }
                
                if (device_queue_families.supports_operation(QueueFamilies::Operation::TRANSFER) && device_queue_families.has_dedicated_queue(QueueFamilies::Operation::TRANSFER)) {
                
                }
                
                return score;
            };
        }
        
        VulkanPhysicalDevice device { };
        int highest_device_score = -1;
        
        for (std::uint32_t i = 0u; i < device_count; ++i) {
            VkPhysicalDevice device_handle = devices[i];
    
            // Query support for requested validation layers.
            std::uint32_t device_validation_layer_count = 0u;
            fp_vk_enumerate_device_layer_properties_(device_handle, &device_validation_layer_count, nullptr);
            
            std::vector<VkLayerProperties> device_validation_layers(device_validation_layer_count);
            fp_vk_enumerate_device_layer_properties_(device_handle, &device_validation_layer_count, device_validation_layers.data());
    
            std::vector<const char*> unsupported_layers;
            unsupported_layers.reserve(validation_layers_.size()); // Maximum number of unsupported layers.
    
            // Verify that all requested validation layers are available.
            for (const char* requested : validation_layers_) {
                bool found = false;
        
                for (std::uint32_t j = 0u; j < device_validation_layer_count; ++j) {
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
                // TODO: debug message (device was not selected due to unsupported layers....).
                continue;
            }
            
            std::uint32_t device_extension_count = 0u;
            fp_vk_enumerate_device_extension_properties_(device_handle, nullptr, &device_extension_count, nullptr);
            
            std::vector<VkExtensionProperties> device_extensions(device_extension_count);
            fp_vk_enumerate_device_extension_properties_(device_handle, nullptr, &device_extension_count, device_extensions.data());
            
            PFN_vkGetInstanceProcAddr
            
            for (const char* layer : validation_layers_) {
                std::uint32_t layer_extension_count = 0u;
                fp_vk_enumerate_device_extension_properties_(device_handle, layer, &layer_extension_count, nullptr);
                
                std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
                fp_vk_enumerate_device_extension_properties_(device_handle, layer, &layer_extension_count, layer_extensions.data());
                
                device_extension_count += layer_extension_count;
                device_extensions.insert(device_extensions.end(), layer_extensions.begin(), layer_extensions.end());
            }
            
            std::vector<const char*> unsupported_extensions;
            unsupported_extensions.reserve(extensions_.size());
            
            for (const char* requested : extensions_) {
                bool found = false;

                for (std::uint32_t j = 0u; j < device_extension_count; ++j) {
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
                // TODO: debug message (device was not selected due to unsupported extensions....).
                continue;
            }
    
            VkPhysicalDeviceProperties device_properties { };
            fp_vk_get_physical_device_properties_(device_handle, &device_properties);
            
            if (device_properties.apiVersion < instance_.runtime) {
                // TODO: debug message (device was not selected due to version mismatch....).
                continue;
            }
            
            VkPhysicalDeviceFeatures device_features { };
            fp_vk_get_physical_device_features_(device_handle, &device_features);
    
            std::uint32_t queue_family_count = 0u;
            fp_vk_get_physical_device_queue_family_properties_(device_handle, &queue_family_count, nullptr);
    
            std::vector<VkQueueFamilyProperties> device_queue_families(queue_family_count);
            fp_vk_get_physical_device_queue_family_properties_(device_handle, &queue_family_count, device_queue_families.data());
            
            int current_device_score = selector_(device_properties, device_features, device_queue_families);

            if (current_device_score > highest_device_score) {
                // Always pick the device with the highest score.
                device.handle_ = device_handle;
                device.properties = { device_properties };
                device.features = { device_features };
                device.queue_families = { device_queue_families };
                
                highest_device_score = current_device_score;
            }
        }
        
        if (highest_device_score <= 0) {
            throw VulkanException("ERROR: Could not select a suitable physical device given selection criteria.");
        }
        
        return device;
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_selector_function(const SelectorFn& selector) {
        selector_ = selector;
        return *this;
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_debug_name(const char* name) {
        if (strlen(name) > 0u) {
            name_ = name;
        }
        return *this;
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_enabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        // Ensure requested extension does not already exist.
        bool found = false;
        
        for (const char* extension : extensions_) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Only add the first instance of the extension.
            extensions_.emplace_back(requested);
        }
        
        return *this;
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_disabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        for (std::size_t i = 0u; i < extensions_.size(); ++i) {
            const char* extension = extensions_[i];
            
            if (strcmp(requested, extension) == 0) {
                // Found extension.
                std::swap(extensions_[i], extensions_[extensions_.size() - 1u]); // Swap idiom.
                extensions_.pop_back();
                
                // Extension is guaranteed to be present in the list only once.
                break;
            }
        }
        
        return *this;
    }
    
    VulkanPhysicalDevice::Properties::Properties(const VkPhysicalDeviceProperties& properties) : runtime({
                                                                                                     .major = VK_API_VERSION_MAJOR(properties.apiVersion),
                                                                                                     .minor = VK_API_VERSION_MINOR(properties.apiVersion),
                                                                                                     .patch = VK_API_VERSION_PATCH(properties.apiVersion)
                                                                                                 }),
                                                                                                 limits(properties.limits),
                                                                                                 driver_version(properties.driverVersion),
                                                                                                 device_name(std::string(properties.deviceName))
                                                                                                 {
        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                type = VulkanPhysicalDevice::Properties::Type::DISCRETE_GPU;
                break;
                
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                type = VulkanPhysicalDevice::Properties::Type::INTEGRATED_GPU;
                break;
                
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                type = VulkanPhysicalDevice::Properties::Type::VIRTUAL_GPU;
                break;
                
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                type = VulkanPhysicalDevice::Properties::Type::CPU;
                break;
                
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            default:
                type = VulkanPhysicalDevice::Properties::Type::OTHER;
                break;
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
                                                                                         tesselation_shader(features.tessellationShader),
                                                                                         sample_rate_shading(features.sampleRateShading),
                                                                                         dual_src_blend(features.dualSrcBlend),
                                                                                         logic_operations(features.logicOp),
                                                                                         multi_draw_indirect(features.multiDrawIndirect),
                                                                                         draw_indirect_first_instance(features.drawIndirectFirstInstance),
                                                                                         depth_clamp(features.depthClamp),
                                                                                         depth_bias_clamp(features.depthBiasClamp),
                                                                                         fill_mode_non_solid(features.fillModeNonSolid),
                                                                                         depth_bounds(features.depthBounds),
                                                                                         wide_lines(features.wideLines),
                                                                                         large_points(features.largePoints),
                                                                                         alpha_to_one(features.alphaToOne),
                                                                                         multi_viewport(features.multiViewport),
                                                                                         sampler_anisotropy(features.samplerAnisotropy),
                                                                                         texture_compression_etc2(features.textureCompressionETC2),
                                                                                         texture_compression_astc_ldr(features.textureCompressionASTC_LDR),
                                                                                         texture_compression_bc(features.textureCompressionBC),
                                                                                         occlusion_query_precise(features.occlusionQueryPrecise),
                                                                                         pipeline_statistics_query(features.pipelineStatisticsQuery),
                                                                                         vertex_pipeline_stores_and_atomics(features.vertexPipelineStoresAndAtomics),
                                                                                         fragment_stores_and_atomics(features.fragmentStoresAndAtomics),
                                                                                         shader_tesselation_and_geometry_point_size(features.shaderTessellationAndGeometryPointSize),
                                                                                         shader_image_gather_extended(features.shaderImageGatherExtended),
                                                                                         shader_storage_image_extended_formats(features.shaderStorageImageExtendedFormats),
                                                                                         shader_storage_multisample(features.shaderStorageImageMultisample),
                                                                                         shader_storage_image_read_without_format(features.shaderStorageImageReadWithoutFormat),
                                                                                         shader_storage_image_write_without_format(features.shaderStorageImageWriteWithoutFormat),
                                                                                         shader_uniform_buffer_array_dynamic_indexing(features.shaderUniformBufferArrayDynamicIndexing),
                                                                                         shader_sampled_image_array_dynamic_indexing(features.shaderSampledImageArrayDynamicIndexing),
                                                                                         shader_storage_buffer_array_dynamic_indexing(features.shaderStorageBufferArrayDynamicIndexing),
                                                                                         shader_storage_image_array_dynamic_indexing(features.shaderStorageImageArrayDynamicIndexing),
                                                                                         shader_clip_distance(features.shaderClipDistance),
                                                                                         shader_cull_distance(features.shaderCullDistance),
                                                                                         shader_float64(features.shaderFloat64),
                                                                                         shader_int64(features.shaderInt64),
                                                                                         shader_int16(features.shaderInt16),
                                                                                         shader_resource_residency(features.shaderResourceResidency),
                                                                                         shader_resource_min_lod(features.shaderResourceMinLod),
                                                                                         sparse_binding(features.sparseBinding),
                                                                                         sparse_residency_buffer(features.sparseResidencyBuffer),
                                                                                         sparse_residency_image2d(features.sparseResidencyImage2D),
                                                                                         sparse_residency_image3d(features.sparseResidencyImage3D),
                                                                                         sparse_residency_2_samples(features.sparseResidency2Samples),
                                                                                         sparse_residency_4_samples(features.sparseResidency4Samples),
                                                                                         sparse_residency_8_samples(features.sparseResidency8Samples),
                                                                                         sparse_residency_16_samples(features.sparseResidency16Samples),
                                                                                         sparse_residency_aliased(features.sparseResidencyAliased),
                                                                                         variable_multisample_rate(features.variableMultisampleRate),
                                                                                         inherited_queries(features.inheritedQueries)
                                                                                         {
    }
    
    VulkanPhysicalDevice::Features::~Features() = default;

}