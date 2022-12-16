
#include "vulkan/queue.hpp"
#include "vulkan/loader.hpp"
#include "core/debug.hpp"
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"

#include <vulkan/vulkan.h>
#include <utility>

namespace vks {
    
    struct VulkanQueue::Data : public VulkanQueue {
        Data(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<VulkanDevice> device);
        ~Data() override;
        
        NODISCARD bool build();
        
        std::shared_ptr<VulkanInstance> m_instance;
        std::shared_ptr<VulkanDevice> m_device;
        
        VkQueue m_handle;
        VulkanQueue::Operation m_operation;
        i32 m_index;
    };
    
    VulkanQueue::Data::Data(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<VulkanDevice> device) : m_instance(std::move(instance)),
                                                                                                              m_device(std::move(device)),
                                                                                                              m_handle(nullptr),
                                                                                                              m_operation(static_cast<VulkanQueue::Operation>(0u)),
                                                                                                              m_index(-1)
                                                                                                              {
    }
    
    VulkanQueue::Data::~Data() {
    }
    
    bool VulkanQueue::Data::build() {
        static void (VKAPI_PTR* fp_vk_get_device_queue)(VkDevice, u32, u32, VkQueue*) = reinterpret_cast<decltype(fp_vk_get_device_queue)>(detail::vk_get_instance_proc_addr(*m_instance, "vkGetDeviceQueue"));
        if (!fp_vk_get_device_queue) {
            return false;
        }
        
        // Ensure request to build is being done on at least one valid operation.
        if (((m_operation & Operation::GRAPHICS) == Operation::GRAPHICS) ||
            ((m_operation & Operation::PRESENTATION) == Operation::PRESENTATION) ||
            ((m_operation & Operation::COMPUTE) == Operation::COMPUTE) ||
            ((m_operation & Operation::TRANSFER) == Operation::TRANSFER)) {
            
        }

        if (m_index) {
            return false;
        }
        
        // Retrieve queue handle.
        fp_vk_get_device_queue(*m_device, m_index, 0u, &m_handle);
        return true;
    }
    
    VulkanQueue::VulkanQueue() {
    }
    
    VulkanQueue::~VulkanQueue() {
    }
    
    VulkanQueue::Builder::Builder(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<VulkanDevice> device) : m_queue(std::make_shared<VulkanQueue::Data>(std::move(instance), std::move(device))) {
    }
    
    VulkanQueue::Builder::~Builder() {
    }
    
    std::shared_ptr<VulkanQueue> VulkanQueue::Builder::build() {
        std::shared_ptr<VulkanQueue::Data> data = std::reinterpret_pointer_cast<VulkanQueue::Data>(m_queue);
        if (!data->build()) {
            return std::shared_ptr<VulkanQueue>(); // nullptr
        }
    
        std::shared_ptr<VulkanInstance> instance = data->m_instance;
        std::shared_ptr<VulkanDevice> device = data->m_device;
        
        // Builder should not maintain ownership over created queues, and allow for subsequent calls to 'build' to generate valid standalone queues.
        std::shared_ptr<VulkanQueue> queue = std::move(m_queue);
        m_queue = std::make_shared<VulkanQueue::Data>(instance, device); // Ensure builder validity after transferring ownership.
        return std::move(queue);
    }
    
    VulkanQueue::Builder& VulkanQueue::Builder::with_operation(VulkanQueue::Operation operation, u32 queue_family_index) {
        std::shared_ptr<VulkanQueue::Data> data = std::reinterpret_pointer_cast<VulkanQueue::Data>(m_queue);
        data->m_operation = operation;
        data->m_index = static_cast<i32>(queue_family_index);
        return *this;
    }
    
}


