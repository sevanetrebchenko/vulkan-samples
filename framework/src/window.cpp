
#include "window.hpp"
#include "context.hpp"
#include <cassert>

namespace vks {

    void initialize_glfw() {
        // GLFW should only be initialized once
        static bool initialized = false;
        if (!initialized) {
            glfwInit();
            initialized = true;
        }
    }
    
    Window::Window() {
        initialize_glfw();
        
        // In OpenGL, the window and rendering context (instance) are coupled together
        // In Vulkan, the instance is created by the API itself and context creation should be disabled using GLFW_NO_API
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    
    Window::~Window() {
    }
    
    bool Window::active() const {
        return false;
    }
    
    void Window::close() {
    
    }
    
    void Window::poll() {
    
    }
    
    void Window::set_width(unsigned int width) {
    
    }
    
    void Window::set_height(unsigned int height) {
    
    }
    
    void Window::set_extent(unsigned int width, unsigned int height) {
    
    }
    
    unsigned Window::get_width() const {
        return 0;
    }
    
    unsigned Window::get_height() const {
        return 0;
    }
    
    void Window::on_window_resize(int width, int height) {
    
    }
    
    void Window::on_key_press(int key) {
    
    }
    
    void Window::on_mouse_button_press(int button) {
    
    }
    
    void Window::on_mouse_move(double x, double y) {
    
    }
    
    void Window::on_mouse_scroll(double distance) {
    
    }
    
}
