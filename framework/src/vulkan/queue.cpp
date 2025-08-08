
#include "vks/vulkan/queue.hpp"

namespace vks {
    
    Queue::Queue() : m_queue(VK_NULL_HANDLE),
                     m_flags(0) {
    }
    
    Queue::Queue(VkQueue queue, std::uint32_t family_index, VkQueueFlags flags, bool is_async) : m_queue(queue) {
        m_flags = family_index & FAMILY_INDEX_MASK;
        
        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            m_flags |= (1 << GRAPHICS_BIT);
        }
        if (flags & VK_QUEUE_COMPUTE_BIT) {
            m_flags |= (1 << COMPUTE_BIT);
        }
        if (flags & VK_QUEUE_TRANSFER_BIT) {
            m_flags |= (1 << TRANSFER_BIT);
        }
        
        m_flags |= (is_async << ASYNC_BIT);
    }
    
    Queue::operator bool() const {
        return m_queue;
    }
    
    VkQueue Queue::get_queue() const {
        return m_queue;
    }
    
    std::uint32_t Queue::get_family_index() const {
        return m_flags & FAMILY_INDEX_MASK;
    }
    
    bool Queue::supports_graphics() const {
        return m_flags & (1 << GRAPHICS_BIT);
    }
    
    bool Queue::supports_compute() const {
        return m_flags & (1 << COMPUTE_BIT);
    }
    
    bool Queue::supports_transfer() const {
        return m_flags & (1 << TRANSFER_BIT);
    }
    
    bool Queue::is_async() const {
        return m_flags & (1 << ASYNC_BIT);
    }
    
}