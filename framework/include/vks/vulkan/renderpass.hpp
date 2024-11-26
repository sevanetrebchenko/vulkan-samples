
#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP

#include <vulkan/vulkan.h>

namespace vks {
    
    struct AttachmentDescription {
        unsigned width;
        unsigned height;
        VkFormat format;
        const char* name;
        unsigned sample_count;
        unsigned level_count;
        unsigned layer_count;
        
        bool persistent; // Whether this attachment exists across multiple frames
        // Transient attachments are created inline by the frame graph
    };
    
    struct BufferDescription {
        VkBufferUsageFlags usage;
        bool persistent;
    };
    
    
    struct RenderPassDescription {
        RenderPassDescription& add_attachment(const char* name, AttachmentDescription attachment_description);
    };
    
    struct RenderPass {
    
    };
    
}

#endif // RENDERPASS_HPP


// Setup phase
// 1. declare rendering pass
//    define inputs and outputs for each pass
//    resources must declare all used resources (read, write, create)
//    other resources may be imported into the frame graph
