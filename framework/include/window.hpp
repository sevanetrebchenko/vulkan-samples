
#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <glfw/glfw3.h>
#include <memory> // std::shared_ptr

namespace vks {
    
    // Forward declarations
    class Context;
    
    class Window {
        public:
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
            
        private:
            // Event dispatch functions (hooked up to window callbacks)
            void on_window_resize(int width, int height);
            void on_key_press(int key);
            void on_mouse_button_press(int button);
            void on_mouse_move(double x, double y);
            void on_mouse_scroll(double distance);
            
            GLFWwindow* m_window;
            const char* m_name;
            
            VkSurfaceKHR m_surface;
            
            unsigned m_width;
            unsigned m_height;
    };
    
    class Window::Builder {
        public:
            Builder(std::shared_ptr<Context> context);
            ~Builder();
            
            [[nodiscard]] std::shared_ptr<Window> build();
            
            Builder& set_width(unsigned width);
            Builder& set_height(unsigned height);
            
            Builder& set_name(const char* name);
            
        private:
            std::shared_ptr<Context> m_context;
            std::shared_ptr<Window> m_handle;
    };
    
}

#endif // WINDOW_HPP
