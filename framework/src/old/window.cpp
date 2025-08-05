
#include "window.hpp"
#include "context.hpp"
#include <cassert>

namespace vks {
    
    Window::Window() {
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
