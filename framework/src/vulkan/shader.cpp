
#include "vks/vulkan/shader.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"

namespace vks {

    ShaderStageDescription::ShaderStageDescription(std::filesystem::path p) : path(std::move(p)) {
        // Parse shader stage from filepath extension
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("");
        }

        // Determine stage from shader extension
        std::filesystem::path extension = path.extension();
        if (extension == ".vert") {
            stage = ShaderStage::Vertex;
        }
        else if (extension == ".frag") {
            stage = ShaderStage::Fragment;
        }
        else if (extension == ".comp") {
            stage = ShaderStage::Compute;
        }
        else {
            // Unknown shader type
            throw std::runtime_error("");
        }
    }
    
    ShaderStageDescription::ShaderStageDescription(std::filesystem::path p, ShaderStage s) : path(std::move(p)),
                                                                                             stage(s) {
    }
    
    ShaderStageDescription::~ShaderStageDescription() = default;

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