
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector> // std::vector
#include <unordered_map> // std::unordered_map

class Sample {
    public:
        Sample(const char* name);
        ~Sample();

        void initialize_window(int width = 1920, int height = 1080);
        
        void initialize_vulkan();

        void render();
        
    protected:
        std::vector<const char*> get_supported_instance_extensions() const;
        
        void load_shader(const char* filepath);

        // Section: input
        virtual void on_window_resized(int width, int height);

        virtual void on_key_pressed(int key);
        
        virtual void on_mouse_button_pressed(int button);
        virtual void on_mouse_moved(glm::vec2 position);
        virtual void on_mouse_scrolled(double distance);

        // Map of state for both keyboard and mouse
        // Everything starts out as false (inactive)
        std::unordered_map<int, bool> input;

        bool is_key_down(int key) const;
        glm::vec2 get_mouse_position() const;

        // Section: Vulkan instance
        VkInstance instance;
        std::vector<const char*> enabled_instance_extensions;

        // Section: Vulkan physical device
        VkPhysicalDevice physical_device;
        std::vector<const char*> enabled_device_extensions;
        
        VkPhysicalDeviceProperties physical_device_properties;
        VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
        
        VkPhysicalDeviceFeatures physical_device_features;
        VkPhysicalDeviceFeatures enabled_physical_device_features;
        
        // Section: Vulkan logical device
        VkDevice device;

        VkQueue graphics_queue;

        VkCommandPool command_pool;

        // Section: Synchronization objects
        VkSemaphore is_presentation_complete;
        VkSemaphore is_rendering_complete;
        std::vector<VkFence> fences;

        //
        unsigned width;
        unsigned height;

        GLFWwindow* window;
        const char* name;

        struct Settings {
            bool fullscreen;
            bool debug;
        } settings;

    private:
        // Event dispatch functions (hooked up to window callbacks)
        void on_window_resize(int width, int height);
        void on_key_press(int key);
        void on_mouse_button_press(int button);
        void on_mouse_move(double x, double y);
        void on_mouse_scroll(double distance);
        
        void create_vulkan_instance();
        void initialize_debugging();

        // Rendering loop
        void prepare();
        void submit();
        
        VkDebugUtilsMessengerEXT debug_messenger;

};

#endif // SAMPLE_HPP
