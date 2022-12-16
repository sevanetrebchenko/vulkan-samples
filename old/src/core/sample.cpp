
#include "core/sample.hpp"
#include <vulkan/vulkan.h>

#include <windows.h>
#include <memory>
#include <iostream>

namespace vks {

    bool function(int event) {
        std::cout << "hello from function" << std::endl;
        return false;
    }
    
    template <typename ClassType, typename EventType>
    inline auto callback(std::shared_ptr<ClassType> instance, bool(ClassType::*function)(const EventType&)) {
        std::weak_ptr<ClassType> weak = instance;
        
        return [instance = weak, function](const EventType& e) {
            if (!instance.expired()) {
                return std::mem_fn(function)(instance.lock(), e);
            }
            
            std::cout << "except" << std::endl;
            
            throw std::runtime_error("");
        };
    }
    
    Sample::Sample() : m_instance(VulkanInstance::Builder()
                                      .with_target_runtime_version(1u, 0u)
                                      .with_enabled_extension("")
                                      .with_application_name("")
                                      .build()),
                       m_device(m_instance->create_device().with_supported_queue(VulkanQueue::Operation::TRANSFER | VulkanQueue::Operation::GRAPHICS).build())
                       {
    
//        event_listener.register_event_handler<int>(&function);
//        event_listener.register_event_handler<float>(&Sample::test);
        
//        auto h = detail::vk_get_instance_proc_addr(instance_, "vkGetPhysicalDeviceProperties2KHR");
//        if (!h) {
//            throw std::runtime_error("");
//        }
        
//        instance_.create_device().with_physical_device_selector_function(function);

//        instance_.create_device().with_physical_device_selector_function([this](const VulkanDevice::Properties&, const VulkanDevice::Features&) -> int {
//        });

//        m_instance->create_device().build();
//        m_instance->create_window().build();
        
//        while (true) {
//            MSG message;
//            while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
//                TranslateMessage(&message);
//                DispatchMessageA(&message);
//            }
//        }
    }

    Sample::~Sample() {
    }

} // namespace vks