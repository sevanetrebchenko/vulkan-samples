
#pragma once

#include "core/defines.hpp"
#include <memory>
#include <functional>


namespace vks {
    
    template <typename T>
    class EventListener {
        public:
            EventListener(std::shared_ptr<T> instance) : instance(instance) {
            }
            ~EventListener() = default;
            
            template <typename EventType>
            void register_event_handler(bool(T::*handler)(EventType)) {
                if (!instance.expired()) {
                    std::mem_fn(handler)(instance.lock(), EventType{ });
                }
            }
            
            template <typename EventType>
            void register_event_handler(bool(*handler)(EventType)) {
                handler(EventType { });
            }
            
        private:
            std::weak_ptr<T> instance;
            
            std::vector<std::function<bool(void*)>> listeners;
    };
    
}