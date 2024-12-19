
#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

namespace vks {
    
    class CommandBuffer {
        public:
            void begin();
            void submit();
    
            void bind_pipeline();
    
            // Maybe include an optional size parameter (this interface has no validation checks and will just read as much as the uniform requires)
            // Technically, uniform buffers exist outside of the scope of command buffers, as in they can be updated outside of command buffer recording
            // TODO: is this a race condition? Should there be as many uniform buffers as there are frames in flight? This way, updating uniform buffers will have to wait on a fence to ensure that the data is not changed while in use on the GPU
            // Separating the two will allow for more optimized runtime - the application will be able to update uniform values for future frames, the GPU will have to wait on a semaphore to ensure that all uniform values for that frame are updated
            // Submitting the command buffer (VkSubmitInfo) should wait on all uniforms to be updated (need a custom uniform semaphore provided in pWaitSemaphores)
            // Note that this should be done in parallel (updating uniform buffers, recording command buffers) since these two things don't overlap
            // This is a good thing to verify, especially using dynamic buffer offsets - a change in the number of objects, for example, should wait until the end of the current frame and be done before the start of the next one, for example (to avoid the GPU expecting a different size uniform buffer than the one provided)
            void update_uniform(const char* name, const void* data);
            void update_uniform_block(unsigned set, unsigned binding, const void* data);
    
            // Persistent uniform buffers should be a wrapper to allow for fragmented uniform buffer data
            // This reduces memory churn by allowling dynamically resizable buffers that don't need to be contiguous in memory (since there is no requirement for them to be)
            // This further places emphasis on dynamic uniforms
            // Buffer allocation strategy can be handled by descriptor set index or an optional override
            // Static uniform buffers will be contiguous, while dynamic ones are allowed to be fragmented across memory
    
            void update_push_constants(const char* name, const void* data);
            void update_push_constants(ShaderStage stage, const void* data); // Update the push constants for this shader stage
            void update_push_constants(const void* data);                    // Update the entire push constant block
    
            // For existing meshes / predefined and stored elsewhere
            // TODO: vertex data can be pulled from multiple vertex / index buffers, not sure if this will be necessary but a good thing to know
            // This is done by specifying multiple VkBuffer objects and VkDeviceSize offsets to vkCmdBindVertexBuffers (going to assume buffers are read from sequentially)
            // This can also be used below for allocating dynamic buffers for vertex data
            void bind_vertex_buffer();
            void bind_index_buffer();
    
            // For dynamic meshes // TODO: look into this
            // This will probably required a host_visible buffer to be able to memcpy vertex data into
            // Will return a pointer to the start range
            // The user will be responsible to aligning the vertex data based on the layout of the vertex data expected in the shader (not much the renderer can do about this)
            void* allocate_vertex_buffer(unsigned binding, unsigned size, unsigned stride);
            void* allocate_index_buffer(unsigned size, unsigned type); // need to specify index type, such as VK_INDEX_TYPE_UINT16
            // TODO: potential use case for helper functions to copy data over directly, instead of returning a void*
            // This data is by nature going to be temporary, so no staging buffer required (this will need profiling to see if it has a big impact on performance)
    
            void begin_render_pass();
            
        private:
        
    };
    
}

#endif // COMMAND_BUFFER_HPP
