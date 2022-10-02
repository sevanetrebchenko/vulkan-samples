
#pragma once

#include "core/defines.hpp"
#include "vulkan/types.hpp"
#include <vulkan/vulkan.h>
#include <functional>
#include <cstdint>

namespace vks {

    // A VulkanDevice encapsulates both physical hardware and a logical interface with the Vulkan API.
    class VulkanDevice {
        public:
            class Builder;
            
            enum class Type : char {
                OTHER = 0,
                INTEGRATED_GPU = 1,
                DISCRETE_GPU = 2,
                VIRTUAL_GPU = 3,
                CPU = 4
            };
            
            struct Features {
            
            };
        
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html
            struct Limits {
                std::uint32_t max_image_dimension_1d;
                std::uint32_t max_image_dimension_2d;
                std::uint32_t max_image_dimension_3d;
                std::uint32_t max_image_dimension_cube;
                
                std::uint32_t max_image_array_layers;
                
                uint32_t              maxImageArrayLayers;
                uint32_t              maxTexelBufferElements;
                uint32_t              maxUniformBufferRange;
                uint32_t              maxStorageBufferRange;
                uint32_t              maxPushConstantsSize;
                uint32_t              maxMemoryAllocationCount;
                uint32_t              maxSamplerAllocationCount;
                VkDeviceSize          bufferImageGranularity;
                VkDeviceSize          sparseAddressSpaceSize;
                uint32_t              maxBoundDescriptorSets;
                uint32_t              maxPerStageDescriptorSamplers;
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
                VkDeviceSize          minTexelBufferOffsetAlignment;
                VkDeviceSize          minUniformBufferOffsetAlignment;
                VkDeviceSize          minStorageBufferOffsetAlignment;
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
            };
            
            struct Properties {
                Version api_version;
                Type type;
                Limits limits;
                
                // Vendor information.
                std::uint32_t driver_version;
                std::uint32_t vendor_id;
                std::uint32_t device_id;
                const char* device_name;
            };
        
            ~VulkanDevice();
        
            VulkanDevice(VulkanDevice&& other) noexcept;
            VulkanDevice& operator=(VulkanDevice&& other) noexcept;
        
            // Devices should not be copied.
            VulkanDevice(const VulkanDevice& other) = delete;
            VulkanDevice& operator=(const VulkanDevice& other) = delete;
        
            const Properties properties;
            
        private:
            VulkanDevice();
    };
    
    class VulkanInstance;
    
    class VulkanDevice::Builder {
        public:
            Builder(VulkanInstance* instance);
            ~Builder();

            NODISCARD VulkanDevice build();
            
            NODISCARD Builder& with_debug_name(const char* debug_name);
            // Physical device selection.
            // NODISCARD Builder& using_selector(std::function<int(const Properties&)>&& score_fn);

            // Logical device selection.
            NODISCARD Builder& with_extension(const char* extension);
            NODISCARD Builder& with_features(const VulkanDevice::Features& features);
            
            // TODO: queue selection
            // TODO: vks bootstrap
            // TODO: error_code and error_category

        private:
            NODISCARD std::vector<const char*> validate_extensions(VkPhysicalDevice handle) const;
            
            VulkanInstance* instance_;
            std::function<int(const VulkanDevice::Properties&)> selector_;
            
            std::vector<const char*> requested_extensions_;
    };
    
}
