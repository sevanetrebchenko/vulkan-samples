
#ifndef SHADER_TPP
#define SHADER_TPP

namespace vks {
    
    template <typename T>
    void ShaderCompiler::define_constant(const char* name, T value) {
        using Type = typename std::decay<T>::type;
        static_assert(std::is_same<Type, bool>::value || std::is_same<Type, int>::value || std::is_same<Type, unsigned int>::value || std::is_same<Type, float>::value || std::is_same<Type, double>::value, "shader specialization constant must be one of: bool, int, uint, float, double");

        bool found = false;
        for (ShaderModule::SpecializationConstant& constant : m_module.constants) {
            if (strcmp(constant.name, name) == 0) {
                constant.value = value; // Update value if constant already exists
                constant.size = sizeof(Type);
                found = true;
            }
        }
        
        if (!found) {
            ShaderModule::SpecializationConstant& constant = m_module.constants.emplace_back();
            constant.name = name;
            constant.value = value;
            constant.size = sizeof(Type);
        }
    }
    
}

#endif // SHADER_TPP