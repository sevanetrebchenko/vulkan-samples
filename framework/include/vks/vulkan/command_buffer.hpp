
#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

#include <string> // std::string

namespace vks {
    
    class CommandBuffer {
        public:
            void bind_pipeline(const std::string& name);
            
            template <typename T>
            void set_uniform(const std::string& name, const T& data);
            
            void bind_texture(const std::string& name, const std::string& resource);
            
            void draw(std::uint32_t vertex_count, std::uint32_t instance_count = 1, std::uint32_t first_vertex = 0, std::uint32_t first_instance = 0);
            void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count = 1, std::uint32_t first_index = 0, std::int32_t vertex_offset = 0, std::uint32_t first_instance = 0);
            void draw_fullscreen_triangle();
            
            // Inline geometry
            template <typename T>
            T* allocate_vertex_buffer()
            
            template <typename Vertex>
            void draw_vertices(const std::vector<Vertex>& vertices);
            
        private:
        
    };
    
}

#endif // COMMAND_BUFFER_HPP
