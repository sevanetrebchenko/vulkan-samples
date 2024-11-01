
#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <glfw/glfw3.h>
#include <memory> // std::shared_ptr

namespace vks {
    
    // Forward declarations
    class Context;
    
    class Window {
        public:
            // Window is initialized by the Context
            class Builder;
            
            Window();
            ~Window();
            
            [[nodiscard]] bool active() const;
            void close();
            
            // Poll operating system for window / input events.
            void poll();
            
            void set_width(unsigned width);
            void set_height(unsigned height);
            void set_extent(unsigned width, unsigned height);
            
            [[nodiscard]] unsigned get_width() const;
            [[nodiscard]] unsigned get_height() const;
            
            VkSurfaceKHR surface;
            
        private:
            void initialize_surface();
            
            // Event dispatch functions (hooked up to window callbacks)
            void on_window_resize(int width, int height);
            void on_key_press(int key);
            void on_mouse_button_press(int button);
            void on_mouse_move(double x, double y);
            void on_mouse_scroll(double distance);
            
            GLFWwindow* m_window;
            const char* m_name;
            
            unsigned m_width;
            unsigned m_height;
    };
    
}

#endif // WINDOW_HPP
