
#include "vks/window.hpp"
#include <utils/logging.hpp>
#include <vulkan/vk_enum_string_helper.h>

namespace vks {

    Window::Window() {
    }
    
    Window::~Window() {
    }
    
    void Window::initialize(std::uint32_t width, std::uint32_t height, const char* title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        m_width = width;
        m_height = height;
        m_handle = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!m_handle) {
            utils::logging::fatal("Failed to create window");
        }
    }
    
    void Window::shutdown() {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
    }
    
    bool Window::active() const {
        return true;
    }
    
    void Window::close() {
    }
    
    void Window::poll() {
    }
    
    void Window::set_width(std::uint32_t width) {
        m_width = width;
    }
    
    void Window::set_height(std::uint32_t height) {
    }
    
    void Window::set_extent(std::uint32_t width, std::uint32_t height) {
    }
    
    void Window::set_fullscreen() {
    }
    
    std::uint32_t Window::get_width() const {
        return m_width;
    }
    
    std::uint32_t Window::get_height() const {
        return m_height;
    }
    
    VkSurfaceKHR Window::create_surface(VkInstance instance) const {
        if (!m_handle) {
            return nullptr;
        }
        
        VkSurfaceKHR surface;
        
        VkResult result = glfwCreateWindowSurface(instance, m_handle, nullptr, &surface);
        if (result != VK_SUCCESS) {
            utils::logging::fatal("glfwCreateWindowSurface failed with code {} ({})", static_cast<std::underlying_type_t<VkResult>>(result), string_VkResult(result)); \
        }
        
        return surface;
    }
    
    void Window::toggle_fullscreen(bool fullscreen) {
    }
    
}