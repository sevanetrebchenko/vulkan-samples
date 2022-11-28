
#pragma once

#include "core/defines.hpp"

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    class VulkanDevice;
    
    class VulkanQueue : public ManagedObject<VulkanQueue> {
        public:
            class Builder;
            
            enum class Operation : u8 {
                GRAPHICS     = 1u << 0u,
                PRESENTATION = 1u << 1u,
                COMPUTE      = 1u << 2u,
                TRANSFER     = 1u << 3u
            };
            
            ~VulkanQueue() override;
            
        private:
            class Data;
            
            VulkanQueue();
    };
    
    DEFINE_ENUM_BITFIELD_OPERATIONS(VulkanQueue::Operation);
    
    class VulkanQueue::Builder {
        public:
            Builder(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<VulkanDevice> device);
            ~Builder();
            
            NODISCARD std::shared_ptr<VulkanQueue> build();
            
            Builder& with_operation(VulkanQueue::Operation operation, u32 queue_family_index);
            
        private:
            std::shared_ptr<VulkanQueue> m_queue;
    };
    
}