
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector> // std::vector
#include <array> // std::array
#include <unordered_map> // std::unordered_map


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
        virtual void render() = 0;
        
        virtual std::vector<const char*> request_instance_extensions() const;
        
        virtual std::vector<const char*> request_device_extensions() const;
        virtual VkPhysicalDeviceFeatures request_device_features() const;
        
        // Application tries to find a dedicated queue of the specified type
        virtual VkQueueFlags request_device_queues() const;
        
        // Detects shader module type from extension
        // .vert - vertex
        // .frag - fragment
        // .comp - compute
        // TODO: more shader types
        VkShaderModule load_shader(const char* filepath);

        // Section: input
        virtual void on_window_resized(int width, int height);

        virtual void on_key_pressed(int key);
        
        virtual void on_mouse_button_pressed(int button);
        virtual void on_mouse_moved(glm::vec2 position);
        virtual void on_mouse_scrolled(double distance);

        bool is_key_down(int key) const;
        glm::vec2 get_mouse_position() const;

        static constexpr int NUM_FRAMES_IN_FLIGHT = 3; // Increasing this number increases rendering latency by that many frames
        
        // Section: Vulkan instance
        VkInstance instance;
        std::vector<const char*> enabled_instance_extensions;

        // Section: Vulkan physical device
        VkPhysicalDevice physical_device;
        std::vector<const char*> enabled_device_extensions;
        
        VkPhysicalDeviceProperties physical_device_properties;
        
        VkPhysicalDeviceFeatures physical_device_features;
        VkPhysicalDeviceFeatures enabled_physical_device_features;
        
        // Section: Vulkan logical device
        VkDevice device;
        
        VkCommandPool command_pool; // Command buffers are allocated from here
        std::array<VkCommandBuffer, NUM_FRAMES_IN_FLIGHT> command_buffers;
        unsigned queue_family_index;
        VkQueue queue;
        VkRenderPass render_pass;
        
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
        std::array<VkImage, NUM_FRAMES_IN_FLIGHT> swapchain_images;
        std::array<VkImageView, NUM_FRAMES_IN_FLIGHT> swapchain_image_views;
        VkExtent2D swapchain_extent;
        VkPresentModeKHR swapchain_present_mode;
        
        // Section: depth buffer
        VkImage depth_buffer;
        VkFormat depth_buffer_format;
        VkImageView depth_buffer_view;
        VkDeviceMemory depth_buffer_memory;
        
        unsigned frame_index; // Index of the current swapchain image
        
        // Framebuffers for final rendering output
        std::array<VkFramebuffer, NUM_FRAMES_IN_FLIGHT> framebuffers;
        
        // Section: synchronization
        std::array<VkSemaphore, NUM_FRAMES_IN_FLIGHT> is_presentation_complete;
        std::array<VkSemaphore, NUM_FRAMES_IN_FLIGHT> is_rendering_complete;
        std::array<VkFence, NUM_FRAMES_IN_FLIGHT> is_frame_in_flight;
        
        struct Settings {
            Settings();
            
            bool fullscreen;
            bool headless;
            bool debug;
            
            // Runtime settings
            bool use_depth_buffer;
        } settings;

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
        
        void create_command_pool(); // Must happen after device queues are selected
        void allocate_command_buffers(); // Allocates command buffers for swapchain images
        virtual void record_command_buffers(unsigned image_index) = 0; // Samples do all the work on how to present to the screen
        void deallocate_command_buffers();
        void destroy_command_pool();
        
        void create_synchronization_objects();
        void destroy_synchronization_objects();
        
        void create_depth_buffer();
        void destroy_depth_buffer();

        // Things for the Sample to do
        virtual void initialize_pipelines() = 0;
        virtual void destroy_pipelines() = 0;
        virtual void initialize_render_passes() = 0;
        virtual void destroy_render_passes() = 0;
        virtual void initialize_framebuffers() = 0;
        void destroy_framebuffers(); // Framebuffers are owned by the base Sample
        
        // For things like vertex buffers, index buffers, etc.
        virtual void initialize_resources() = 0;
        virtual void destroy_resources() = 0;
        
        // Event dispatch functions (hooked up to window callbacks)
        void on_window_resize(int width, int height);
        void on_key_press(int key);
        void on_mouse_button_press(int button);
        void on_mouse_move(double x, double y);
        void on_mouse_scroll(double distance);

        VkDebugUtilsMessengerEXT debug_messenger;
        
        bool initialized;
        bool running;
        
        double dt;
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
