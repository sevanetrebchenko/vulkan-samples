
#include "window.hpp"
#include "context.hpp"
#include <cassert>

namespace vks {
    
    Window::Builder::Builder(std::shared_ptr<Context> context) : m_handle(std::make_shared<Window>()),
                                                                 m_context(std::move(context)) {
    }
    
    std::shared_ptr<Window> Window::Builder::build() {

        
        m_handle->m_window = glfwCreateWindow((int) m_handle->m_width, (int) m_handle->m_height, m_handle->m_name, nullptr, nullptr);
        assert(m_handle->m_window); // TODO: replace
        
        glfwSetWindowUserPointer(m_handle->m_window, m_handle.get()); // Reference the underlying Window pointer
        
        // Initialize window surface
        // Surface needs to be created after creating the vulkan instance (Vulkan surface may affect physical device selection)
        if (glfwCreateWindowSurface(m_context->instance, m_handle->m_window, nullptr, &m_handle->) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan surface.");
        }
    }

    
}
