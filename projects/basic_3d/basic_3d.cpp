
#include "sample.hpp"
#include "helpers.hpp"

// Basic scene for demonstrating rendering of 3D models

class Basic3D final : public Sample {
    public:
        Basic3D() : Sample("Basic 3D") {
        }
        
        ~Basic3D() override {
        }
        
    private:
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        
        VkCommandPool transient_command_pool;
        
        VkQueueFlags request_device_queues() const override {
            return VK_QUEUE_TRANSFER_BIT; // Demo needs transfer support for staging buffers
        }
        
        void render() override {
        }
        
        void record_command_buffers(unsigned image_index) override {
        }
        
        void initialize_pipelines() override {
        }
        
        void destroy_pipelines() override {
        }
        
        void initialize_render_passes() override {
        }
        
        void destroy_render_passes() override {
        }
        
        void initialize_framebuffers() override {
        }
        
        void initialize_resources() override {
            Model bunny = load_model("assets/models/bunny_high_poly.obj");
            
            std::size_t vertex_buffer_size = bunny.vertices.size() * (sizeof(glm::vec3) * 2);
            VkBuffer vertex_staging_buffer { };
            VkDeviceMemory vertex_staging_buffer_memory { };
            
            // In order to be able to write vertex data into the vertex buffer, the buffer needs to (at least) support VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT memory use (+ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, otherwise requires the use of vkFlushMappedMemoryRanges to make the data visible to the GPU)
            // The memory heap this buffer is allocated from is likely not in the optimal layout for the GPU to read from
            // To get around this, this example uses a two phase approach to upload vertex data to the GPU:
            //   1. Create a staging buffer that is visible from the CPU and write the vertex data into this buffer
            //   2. Use a buffer copy command to move the data from the staging buffer (SRC) into device local memory (DST), which has the optimal layout for the GPU to read data from
            // This sample only uses vertex position and normal
            create_buffer(physical_device, device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertex_staging_buffer, vertex_staging_buffer_memory);
            
            // Upload vertex data into vertex staging buffer
            void* data;
            vkMapMemory(device, vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &data);
                // Copy vertex data
                std::size_t offset = 0u;
                for (const Model::Vertex& vertex : bunny.vertices) {
                    memcpy(reinterpret_cast<char*>(data) + offset, &vertex.position, sizeof(glm::vec3));
                    offset += sizeof(vertex.position);
                }
            vkUnmapMemory(device, vertex_staging_buffer_memory);
            
            // Actual vertex buffer residing in device memory
            create_buffer(physical_device, device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);
            
            // Index staging buffer
            std::size_t index_buffer_size = bunny.indices.size() * sizeof(unsigned);
            VkBuffer index_staging_buffer { };
            VkDeviceMemory index_staging_buffer_memory { };
            
            create_buffer(physical_device, device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, index_staging_buffer, index_staging_buffer_memory);
            vkMapMemory(device, vertex_staging_buffer_memory, 0, index_buffer_size, 0, &data);
                memcpy(data, bunny.indices.data(), bunny.indices.size() * sizeof(unsigned));
            vkUnmapMemory(device, vertex_staging_buffer_memory);
            
            create_buffer(physical_device, device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);
            
            // Allocate a transient command pool for transient (one-time) commands
            VkCommandBuffer command_buffer = allocate_transient_command_buffer();
            VkCommandBufferBeginInfo begin_info { };
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(command_buffer, &begin_info);
                VkBufferCopy copy_region { };
                
                // Submit command to copy vertex data
                copy_region.srcOffset = 0;
                copy_region.dstOffset = 0;
                copy_region.size = vertex_buffer_size;
                vkCmdCopyBuffer(command_buffer, vertex_staging_buffer, vertex_buffer, 1, &copy_region);
                
                // Submit command to copy index data
                copy_region.srcOffset = 0;
                copy_region.dstOffset = 0;
                copy_region.size = index_buffer_size;
                vkCmdCopyBuffer(command_buffer, index_staging_buffer, index_buffer, 1, &copy_region);
            vkEndCommandBuffer(command_buffer);
            
            VkSubmitInfo submit_info { };
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

            // This transfer is a one-time operation, easier just to wait for it to complete
            // An alternative approach here would be to use a fence, which would allow scheduling multiple transfers in parallel and give the GPU more opportunities to optimize
            vkQueueWaitIdle(queue);
            
            deallocate_transient_command_buffer(command_buffer);
            
            // Staging buffer resources are no longer necessary
            vkFreeMemory(device, vertex_staging_buffer_memory, nullptr);
            vkDestroyBuffer(device, vertex_staging_buffer, nullptr);
            
            vkFreeMemory(device, index_staging_buffer_memory, nullptr);
            vkDestroyBuffer(device, index_staging_buffer, nullptr);
        }
        
        void destroy_resources() override {
            vkFreeMemory(device, vertex_buffer_memory, nullptr);
            vkDestroyBuffer(device, vertex_buffer, nullptr);
            
            vkFreeMemory(device, index_buffer_memory, nullptr);
            vkDestroyBuffer(device, index_buffer, nullptr);
        }
        
};

DEFINE_SAMPLE_MAIN(Basic3D);
