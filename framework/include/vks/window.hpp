
#ifndef WINDOW_HPP
#define WINDOW_HPP

#define NOMINMAX
#include <vulkan/vulkan.h> // Needs to come before GLFW include
#include <GLFW/glfw3.h>
#include <cstdint> // std::uint32_t

namespace vks {
    
    class Window {
        public:
            Window();
            ~Window();
            
            void initialize(std::uint32_t width, std::uint32_t height, const char* title);
            void shutdown();
            
            [[nodiscard]] bool active() const;
            void close();
            
            // Poll operating system for window / input events
            void poll();
            
            // Switches display to windowed (if fullscreen)
            // TODO: windows support rendering to a smaller resolution at fullscreen
            void set_width(std::uint32_t width);
            void set_height(std::uint32_t height);
            void set_extent(std::uint32_t width, std::uint32_t height);
            
            void set_fullscreen();
            
            [[nodiscard]] std::uint32_t get_width() const;
            [[nodiscard]] std::uint32_t get_height() const;
            
            VkSurfaceKHR create_surface(VkInstance instance) const;
            
        private:
            void toggle_fullscreen(bool fullscreen);
            
            GLFWwindow* m_handle;
            const char* m_title;
            
            bool m_active;
            bool m_fullscreen;
            
            std::uint32_t m_width;
            std::uint32_t m_height;
    };
    
}

#endif // WINDOW_HPP
