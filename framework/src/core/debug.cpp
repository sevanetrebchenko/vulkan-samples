
#include "core/debug.hpp"
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace vks {
    
    VulkanException::VulkanException(const char* format, ...) : std::runtime_error(""),
                                                                message_()
                                                    {
        std::va_list args;
        va_start(args, format);
    
        va_list args_copy;
        va_copy(args_copy, args);
    
        // Function returns the number of characters (without null-terminator) that would have been written.
        int num_characters = vsnprintf(nullptr, 0u, format, args_copy);
    
        if (num_characters < 0) {
            va_end(args_copy);
            va_end(args);
        
            throw std::runtime_error("ERROR: Encoding error occurred attempting to process VulkanException.");
        }
    
        message_.resize(num_characters + 1);
    
        // Resulting buffer is guaranteed to be long enough to hold the resulting string and be null-terminated.
        vsnprintf(message_.data(), message_.size(), format, args);
    
        va_end(args_copy);
        va_end(args);
    }
    
    VulkanException::~VulkanException() = default;
    
    const char* VulkanException::what() const noexcept {
        return message_.c_str();
    }
    
    void debug_assert(const char* expression, bool result, const char* file, const char* function, int line, const char* format, ...) {
        if (result) {
            return;
        }
        
        std::stringstream builder;
        
        std::va_list args;
        va_start(args, format);
        
        va_list args_copy;
        va_copy(args_copy, args);
        
        // Function returns the number of characters (without null-terminator) that would have been written.
        int num_characters = vsnprintf(nullptr, 0u, format, args_copy);
        
        if (num_characters < 0) {
            va_end(args_copy);
            va_end(args);
            
            builder << "ERROR: Encoding error occurred attempting to process assertion '" << expression << "' in " << function << ", " << file << ':' << line;
            throw std::runtime_error(builder.str());
        }
        
        std::string output;
        output.resize(num_characters + 1);
        
        // Resulting buffer is guaranteed to be long enough to hold the resulting string and be null-terminated.
        vsnprintf(output.data(), output.size(), format, args);
        
        va_end(args_copy);
        va_end(args);
        
        builder << "ERROR: Assertion '" << expression << "' failed in " << function << ", " << file << ':' << line << ": " << output;
        throw std::runtime_error(builder.str());
    }
    
}

