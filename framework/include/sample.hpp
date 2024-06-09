
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector> // std::vector
#include <unordered_map> // std::unordered_map

class Sample {
    public:
        Sample(const char* name = "My Vulkan Sample");
        virtual ~Sample();

        // Configuration
        void set_dimensions(int width, int height);
        void set_headless(bool headless);
        
        void set_debug_mode(bool enabled);
        
        void initialize();
        void run();

        bool active() const;
        
    protected:
        virtual void initialize_resources();
        
        virtual std::vector<const char*> request_instance_extensions() const;
        
        virtual std::vector<const char*> request_device_extensions() const;
        virtual VkPhysicalDeviceFeatures request_device_features() const;
        
        // Application tries to find a dedicated queue of the specified type
        virtual VkQueueFlags request_device_queues() const;
        
        void load_shader(const char* filepath);

        // Section: input
        virtual void on_window_resized(int width, int height);

        virtual void on_key_pressed(int key);
        
        virtual void on_mouse_button_pressed(int button);
        virtual void on_mouse_moved(glm::vec2 position);
        virtual void on_mouse_scrolled(double distance);

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
        
        VkCommandPool command_pool; // Command buffers are allocated from here
        VkQueue queue;
        
        // Section: Windowing
        int width;
        int height;

        GLFWwindow* window;
        const char* name;
        
        VkSurfaceKHR surface;
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkSurfaceFormatKHR surface_format;
        
        // Section: Vulkan swapchain
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        VkExtent2D swapchain_extent;
        VkPresentModeKHR swapchain_present_mode;
        
        std::vector<VkFramebuffer> swapchain_framebuffers;

        struct Settings {
            Settings();
            
            bool fullscreen;
            bool headless;
            bool debug;
        } settings;

    private:
        void initialize_glfw();
        
        void initialize_vulkan();
        
        void create_vulkan_instance();
        void initialize_debugging();
        void create_surface();
        void select_physical_device();
        void create_logical_device();
        
        void initialize_swapchain();
        
        void initialize_window();
        
        // Event dispatch functions (hooked up to window callbacks)
        void on_window_resize(int width, int height);
        void on_key_press(int key);
        void on_mouse_button_press(int button);
        void on_mouse_move(double x, double y);
        void on_mouse_scroll(double distance);
        


        // Rendering loop
        void prepare();
        void submit();
        
        VkDebugUtilsMessengerEXT debug_messenger;
        
        bool initialized;
        bool running;
};

#endif // SAMPLE_HPP
