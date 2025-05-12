
#ifndef UTILITY_TPP
#define UTILITY_TPP

#include <functional>  // std::hash

namespace vks {
    
    template <typename T>
    void hash_combine(std::size_t& seed, const T& value) {
        std::size_t hash;
        
        if constexpr (std::is_same<std::decay<T>::type, char*>::value) {
            hash = std::hash<std::string_view>{}(value, strlen(value));
        }
        else {
            hash = std::hash<T>{}(value);
        }
        
        // Hashing function inspired by Boost
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    
}

#endif // UTILITY_TPP