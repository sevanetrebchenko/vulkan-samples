
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
    
    const char* to_string(VkResult result) {
        switch (result) {
            case VK_SUCCESS:
                return "VK_SUCCESS";

            case VK_NOT_READY:
                return "VK_NOT_READY";
                
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
                
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
                
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
                
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
    
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
    
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
                
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
                
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
                
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
                
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
                
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
                
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
                
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
                
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
                
            case VK_ERROR_UNKNOWN:
                return "VK_ERROR_UNKNOWN";
                
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return "VK_ERROR_OUT_OF_POOL_MEMORY";
                
            case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
                
            case VK_ERROR_FRAGMENTATION:
                return "VK_ERROR_FRAGMENTATION";
                
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
                return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
                
            case VK_PIPELINE_COMPILE_REQUIRED:
                return "VK_PIPELINE_COMPILE_REQUIRED";
                
            case VK_ERROR_SURFACE_LOST_KHR:
                return "VK_ERROR_SURFACE_LOST_KHR";
                
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
                
            case VK_SUBOPTIMAL_KHR:
                return "VK_SUBOPTIMAL_KHR";
                
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "VK_ERROR_OUT_OF_DATE_KHR";
                
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
                
            case VK_ERROR_VALIDATION_FAILED_EXT:
                return "VK_ERROR_VALIDATION_FAILED_EXT";
                
            case VK_ERROR_INVALID_SHADER_NV:
                return "VK_ERROR_INVALID_SHADER_NV";
                
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
                return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
                
            case VK_ERROR_NOT_PERMITTED_KHR:
                return "VK_ERROR_NOT_PERMITTED_KHR";
                
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
                
            case VK_THREAD_IDLE_KHR:
                return "VK_THREAD_IDLE_KHR";
                
            case VK_THREAD_DONE_KHR:
                return "VK_THREAD_DONE_KHR";
                
            case VK_OPERATION_DEFERRED_KHR:
                return "VK_OPERATION_DEFERRED_KHR";
                
            case VK_OPERATION_NOT_DEFERRED_KHR:
                return "VK_OPERATION_NOT_DEFERRED_KHR";
                
            case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
                return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
                
            default:
                return "UNKNOWN";
        }
    }
    
}

