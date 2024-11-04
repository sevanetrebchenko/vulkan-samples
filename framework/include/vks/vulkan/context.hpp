
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "vks/vulkan/pipeline.hpp"
#include <vulkan/vulkan.h>

namespace vks {

    struct Context {
        void create_pipeline(const PipelineDescription& pipeline_description);
        
        VkInstance instance;
        VkPhysicalDevice gpu;
        VkDevice device;
    };
    

}

#endif // CONTEXT_HPP
