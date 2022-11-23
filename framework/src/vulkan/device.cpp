
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

#include <cstring>
#include <iostream>

namespace vks {
    
    VulkanDevice::Properties::operator VkPhysicalDeviceProperties() const {
        VkPhysicalDeviceType device_type;
        
        switch (type) {
            case Type::DISCRETE_GPU:
                device_type = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                break;
            
            case Type::INTEGRATED_GPU:
                device_type = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
                break;
            
            case Type::VIRTUAL_GPU:
                device_type = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
                break;
            
            case Type::CPU:
                device_type = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU;
                break;
            
            default:
            case Type::OTHER:
                device_type = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_OTHER;
                break;
        }
        
        VkPhysicalDeviceProperties device_properties {
            .apiVersion = runtime,
            .driverVersion = driver_version,
            .vendorID = { },
            .deviceID = { },
            .deviceType = device_type,
            .deviceName = { },
            .pipelineCacheUUID = { },
            .limits = limits,
            .sparseProperties = { }
        };
        
        // Copy device name into 'device_properties' buffer.
        strcpy(&device_properties.deviceName[0], device_name.c_str());
        
        return device_properties;
    }
    
    VulkanDevice::Properties::Limits::operator VkPhysicalDeviceLimits() const {
        return VkPhysicalDeviceLimits {
            .maxImageDimension1D = max_image_dimension_1d,
            .maxImageDimension2D = max_image_dimension_2d,
            .maxImageDimension3D = max_image_dimension_3d,
            .maxImageDimensionCube = max_image_dimension_cube,
            .maxImageArrayLayers = max_image_array_layers,
            .maxTexelBufferElements = max_texel_buffer_elements,
            .maxUniformBufferRange = max_uniform_buffer_range,
            .maxStorageBufferRange = max_storage_buffer_range,
            .maxPushConstantsSize = max_push_constants_size,
            .maxMemoryAllocationCount = max_memory_allocation_count,
            .maxSamplerAllocationCount = max_sampler_allocation_count,
            .bufferImageGranularity = buffer_image_granularity,
            .sparseAddressSpaceSize = sparse_address_space_size,
            .maxBoundDescriptorSets = max_bound_descriptor_sets,
            .maxPerStageDescriptorSamplers = max_per_stage_descriptor_samplers,
            .maxPerStageDescriptorUniformBuffers = max_per_stage_descriptor_uniform_buffers,
            .maxPerStageDescriptorStorageBuffers = max_per_stage_descriptor_storage_buffers,
            .maxPerStageDescriptorSampledImages = max_per_stage_descriptor_sampled_images,
            .maxPerStageDescriptorStorageImages = max_per_stage_descriptor_storage_images,
            .maxPerStageDescriptorInputAttachments = max_per_stage_descriptor_input_attachments,
            .maxPerStageResources = max_per_stage_resources,
            .maxDescriptorSetSamplers = max_descriptor_set_samplers,
            .maxDescriptorSetUniformBuffers = max_descriptor_set_uniform_buffers,
            .maxDescriptorSetUniformBuffersDynamic = max_descriptor_set_uniform_buffers_dynamic,
            .maxDescriptorSetStorageBuffers = max_descriptor_set_storage_buffers,
            .maxDescriptorSetStorageBuffersDynamic = max_descriptor_set_storage_buffers_dynamic,
            .maxDescriptorSetSampledImages = max_descriptor_set_sampled_images,
            .maxDescriptorSetStorageImages = max_descriptor_set_storage_images,
            .maxDescriptorSetInputAttachments = max_descriptor_set_input_attachments,
            .maxVertexInputAttributes = max_vertex_input_attributes,
            .maxVertexInputBindings = max_vertex_input_bindings,
            .maxVertexInputAttributeOffset = max_vertex_input_attribute_offset,
            .maxVertexInputBindingStride = max_vertex_input_binding_stride,
            .maxVertexOutputComponents = max_vertex_output_components,
            .maxTessellationGenerationLevel = max_tesselation_generation_level,
            .maxTessellationPatchSize = max_tesselation_patch_size,
            .maxTessellationControlPerVertexInputComponents = max_tesselation_control_per_vertex_input_components,
            .maxTessellationControlPerVertexOutputComponents = max_tesselation_control_per_vertex_output_components,
            .maxTessellationControlPerPatchOutputComponents = max_tesselation_control_per_patch_output_components,
            .maxTessellationControlTotalOutputComponents = max_tesselation_control_total_output_components,
            .maxTessellationEvaluationInputComponents = max_tesselation_evaluation_input_components,
            .maxTessellationEvaluationOutputComponents = max_tesselation_evaluation_output_components,
            .maxGeometryShaderInvocations = max_geometry_shader_invocations,
            .maxGeometryInputComponents = max_geometry_input_components,
            .maxGeometryOutputComponents = max_geometry_output_components,
            .maxGeometryOutputVertices = max_geometry_output_vertices,
            .maxGeometryTotalOutputComponents = max_geometry_total_output_components,
            .maxFragmentInputComponents = max_fragment_input_components,
            .maxFragmentOutputAttachments = max_fragment_output_attachments,
            .maxFragmentDualSrcAttachments = max_fragment_dual_src_attachments,
            .maxFragmentCombinedOutputResources = max_fragment_combined_output_resources,
            .maxComputeSharedMemorySize = max_compute_shared_memory_size,
            .maxComputeWorkGroupCount = { max_compute_work_group_count[0], max_compute_work_group_count[1], max_compute_work_group_count[2] },
            .maxComputeWorkGroupInvocations = max_compute_work_group_invocations,
            .maxComputeWorkGroupSize = { max_compute_work_group_size[0], max_compute_work_group_size[1], max_compute_work_group_size[2] },
            .subPixelPrecisionBits = subpixel_precision_bits,
            .subTexelPrecisionBits = subtexel_precision_bits,
            .mipmapPrecisionBits = mipmap_precision_bits,
            .maxDrawIndexedIndexValue = max_draw_indexed_index_value,
            .maxDrawIndirectCount = max_draw_indirect_count,
            .maxSamplerLodBias = max_sampler_lod_bias,
            .maxSamplerAnisotropy = max_sampler_anisotropy,
            .maxViewports = max_viewports,
            .maxViewportDimensions = { max_viewport_dimensions[0], max_viewport_dimensions[1] },
            .viewportBoundsRange = { viewport_bounds_range[0], viewport_bounds_range[1] },
            .viewportSubPixelBits = viewport_subpixel_bits,
            .minMemoryMapAlignment = min_memory_map_alignment,
            .minTexelBufferOffsetAlignment = min_texel_buffer_offset_alignment,
            .minUniformBufferOffsetAlignment = min_uniform_buffer_offset_alignment,
            .minStorageBufferOffsetAlignment = min_storage_buffer_offset_alignment,
            .minTexelOffset = min_texel_offset,
            .maxTexelOffset = max_texel_offset,
            .minTexelGatherOffset = min_texel_gather_offset,
            .maxTexelGatherOffset = max_texel_gather_offset,
            .minInterpolationOffset = min_interpolation_offset,
            .maxInterpolationOffset = max_interpolation_offset,
            .subPixelInterpolationOffsetBits = subpixel_interpolation_offset_bits,
            .maxFramebufferWidth = max_framebuffer_width,
            .maxFramebufferHeight = max_framebuffer_height,
            .maxFramebufferLayers = max_framebuffer_layers,
            .framebufferColorSampleCounts = framebuffer_color_sample_counts,
            .framebufferDepthSampleCounts = framebuffer_depth_sample_counts,
            .framebufferStencilSampleCounts = framebuffer_stencil_sample_counts,
            .framebufferNoAttachmentsSampleCounts = framebuffer_no_attachments_sample_counts,
            .maxColorAttachments = max_color_attachments,
            .sampledImageColorSampleCounts = sampled_image_color_sample_counts,
            .sampledImageIntegerSampleCounts = sampled_image_integer_sample_counts,
            .sampledImageDepthSampleCounts = sample_image_depth_sample_counts,
            .sampledImageStencilSampleCounts = sample_image_stencil_sample_counts,
            .storageImageSampleCounts = storage_image_sample_counts,
            .maxSampleMaskWords = max_sample_mask_words,
            .timestampComputeAndGraphics = timestamp_compute_and_graphics,
            .timestampPeriod = timestamp_period,
            .maxClipDistances = max_clip_distances,
            .maxCullDistances = max_cull_distances,
            .maxCombinedClipAndCullDistances = max_combined_clip_and_cull_distances,
            .discreteQueuePriorities = discrete_queue_priorities,
            .pointSizeRange = { point_size_range[0], point_size_range[1] },
            .lineWidthRange = { line_width_range[0], line_width_range[1] },
            .pointSizeGranularity = point_size_granularity,
            .lineWidthGranularity = line_width_granularity,
            .strictLines = strict_lines,
            .standardSampleLocations = standard_sample_locations,
            .optimalBufferCopyOffsetAlignment = optimal_buffer_copy_offset_alignment,
            .optimalBufferCopyRowPitchAlignment = optimal_buffer_copy_row_pitch_alignment,
            .nonCoherentAtomSize = non_coherent_atom_size
        };
    }
    
    VulkanDevice::Features::operator VkPhysicalDeviceFeatures() const {
        return VkPhysicalDeviceFeatures {
            .robustBufferAccess = robust_buffer_access,
            .fullDrawIndexUint32 = full_draw_index_uint32,
            .imageCubeArray = image_cube_array,
            .independentBlend = independent_blend,
            .geometryShader = geometry_shader,
            .tessellationShader = tesselation_shader,
            .sampleRateShading = sample_rate_shading,
            .dualSrcBlend = dual_src_blend,
            .logicOp = logic_operations,
            .multiDrawIndirect = multi_draw_indirect,
            .drawIndirectFirstInstance = draw_indirect_first_instance,
            .depthClamp = depth_clamp,
            .depthBiasClamp = depth_bias_clamp,
            .fillModeNonSolid = fill_mode_non_solid,
            .depthBounds = depth_bounds,
            .wideLines = wide_lines,
            .largePoints = large_points,
            .alphaToOne = alpha_to_one,
            .multiViewport = multi_viewport,
            .samplerAnisotropy = sampler_anisotropy,
            .textureCompressionETC2 = texture_compression_etc2,
            .textureCompressionASTC_LDR = texture_compression_astc_ldr,
            .textureCompressionBC = texture_compression_bc,
            .occlusionQueryPrecise = occlusion_query_precise,
            .pipelineStatisticsQuery = pipeline_statistics_query,
            .vertexPipelineStoresAndAtomics = vertex_pipeline_stores_and_atomics,
            .fragmentStoresAndAtomics = fragment_stores_and_atomics,
            .shaderTessellationAndGeometryPointSize = shader_tesselation_and_geometry_point_size,
            .shaderImageGatherExtended = shader_image_gather_extended,
            .shaderStorageImageExtendedFormats = shader_storage_image_extended_formats,
            .shaderStorageImageMultisample = shader_storage_multisample,
            .shaderStorageImageReadWithoutFormat = shader_storage_image_read_without_format,
            .shaderStorageImageWriteWithoutFormat = shader_storage_image_write_without_format,
            .shaderUniformBufferArrayDynamicIndexing = shader_uniform_buffer_array_dynamic_indexing,
            .shaderSampledImageArrayDynamicIndexing = shader_sampled_image_array_dynamic_indexing,
            .shaderStorageBufferArrayDynamicIndexing = shader_storage_buffer_array_dynamic_indexing,
            .shaderStorageImageArrayDynamicIndexing = shader_storage_image_array_dynamic_indexing,
            .shaderClipDistance = shader_clip_distance,
            .shaderCullDistance = shader_cull_distance,
            .shaderFloat64 = shader_float64,
            .shaderInt64 = shader_int64,
            .shaderInt16 = shader_int16,
            .shaderResourceResidency = shader_resource_residency,
            .shaderResourceMinLod = shader_resource_min_lod,
            .sparseBinding = sparse_binding,
            .sparseResidencyBuffer = sparse_residency_buffer,
            .sparseResidencyImage2D = sparse_residency_image2d,
            .sparseResidencyImage3D = sparse_residency_image3d,
            .sparseResidency2Samples = sparse_residency_2_samples,
            .sparseResidency4Samples = sparse_residency_4_samples,
            .sparseResidency8Samples = sparse_residency_8_samples,
            .sparseResidency16Samples = sparse_residency_16_samples,
            .sparseResidencyAliased = sparse_residency_aliased,
            .variableMultisampleRate = variable_multisample_rate,
            .inheritedQueries = inherited_queries
        };
    }
    
    VulkanDevice::Properties convert(const VkPhysicalDeviceProperties& device_properties) {
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
        
        return VulkanDevice::Properties {
            .type = device_type,
            .limits = {
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
                .buffer_image_granularity = static_cast<uint32_t>(device_properties.limits.bufferImageGranularity),
                .sparse_address_space_size = static_cast<uint32_t>(device_properties.limits.sparseAddressSpaceSize),
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
    }
    
    VulkanDevice::Features convert(const VkPhysicalDeviceFeatures& device_features) {
        // Safe to cast to bool: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBool32.html
        return VulkanDevice::Features {
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
    }
    
//    VulkanDevice::QueueFamilies convert(const std::vector<VkQueueFamilyProperties>& device_queue_families) {
//
//    }
    
    VulkanDevice::Builder::Builder(const VulkanInstance& instance) : fp_vk_enumerate_physical_devices_(reinterpret_cast<typeof(fp_vk_enumerate_physical_devices_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"))),
                                                                     fp_vk_get_physical_device_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties"))),
                                                                     fp_vk_get_physical_device_features_(reinterpret_cast<typeof(fp_vk_get_physical_device_features_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceFeatures"))),
                                                                     fp_vk_get_physical_device_queue_family_properties_(reinterpret_cast<typeof(fp_vk_get_physical_device_queue_family_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"))),
                                                                     fp_vk_enumerate_device_extension_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_extension_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumerateDeviceExtensionProperties"))),
                                                                     fp_vk_enumerate_device_layer_properties_(reinterpret_cast<typeof(fp_vk_enumerate_device_layer_properties_)>(detail::vk_get_instance_proc_addr(instance, "vkEnumerateDeviceLayerProperties"))),
                                                                     fp_vk_create_device_(),
                                                                     m_fp_vk_get_physical_device_win32_presentation_support(reinterpret_cast<typeof(m_fp_vk_get_physical_device_win32_presentation_support)>(detail::vk_get_instance_proc_addr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"))),
                                                                     instance_(instance),
                                                                     requested_features_()
                                                                     {
//                                                                     selector_([this](const Properties& device_properties, const Features& device_features, const QueueFamilies& device_queue_families) -> int {
//                                                                         int score = 0;
//
//                                                                         // Bias towards dedicated graphics hardware.
//                                                                         switch (device_properties.type) {
//                                                                             case Properties::Type::DISCRETE_GPU:
//                                                                                 score += 1000;
//                                                                                 break;
//
//                                                                             case Properties::Type::INTEGRATED_GPU:
//                                                                                 score += 100;
//                                                                                 break;
//
//                                                                             case Properties::Type::VIRTUAL_GPU:
//                                                                             case Properties::Type::CPU:
//                                                                                 score += 10;
//                                                                                 break;
//
//                                                                             default:
//                                                                             case Properties::Type::OTHER:
//                                                                                 break;
//                                                                         }
//
//                                                                         // Bias towards a device that has a dedicated transfer queue.
//                                                                         if (device_queue_families.supports_operation(Queue::Operation::TRANSFER) && device_queue_families.has_dedicated_queue(Queue::Operation::TRANSFER)) {
//                                                                             score += 100;
//                                                                         }
//
//                                                                         return score;
//                                                                     }) {
        // TODO:
        ASSERT(fp_vk_enumerate_physical_devices_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_properties_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_features_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_queue_family_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_extension_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_layer_properties_ != nullptr, "");
        // ASSERT(fp_vk_create_device != nullptr, "");
    }
    
    VulkanDevice VulkanDevice::Builder::build() const {
        VkResult result;
    
        // Select physical device. --------------------------------------------------------------------------------------------------------
        // Get available physical devices.
        std::uint32_t device_count = 0u;
        result = fp_vk_enumerate_physical_devices_(instance_, &device_count, nullptr);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to get physical device count.");
        }
    
        if (device_count == 0u) {
            throw std::runtime_error("Failed to find GPU with Vulkan support.");
        }
    
        std::vector<VkPhysicalDevice> devices(device_count);
        result = fp_vk_enumerate_physical_devices_(instance_, &device_count, devices.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to enumerate physical devices.");
        }
    
        VulkanDevice device { };
        int highest_device_score = -1;
    
        for (std::uint32_t i = 0u; i < device_count; ++i) {
            DeviceContext context {
                .device = devices[i]
            };
            
            // Verify that given device supports all requested device validation layers.
            if (!verify_device_validation_layer_support(context.device)) {
                continue;
            }
        
            // Verify that given device supports all requested device extensions.
            if (!verify_device_extension_support(context.device)) {
                continue;
            }
        
            // Verify that given device properties meet requested requirements.
            fp_vk_get_physical_device_properties_(context.device, &context.properties);
//            if (!verify_device_properties(context.properties)) {
//                continue;
//            }
            
            // Verify that requested device features are supported by the given device.
            VkPhysicalDeviceFeatures device_features { };
            fp_vk_get_physical_device_features_(context.device, &context.features);
            if (!verify_device_feature_support(context.features)) {
                continue;
            }
        
            // Verify that requested device queue family properties are supported by the given device.
            std::uint32_t queue_family_count = 0u;
            fp_vk_get_physical_device_queue_family_properties_(context.device, &queue_family_count, nullptr);
    
            context.queue_families.resize(queue_family_count);
            fp_vk_get_physical_device_queue_family_properties_(context.device, &queue_family_count, context.queue_families.data());
        
            if (!verify_device_queue_family_support(context.device, context.queue_families)) {
                continue;
            }
            
//            int current_device_score = selector_(device_properties, device_features, device_queue_families);
//
//            // Always pick the device with the highest score.
//            if (current_device_score > highest_device_score) {
//                device = current;
//                highest_device_score = current_device_score;
//            }
        }
    
        if (highest_device_score <= 0) {
            throw std::runtime_error("Failed to find suitable physical device given selection criteria.");
        }
        
        
        // Construct logical device. ----------------------------------------------------------------------------------
        
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_physical_device_selector_function(int(*selector)(const Properties& p, const Features& f)) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_features(const Features& requested) {
        requested_features_ = requested;
        return *this;
    }
    
//    VulkanDevice::Builder& VulkanDevice::Builder::with_dedicated_queue(Queue::Operation requested) {
//        bool found = false;
//
//        for (Queue::Operation available : dedicated_queues_) {
//            if (available == requested) {
//                found = true;
//                break;
//            }
//        }
//
//        // Only add a dedicated queue request if it doesn't already exist.
//        if (!found) {
//            dedicated_queues_.emplace_back(requested);
//        }
//
//        return *this;
//    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_extension(const char* requested) {
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
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_extension(const char* requested) {
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
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_validation_layer(const char* layer) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_validation_layer(const char* layer) {
    
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_debug_name(const char* name) {
        if (strlen(name) > 0u) {
            name_ = name;
        }
        
        return *this;
    }
    
    bool VulkanDevice::Builder::verify_device_extension_support(VkPhysicalDevice device_handle) const {
        VkResult result;
        
        std::uint32_t device_extension_count = 0u;
        result = fp_vk_enumerate_device_extension_properties_(device_handle, nullptr, &device_extension_count, nullptr);
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
        
        std::vector<VkExtensionProperties> device_extensions(device_extension_count);
        result = fp_vk_enumerate_device_extension_properties_(device_handle, nullptr, &device_extension_count, device_extensions.data());
        
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
            // TODO: log message
            return false;
        }
        
        return true;
    }
    
    bool VulkanDevice::Builder::verify_device_validation_layer_support(VkPhysicalDevice device_handle) const {
        VkResult result;
        
        std::uint32_t device_validation_layer_count = 0u;
        result = fp_vk_enumerate_device_layer_properties_(device_handle, &device_validation_layer_count, nullptr);
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
    
        std::vector<VkLayerProperties> device_validation_layers(device_validation_layer_count);
        result = fp_vk_enumerate_device_layer_properties_(device_handle, &device_validation_layer_count, device_validation_layers.data());
        if (result != VK_SUCCESS) {
            // TODO: log message
            return false;
        }
    
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
            // TODO: log message
            return false;
        }
        
        return true;
    }
    
    bool VulkanDevice::Builder::verify_device_properties(const VkPhysicalDeviceProperties& device_properties) const {
        return device_properties.apiVersion >= instance_.runtime;
    }
    
    bool VulkanDevice::Builder::verify_device_feature_support(const VkPhysicalDeviceFeatures& device_features) const {
        if (requested_features_.robust_buffer_access && !static_cast<bool>(device_features.robustBufferAccess)) {
            return false;
        }
        
        if (requested_features_.full_draw_index_uint32 && !static_cast<bool>(device_features.fullDrawIndexUint32)) {
            return false;
        }
        
        if (requested_features_.image_cube_array && !static_cast<bool>(device_features.imageCubeArray)) {
            return false;
        }
        
        if (requested_features_.independent_blend && !static_cast<bool>(device_features.independentBlend)) {
            return false;
        }
        
        if (requested_features_.geometry_shader && !static_cast<bool>(device_features.geometryShader)) {
            return false;
        }
        
        if (requested_features_.tesselation_shader && !static_cast<bool>(device_features.tessellationShader)) {
            return false;
        }
        
        if (requested_features_.sample_rate_shading && !static_cast<bool>(device_features.sampleRateShading)) {
            return false;
        }
        
        if (requested_features_.dual_src_blend && !static_cast<bool>(device_features.dualSrcBlend)) {
            return false;
        }
        
        if (requested_features_.logic_operations && !static_cast<bool>(device_features.logicOp)) {
            return false;
        }
        
        if (requested_features_.multi_draw_indirect && !static_cast<bool>(device_features.multiDrawIndirect)) {
            return false;
        }
        
        if (requested_features_.draw_indirect_first_instance && !static_cast<bool>(device_features.drawIndirectFirstInstance)) {
            return false;
        }
        
        if (requested_features_.depth_clamp && !static_cast<bool>(device_features.depthClamp)) {
            return false;
        }
        
        if (requested_features_.depth_bias_clamp && !static_cast<bool>(device_features.depthBiasClamp)) {
            return false;
        }
        
        if (requested_features_.fill_mode_non_solid && !static_cast<bool>(device_features.fillModeNonSolid)) {
            return false;
        }
        
        if (requested_features_.depth_bounds && !static_cast<bool>(device_features.depthBounds)) {
            return false;
        }
        
        if (requested_features_.wide_lines && !static_cast<bool>(device_features.wideLines)) {
            return false;
        }
        
        if (requested_features_.large_points && !static_cast<bool>(device_features.largePoints)) {
            return false;
        }
        
        if (requested_features_.alpha_to_one && !static_cast<bool>(device_features.alphaToOne)) {
            return false;
        }
        
        if (requested_features_.multi_viewport && !static_cast<bool>(device_features.multiViewport)) {
            return false;
        }
        
        if (requested_features_.sampler_anisotropy && !static_cast<bool>(device_features.samplerAnisotropy)) {
            return false;
        }
        
        if (requested_features_.texture_compression_etc2 && !static_cast<bool>(device_features.textureCompressionETC2)) {
            return false;
        }
        
        if (requested_features_.texture_compression_astc_ldr && !static_cast<bool>(device_features.textureCompressionASTC_LDR)) {
            return false;
        }
        
        if (requested_features_.texture_compression_bc && !static_cast<bool>(device_features.textureCompressionBC)) {
            return false;
        }
        
        if (requested_features_.occlusion_query_precise && !static_cast<bool>(device_features.occlusionQueryPrecise)) {
            return false;
        }
        
        if (requested_features_.pipeline_statistics_query && !static_cast<bool>(device_features.pipelineStatisticsQuery)) {
            return false;
        }
        
        if (requested_features_.vertex_pipeline_stores_and_atomics && !static_cast<bool>(device_features.vertexPipelineStoresAndAtomics)) {
            return false;
        }
        
        if (requested_features_.fragment_stores_and_atomics && !static_cast<bool>(device_features.fragmentStoresAndAtomics)) {
            return false;
        }
        
        if (requested_features_.shader_tesselation_and_geometry_point_size && !static_cast<bool>(device_features.shaderTessellationAndGeometryPointSize)) {
            return false;
        }
        
        if (requested_features_.shader_image_gather_extended && !static_cast<bool>(device_features.shaderImageGatherExtended)) {
            return false;
        }
        
        if (requested_features_.shader_storage_image_extended_formats && !static_cast<bool>(device_features.shaderStorageImageExtendedFormats)) {
            return false;
        }
        
        if (requested_features_.shader_storage_multisample && !static_cast<bool>(device_features.shaderStorageImageMultisample)) {
            return false;
        }
        
        if (requested_features_.shader_storage_image_read_without_format && !static_cast<bool>(device_features.shaderStorageImageReadWithoutFormat)) {
            return false;
        }
        
        if (requested_features_.shader_storage_image_write_without_format && !static_cast<bool>(device_features.shaderStorageImageWriteWithoutFormat)) {
            return false;
        }
        
        if (requested_features_.shader_uniform_buffer_array_dynamic_indexing && !static_cast<bool>(device_features.shaderUniformBufferArrayDynamicIndexing)) {
            return false;
        }
        
        if (requested_features_.shader_sampled_image_array_dynamic_indexing && !static_cast<bool>(device_features.shaderSampledImageArrayDynamicIndexing)) {
            return false;
        }
        
        if (requested_features_.shader_storage_buffer_array_dynamic_indexing && !static_cast<bool>(device_features.shaderStorageBufferArrayDynamicIndexing)) {
            return false;
        }
        
        if (requested_features_.shader_storage_image_array_dynamic_indexing && !static_cast<bool>(device_features.shaderStorageImageArrayDynamicIndexing)) {
            return false;
        }
        
        if (requested_features_.shader_clip_distance && !static_cast<bool>(device_features.shaderClipDistance)) {
            return false;
        }
        
        if (requested_features_.shader_cull_distance && !static_cast<bool>(device_features.shaderCullDistance)) {
            return false;
        }
        
        if (requested_features_.shader_float64 && !static_cast<bool>(device_features.shaderFloat64)) {
            return false;
        }
        
        if (requested_features_.shader_int64 && !static_cast<bool>(device_features.shaderInt64)) {
            return false;
        }
        
        if (requested_features_.shader_int16 && !static_cast<bool>(device_features.shaderInt16)) {
            return false;
        }
        
        if (requested_features_.shader_resource_residency && !static_cast<bool>(device_features.shaderResourceResidency)) {
            return false;
        }
        
        if (requested_features_.shader_resource_min_lod && !static_cast<bool>(device_features.shaderResourceMinLod)) {
            return false;
        }
        
        if (requested_features_.sparse_binding && !static_cast<bool>(device_features.sparseBinding)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_buffer && !static_cast<bool>(device_features.sparseResidencyBuffer)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_image2d && !static_cast<bool>(device_features.sparseResidencyImage2D)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_image3d && !static_cast<bool>(device_features.sparseResidencyImage3D)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_2_samples && !static_cast<bool>(device_features.sparseResidency2Samples)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_4_samples && !static_cast<bool>(device_features.sparseResidency4Samples)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_8_samples && !static_cast<bool>(device_features.sparseResidency8Samples)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_16_samples && !static_cast<bool>(device_features.sparseResidency16Samples)) {
            return false;
        }
        
        if (requested_features_.sparse_residency_aliased && !static_cast<bool>(device_features.sparseResidencyAliased)) {
            return false;
        }
        
        if (requested_features_.variable_multisample_rate && !static_cast<bool>(device_features.variableMultisampleRate)) {
            return false;
        }
        
        if (requested_features_.inherited_queries && !static_cast<bool>(device_features.inheritedQueries)) {
            return false;
        }
        
        return true;
    }
    
    bool VulkanDevice::Builder::verify_device_queue_family_support(VkPhysicalDevice device_handle, const std::vector<VkQueueFamilyProperties>& device_queue_families) const {
        std::uint32_t device_queue_family_count = static_cast<std::uint32_t>(device_queue_families.size());
        std::uint32_t highest_score = 0u;
        
        for (std::uint32_t i = 0u; i < device_queue_family_count; ++i) {
            const VkQueueFamilyProperties& family = device_queue_families[i];
            bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
            bool has_presentation_support = static_cast<bool>(m_fp_vk_get_physical_device_win32_presentation_support(device_handle, i));
            
            std::cout << "Queue family: " << i << ", queue count: " << family.queueCount << std::endl;
            std::cout << "graphics support? " << (has_graphics_support ? "yes " : "no") << std::endl;
            std::cout << "presentation support? " << (has_presentation_support ? "yes" : "no") << std::endl;
            std::cout << "compute support? " << (has_compute_support ? "yes" : "no") << std::endl;
            std::cout << "transfer support? " << (has_transfer_support ? "yes" : "no") << std::endl;
            std::cout << std::endl;
        }
        
        GraphicsQueueFamily graphics_queue_family {
            .index = device_queue_family_count
        };
    
        highest_score = 0u;
        
        // Select a queue family for graphics-related operations.
        for (std::uint32_t i = 0u; i < device_queue_family_count; ++i) {
            const VkQueueFamilyProperties& family = device_queue_families[i];
    
            bool has_graphics_support = (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
            bool has_presentation_support = static_cast<bool>(m_fp_vk_get_physical_device_win32_presentation_support(device_handle, i));
    
            // The ideal graphics queue family supports graphics, presentation, and synchronous compute / transfer operations.
            // Scoring of graphics queue family (based on supported operations):
            // 6. GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
            // 5. GRAPHICS | PRESENTATION | TRANSFER
            // 4. GRAPHICS | PRESENTATION
            // 3. GRAPHICS | COMPUTE | TRANSFER
            // 2. GRAPHICS | TRANSFER
            // 1. GRAPHICS
            std::uint32_t current_score = 0u;
    
            // Note: TRANSFER operations are implicitly allowed on a queue that supports both GRAPHICS and COMPUTE operations.
            if (has_graphics_support) {
                if (has_presentation_support) {
                    if (has_compute_support) {
                        // GRAPHICS | PRESENTATION | COMPUTE | TRANSFER
                        current_score = 6u;
                    }
                    else if (has_transfer_support) {
                        // GRAPHICS | PRESENTATION | TRANSFER
                        current_score = 5u;
                    }
                    else {
                        // GRAPHICS | PRESENTATION
                        current_score = 4u;
                    }
                }
                else if (has_compute_support) {
                    // GRAPHICS | COMPUTE | TRANSFER
                    current_score = 3u;
                }
                else if (has_transfer_support) {
                    // GRAPHICS | TRANSFER
                    current_score = 2u;
                }
                else {
                    // GRAPHICS
                    current_score = 1u;
                }
            }
            
            if (current_score > highest_score) {
                highest_score = current_score;
                graphics_queue_family.index = i;
                graphics_queue_family.num_available_queues = family.queueCount;
            }
        }
        
        if (graphics_queue_family.index == device_queue_family_count) {
            // Found no queue families that support graphics operations.
            return false;
        }
        
        switch (highest_score) {
            case 1u:
                break;
                
            case 2u:
                graphics_queue_family.has_transfer_support = true;
                break;
                
            case 3u:
                graphics_queue_family.has_compute_support = true;
                graphics_queue_family.has_transfer_support = true;
                break;
                
            case 4u:
                graphics_queue_family.has_presentation_support = true;
                break;
                
            case 5u:
                graphics_queue_family.has_presentation_support = true;
                graphics_queue_family.has_transfer_support = true;
                break;
                
            case 6u:
                graphics_queue_family.has_presentation_support = true;
                graphics_queue_family.has_compute_support = true;
                graphics_queue_family.has_transfer_support = true;
                break;
        }
        
        ComputeQueueFamily compute_queue_family {
            .index = device_queue_family_count,
            .async = true
        };
        
        TransferQueueFamily transfer_queue_family {
            .index = device_queue_family_count,
            .async = true
        };

        std::uint32_t highest_compute_score = 0u;
        std::uint32_t highest_transfer_score = 0u;
        
        // Select queue families for compute and transfer operations.
        for (std::uint32_t i = 0u; i < device_queue_family_count; ++i) {
            const VkQueueFamilyProperties& family = device_queue_families[i];
            bool has_compute_support = (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            bool has_transfer_support = (family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
            bool has_presentation_support = static_cast<bool>(m_fp_vk_get_physical_device_win32_presentation_support(device_handle, i));
            
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
            std::uint32_t current_compute_score = 0u;
    
            // Scoring for asynchronous transfer queue family (based on supported operations):
            // 4. TRANSFER
            // 3. COMPUTE | TRANSFER
            // 2. PRESENTATION | TRANSFER
            // 1. PRESENTATION | COMPUTE | TRANSFER
            std::uint32_t current_transfer_score = 0u;
    
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
                compute_queue_family.index = i;
            }
            
            if (current_transfer_score > highest_transfer_score) {
                highest_transfer_score = current_transfer_score;
                transfer_queue_family.index = i;
            }
        }
        
        if (compute_queue_family.index == device_queue_family_count) {
            // Found no queue families that support asynchronous compute operations - default to graphics queue family and synchronized compute operations, if available.
            if (graphics_queue_family.has_compute_support) {
                compute_queue_family.index = graphics_queue_family.index;
                compute_queue_family.async = false;
            }
            else {
                return false;
            }
        }
        
        if (transfer_queue_family.index == device_queue_family_count) {
            // Found no queue families that support asynchronous transfer operations - default to graphics queue family and synchronized transfer operations, if available.
            if (graphics_queue_family.has_transfer_support) {
                transfer_queue_family.index = graphics_queue_family.index;
                transfer_queue_family.async = false;
            }
            else {
                return false;
            }
        }
    
        PresentationQueueFamily presentation_queue_family {
            .index = device_queue_family_count
        };
    
        if (graphics_queue_family.has_presentation_support) {
            // Prefer a presentation queue family that also supports graphics operations for performance reasons.
            presentation_queue_family.index = graphics_queue_family.index;
        }
        else {
            highest_score = 0u;
            
            // Select a queue family for presentation operations.
            for (std::uint32_t i = 0u; i < device_queue_family_count; ++i) {
                const VkQueueFamilyProperties& family = device_queue_families[i];
                bool has_presentation_support = static_cast<bool>(m_fp_vk_get_physical_device_win32_presentation_support(device_handle, i));
                bool shares_compute_support = ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) && (compute_queue_family.index == i);
                bool shares_transfer_support = ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) && (transfer_queue_family.index == i);
                
                // The ideal presentation queue family, if not supported by the graphics queue family, is standalone and independent of the selected compute / transfer queue families.
                // Otherwise, prefer sharing queue bandwidth with other asynchronous operations on the selected compute / transfer queue families.
                // Scoring of presentation queue family (based on supported operations):
                // 4. PRESENTATION
                // 3. PRESENTATION | TRANSFER
                // 2. PRESENTATION | COMPUTE
                // 1. PRESENTATION | COMPUTE | TRANSFER
                
                std::uint32_t current_score = 0u;
                
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
                    presentation_queue_family.index = i;
                    presentation_queue_family.num_available_queues = family.queueCount;
                }
            }
        }
    
        if (presentation_queue_family.index == device_queue_family_count) {
            // Found no queue families that support presentation operations.
            return false;
        }
        
        return true;
    }
    
}
