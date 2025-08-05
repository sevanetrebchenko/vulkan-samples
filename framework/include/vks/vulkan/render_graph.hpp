
#ifndef RENDER_GRAPH_HPP
#define RENDER_GRAPH_HPP

#include <vulkan/vulkan.h>

namespace vks {
    
    struct ColorAttachmentDescription {
        VkAttachmentLoadOp load_op;
        VkAttachmentStoreOp store_op;
        VkClearColorValue clear_color;
    };

    class RenderGraph {
        public:
        
            void compile();
            
            void execute();

            void reset();
            
        private:
        
    };

}

#endif // RENDER_GRAPH_HPP
