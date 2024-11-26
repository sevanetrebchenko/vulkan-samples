
#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

namespace vks {
    
    struct CommandBuffer {

        
        void set_texture();
        void set_buffer();
        
        
        // Rendering state...
        void set_viewport();
        void set_scissor();
        
    };
    
}

#endif // COMMAND_BUFFER_HPP
