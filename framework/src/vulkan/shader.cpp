
#include <utility>

#include "vks/vulkan/shader.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"

namespace vks {
    
    unsigned to_pipeline_index(ShaderStage stage) {
        switch (stage) {
            // Graphics pipelines
            case ShaderStage::Vertex:
                return 0;
            case ShaderStage::TesselationControl:
                return 1;
            case ShaderStage::TesselationEvaluation:
                return 2;
            case ShaderStage::Geometry:
                return 3;
            case ShaderStage::Fragment:
                return 4;
            // Compute pipelines
            case ShaderStage::Compute:
                return 0;
            // Mesh pipelines
            case ShaderStage::Mesh:
                return 0;
            case ShaderStage::Task:
                return 1;
            case ShaderStage::None:
                break;
        }
        
        return -1;
    }

    ShaderStageDescription::ShaderStageDescription() : stage(ShaderStage::None) {
    }
    
    ShaderStageDescription::~ShaderStageDescription() = default;
    
    ShaderStageDescription& ShaderStageDescription::set_filepath(std::filesystem::path _path) {
        path = std::move(_path);
        
        // Attempt to determine stage from shader extension
        std::filesystem::path extension = path.extension();
        if (extension == ".vert") {
            // Vertex
            stage = ShaderStage::Vertex;
        }
        else if (extension == ".tesc") {
            // Tesselation control
            stage = ShaderStage::TesselationControl;
        }
        else if (extension == ".tese") {
            // Tesselation evaluation
            stage = ShaderStage::TesselationEvaluation;
        }
        else if (extension == ".geom") {
            // Geometry
            stage = ShaderStage::Geometry;
        }
        else if (extension == ".frag") {
            // Fragment
            stage = ShaderStage::Fragment;
        }
        else if (extension == ".comp") {
            // Compute
            stage = ShaderStage::Compute;
        }
        else {
            // Unknown shader type
            std::string error = utils::format("Failed to determine shader type - unknown shader extension '{}'", extension);
            utils::logging::error(error);
            throw std::runtime_error(error);
        }
        
        return *this;
    }
    
    ShaderStageDescription& ShaderStageDescription::set_filepath(std::filesystem::path _path, ShaderStage _stage) {
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
    
    std::string Formatter<vks::ShaderStage>::format(vks::ShaderStage stage) const {
        using namespace vks;
        const char* name = "";
        
        if (stage == ShaderStage::Vertex) {
            name = "vertex";
        }
        else if (stage == ShaderStage::TesselationControl) {
            name = "tesselation control";
        }
        else if (stage == ShaderStage::TesselationEvaluation) {
            name = "tesselation evaluation";
        }
        else if (stage == ShaderStage::Geometry) {
            name = "geometry";
        }
        else if (stage == ShaderStage::Fragment) {
            name = "fragment";
        }
        else if (stage == ShaderStage::Compute) {
            name = "compute";
        }
        else if (stage == ShaderStage::Mesh) {
            name = "mesh";
        }
        else if (stage == ShaderStage::Task) {
            name = "task";
        }
        
        return std::move(Formatter<const char*>::format(name));
    }
    
}