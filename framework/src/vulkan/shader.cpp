
#include "vks/vulkan/shader.hpp"

namespace vks {

    const ShaderModule& ShaderCache::get_shader_module(const ShaderStageDescription& description) {
        // Shared (read) lock
        // Multiple get_shader_module calls can run in parallel, but recompilation gets exclusive access
        std::shared_lock read_lock(m_cache_mutex);
        
        std::size_t index = m_description_to_index[description];
        if (index >= m_modules.size()) {
            // Shader does not exist
        }
        else {
            CachedShaderModule& module = m_modules[index];
            if (module.needs_recompilation.load()) {
                // Shaders are compiled when they are requested
                read_lock.unlock();
                std::unique_lock write_lock(m_cache_mutex);
                if (module.needs_recompilation.load()) {
                    compile_shader(description);
                }
            }
        }
        
        
//        auto& entry = m_modules[idx];
//
//        if (entry.needs_recompile.load()) {
//            // Shaders are compiled only when they are requested
//            lock.unlock();
//            std::unique_lock write_lock(m_cache_mutex);
//            // Double-check after acquiring write lock
//            if (entry.needs_recompile.load()) {
//                recompile_shader(entry);
//                entry.needs_recompile.store(false);
//            }
//        }
//
//        return entry.module;

        return { };
    }
    
    void ShaderCache::invalidate_shader_variants(const std::filesystem::path& filepath) {
        std::shared_lock lock(m_cache_mutex);
        if (auto it = m_filepath_to_index.find(filepath); it != m_filepath_to_index.end()) {
            for (std::size_t index : it->second) {
                m_modules[index].needs_recompilation.store(true);
            }
        }
    }
    
    void ShaderCache::compile_shader(const ShaderStageDescription& description) {
    
    }

}