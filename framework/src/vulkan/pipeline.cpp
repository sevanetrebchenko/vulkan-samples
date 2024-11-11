
#include <utility>

#include "vks/vulkan/pipeline.hpp"

namespace vks {
    
    VertexInputDescription& VertexInputDescription::add_attribute(unsigned _binding, const char* name) {
        bool binding_exists = false;
        unsigned binding_index = bindings.size();
        
        bool attribute_exists = false;
        
        for (unsigned i = 0; i < bindings.size(); ++i) {
            const VertexBinding& binding = bindings[i];
            if (binding.binding != _binding) {
                continue;
            }
            
            binding_exists = true;
            binding_index = i;
            
            for (const VertexAttribute& attribute : binding.attributes) {
                if (strcmp(attribute.name, name) == 0) {
                    attribute_exists = true;
                    break;
                }
            }
        }
        
        if (!binding_exists) {
            // Register new binding
            VertexBinding& binding = bindings.emplace_back();
            binding.binding = _binding;
            binding.stride = -1; // Placeholder value, will be calculated during pipeline compilation
            binding.rate = VertexInputRate::Vertex; // By default, vertex attributes are updated per vertex
        }
        
        if (!attribute_exists) {
            // Only allow non-duplicate attribute names
            VertexAttribute& attribute = bindings[binding_index].attributes.emplace_back();
            attribute.name = name;
            attribute.location = -1;
            attribute.format = VK_FORMAT_MAX_ENUM;
        }
        
        return *this;
    }
    
    VertexInputDescription& VertexInputDescription::set_binding_stride(unsigned _binding, unsigned _stride) {
        bool binding_exists = false;
        for (VertexBinding& binding : bindings) {
            if (binding.binding != _binding) {
                continue;
            }
            
            binding_exists = true;
            binding.stride = _stride;
        }
        
        if (!binding_exists) {
            // Register new binding
            VertexBinding& binding = bindings.emplace_back();
            binding.binding = _binding;
            binding.stride = _stride;
            binding.rate = VertexInputRate::Vertex; // By default, vertex attributes are updated per vertex
        }
        
        return *this;
    }
    
    VertexInputDescription& VertexInputDescription::set_binding_input_rate(unsigned _binding, VertexInputRate rate) {
        bool binding_exists = false;
        for (VertexBinding& binding : bindings) {
            if (binding.binding != _binding) {
                continue;
            }
            
            binding_exists = true;
            binding.rate = rate;
        }
        
        if (!binding_exists) {
            // Register new binding
            VertexBinding& binding = bindings.emplace_back();
            binding.binding = _binding;
            binding.stride = -1; // Placeholder value, will be calculated during pipeline compilation
            binding.rate = rate;
        }
        
        return *this;
    }
    
    GraphicsPipelineDescription& GraphicsPipelineDescription::add_shader_stage(ShaderStageDescription stage_description) {
        if (stage_description.stage > ShaderStage::Fragment) {
            throw std::runtime_error("invalid stage");
        }
        shader_stages[to_pipeline_index(stage_description.stage)] = std::move(stage_description);
        return *this;
    }
    
    GraphicsPipelineDescription& GraphicsPipelineDescription::set_primitive_topology(VkPrimitiveTopology _primitive_topology) {
        primitive_topology = _primitive_topology;
        return *this;
    }
    
    GraphicsPipelineDescription& GraphicsPipelineDescription::set_rasterization_state(RasterizationState _rasterization_state) {
        rasterization_state = _rasterization_state;
        return *this;
    }
    
    GraphicsPipelineDescription& GraphicsPipelineDescription::set_vertex_input(VertexInputDescription _vertex_input_description) {
        vertex_input_description = std::move(_vertex_input_description);
        return *this;
    }
    
}
