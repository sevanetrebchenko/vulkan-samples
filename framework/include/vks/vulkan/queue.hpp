
#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vulkan/vulkan.h>
#include <cstdint> // std::uint32_t

namespace vks {
    
    class Queue {
        public:
            Queue();
            Queue(VkQueue queue, std::uint32_t family_index, VkQueueFlags flags, bool is_async);
            
            // Returns whether this queue is a valid queue handle
            [[nodiscard]] operator bool() const;
            
            [[nodiscard]] VkQueue get_queue() const;
            [[nodiscard]] std::uint32_t get_family_index() const;
            
            // Queue properties
            bool supports_graphics() const;
            bool supports_compute() const;
            bool supports_transfer() const;

            // Returns true if this queue can run asynchronously with graphics
            [[nodiscard]] bool is_async() const;
            
        private:
            static constexpr std::uint32_t FAMILY_INDEX_MASK = 0x00FFFFFF;  // bits 0 - 23
            static constexpr std::uint32_t GRAPHICS_BIT = 25;
            static constexpr std::uint32_t COMPUTE_BIT = 26;
            static constexpr std::uint32_t TRANSFER_BIT = 27;
            static constexpr std::uint32_t ASYNC_BIT = 28;
            
            VkQueue m_queue;
            std::uint32_t m_flags;
    };
    
}

#endif // QUEUE_HPP
