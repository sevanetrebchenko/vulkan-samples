
#include <utility>

#include "vks/vulkan/shader.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"

namespace vks {
    
    unsigned to_pipeline_index(VkShaderStageFlags stage) {
        switch (stage) {
            // Graphics pipelines
            case VK_SHADER_STAGE_VERTEX_BIT:
                return 0;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                return 1;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                return 2;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                return 3;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return 4;
            // Compute pipelines
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return 0;
//            // Mesh pipelines
//            case ShaderStage::Mesh:
//                return 0;
//            case ShaderStage::Task:
//                return 1;
            default:
                return -1;
        }
    }

    ShaderStageDescription::ShaderStageDescription() : stage(VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM) {
    }
    
    ShaderStageDescription::~ShaderStageDescription() = default;
    
    ShaderStageDescription& ShaderStageDescription::set_filepath(std::filesystem::path _path) {
        path = std::move(_path);
        
        // Attempt to determine stage from shader extension
        std::filesystem::path extension = path.extension();
        if (extension == ".vert") {
            // Vertex
            stage = VK_SHADER_STAGE_VERTEX_BIT;
        }
        else if (extension == ".tesc") {
            // Tesselation control
            stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        else if (extension == ".tese") {
            // Tesselation evaluation
            stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        else if (extension == ".geom") {
            // Geometry
            stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        else if (extension == ".frag") {
            // Fragment
            stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        else if (extension == ".comp") {
            // Compute
            stage = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        else {
            // Unknown shader type
            std::string error = utils::format("Failed to determine shader type - unknown shader extension '{}'", extension);
            utils::logging::error(error);
            throw std::runtime_error(error);
        }
        
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::set_filepath(std::filesystem::path _path, VkShaderStageFlags _stage) {
        path = std::move(_path);
        stage = _stage;
        return *this;
    }

    ShaderStageDescription& ShaderStageDescription::define_constant(const char* name, bool value) {
        ShaderConstant& constant = get_constant(name);
        constant.value.b = value;
        constant.size = sizeof(bool);
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::define_constant(const char* name, int value) {
        ShaderConstant& constant = get_constant(name);
        constant.value.i = value;
        constant.size = sizeof(int);
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::define_constant(const char* name, unsigned int value) {
        ShaderConstant& constant = get_constant(name);
        constant.value.u = value;
        constant.size = sizeof(unsigned int);
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::define_constant(const char* name, float value) {
        ShaderConstant& constant = get_constant(name);
        constant.value.f = value;
        constant.size = sizeof(float);
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::define_constant(const char* name, double value) {
        ShaderConstant& constant = get_constant(name);
        constant.value.d = value;
        constant.size = sizeof(double);
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::define_macro(const char* name, const char* value) {
        preprocessor_definitions[name] = value;
        return *this;
    }
    
    ShaderConstant& ShaderStageDescription::get_constant(const char* name) {
        for (ShaderConstant& constant : constants) {
            if (strcmp(constant.name, name) == 0) {
                return constant;
            }
        }

        // Create new entry
        ShaderConstant& constant = constants.emplace_back();
        constant.name = name;
        return constant;
    }
    
}

namespace utils {
    
    std::string Formatter<VkShaderStageFlagBits>::format(VkShaderStageFlagBits stage) const {
        using namespace vks;
        const char* name = "";
        
        if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
            name = "vertex";
        }
        else if (stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            name = "tesselation control";
        }
        else if (stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
            name = "tesselation evaluation";
        }
        else if (stage == VK_SHADER_STAGE_GEOMETRY_BIT) {
            name = "geometry";
        }
        else if (stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            name = "fragment";
        }
        else if (stage == VK_SHADER_STAGE_COMPUTE_BIT) {
            name = "compute";
        }
//        else if (stage == ShaderStage::Mesh) {
//            name = "mesh";
//        }
//        else if (stage == ShaderStage::Task) {
//            name = "task";
//        }
        
        return std::move(Formatter<const char*>::format(name));
    }
    
}