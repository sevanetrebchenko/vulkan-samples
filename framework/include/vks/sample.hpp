
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "vks/sample/requirements.hpp"
#include "vks/window.hpp"
#include "vks/vulkan/render_context.hpp"
#include <utils/enum.hpp>
#include <string> // std::string

namespace vks {
    
    class Sample {
        public:
            Sample(std::string name);
            virtual ~Sample();
            
            void initialize();
            void run();
            void shutdown();
            
        protected:
        
        private:
            virtual SampleRequirements get_requirements();
            
            virtual void load() = 0;
            virtual void update(float dt) = 0;
            virtual void render_frame(RenderContext& context) = 0;
            virtual void unload() = 0;
            
            std::string m_name;
            Window m_window;
            RenderContext m_render_context;
    };
    
}

#endif // SAMPLE_HPP
