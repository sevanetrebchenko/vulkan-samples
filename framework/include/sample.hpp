
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "camera.hpp"
#include "model.hpp"
#include "transform.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector> // std::vector
#include <array> // std::array
#include <unordered_map> // std::unordered_map
#include <stdexcept> // std::runtime_error
#include <iostream> // std::cout, std::endl

// To make writing Vulkan applications easier, the Sample base class aims to set up a lot of the boilerplate for the user
// Currently, the Sample is responsible for:
//   - Initializing the Vulkan instance
//   - Initializing Vulkan debugging
//   - Selecting a physical device
//      - Overrides for extensions + enabled device features
//   - Creating the logical device
//   - Initializing the window
//   - Initializing the swapchain + retrieving swapchain images
//   - Allocating command buffers, one per swapchain image, to record final rendering commands to
//   - Creating and initializing the scene depth buffer
//   - Allocating basic synchronization primitives to aid in presentation


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
        void shutdown();

        bool active() const;
        
    protected:
        // For initializing things like vertex buffers, index buffers, etc.
        virtual void initialize_resources() = 0;

        // For any updates that need to happen
        virtual void update() = 0;
        
        // Passes the index of the swapchain image retrieved for presentation (1:1 mapping for final present framebuffer)
        virtual void record_command_buffers(unsigned framebuffer_index) = 0;

        // Framebuffers and pool resources are cleaned up automatically
        virtual void destroy_resources() = 0;
        
        virtual void on_window_resized(int width, int height);
        virtual void on_key_pressed(int key);
        virtual void on_mouse_button_pressed(int button);
        virtual void on_mouse_moved(glm::vec2 position);
        virtual void on_mouse_scrolled(double distance);

        // Helper vulkan functions
        VkCommandBuffer begin_transient_command_buffer();
        void submit_transient_command_buffer(VkCommandBuffer command_buffer); // Automatically calls vkEndCommandBuffer
        
        void initialize_descriptor_pool(unsigned buffer_count, unsigned sampler_count);
        
        int NUM_FRAMES_IN_FLIGHT = 3; // Increasing this number increases rendering latency by that many frames
        
        // May not correspond 1:1 with the resolution of swapchain internals due to display properties (the resolution of high-density displays does not necessarily match pixel data)
        int width;
        int height;
        
        GLFWwindow* window;
        const char* name;
        
        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        
        // Any instance extensions required by the sample must be added to this list during sample construction
        std::vector<const char*> enabled_instance_extensions;
        
        // Swapchain data
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        VkPresentModeKHR swapchain_present_mode;
        VkExtent2D swapchain_extent;
        
        VkPhysicalDevice physical_device;
        VkPhysicalDeviceProperties physical_device_properties;
        VkPhysicalDeviceFeatures physical_device_features;
        
        // Any physical device features required by the sample must be toggled during sample construction
        VkPhysicalDeviceFeatures enabled_physical_device_features;
        
        VkDevice device;
        
        // Any device extensions required by the sample must be added to this list during sample construction
        std::vector<const char*> enabled_device_extensions;
        
        // Base Sample tries to find a queue family with support for all queue operations requested in 'enabled_queue_types'
        // Should be done during sample construction (contains only GRAPHICS by default)
        VkQueueFlags enabled_queue_types;
        
        unsigned queue_family_index;
        VkQueue queue;
        
        VkSurfaceKHR surface;
        VkSurfaceFormatKHR surface_format;
        VkSurfaceCapabilitiesKHR surface_capabilities;
        
        unsigned frame_index; // Index of the current swapchain image
        std::vector<VkCommandBuffer> command_buffers;
        
        // Framebuffers for final rendering output
        std::vector<VkFramebuffer> present_framebuffers;
        
        // Samples only use one depth buffer
        VkImage depth_buffer;
        VkDeviceMemory depth_buffer_memory;
        VkFormat depth_buffer_format;
        VkImageView depth_buffer_view;
        
        // Synchronization objects
        std::vector<VkSemaphore> is_presentation_complete;
        std::vector<VkSemaphore> is_rendering_complete;
        std::vector<VkFence> is_frame_in_flight;
        
        VkCommandPool command_pool;
        VkCommandPool transient_command_pool; // For short-lived commands
        
        VkDescriptorPool descriptor_pool;
        
        struct Settings {
            Settings();
            
            bool fullscreen;
            bool headless;
            bool debug;
            
            // Runtime settings
            bool use_depth_buffer;
        } settings;
        
        Camera camera;
        double dt;
        
    private:
        void initialize_glfw();
        void shutdown_glfw();
        
        void create_vulkan_instance();
        void destroy_vulkan_instance();
        
        void initialize_window();
        void destroy_window();
        
        void create_surface();
        void destroy_surface();
        
        // Selecting a suitable physical device depends on a number of different criteria, including:
        //   - supported device features
        //   - available queue family types and count
        //   - available surface formats / properties
        void select_physical_device();
        void destroy_physical_device();
        
        // Initializing a logical device consists of:
        //   - enabling device extensions
        //   - enabling device features (verified during physical device selection)
        //   - retrieving queue families + creating queues
        void create_logical_device();
        void destroy_logical_device();
        
        // Things like command pools / command buffers, pipelines, framebuffers, and synchronization objects are used once the logical device is created
        
        void initialize_swapchain();
        void destroy_swapchain();
        
        void create_command_pools(); // Must happen after device queues are selected
        void allocate_command_buffers();
        void destroy_command_pool();
        
        void create_synchronization_objects();
        void destroy_synchronization_objects();
        
        void create_depth_buffer();
        void destroy_depth_buffer();
        
        void destroy_framebuffers();
        
        void destroy_descriptor_pool();
        
        // Event dispatch functions (hooked up to window callbacks)
        void on_window_resize(int width, int height);
        void on_key_press(int key);
        void on_mouse_button_press(int button);
        void on_mouse_move(double x, double y);
        void on_mouse_scroll(double distance);
        
        // Invoking sample rendering and presenting the results to the screen is handled by the base
        virtual void render();
        
        // Sample data
        bool initialized;
        bool running;
        
        double last_frame_time;
        double frame_time_accumulator;
        std::size_t frame_count;
};

#define DEFINE_SAMPLE_MAIN(TYPE)            \
int main() {                                \
    Sample* sample = new TYPE();            \
    try {                                   \
        sample->initialize();               \
    }                                       \
    catch (std::runtime_error& e) {         \
        std::cerr << e.what() << std::endl; \
        return 1;                           \
    }                                       \
    while (sample->active()) {              \
        sample->run();                      \
    }                                       \
    sample->shutdown();                     \
    delete sample;                          \
    return 0;                               \
}


#endif // SAMPLE_HPP
