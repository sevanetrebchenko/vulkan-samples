
#pragma once

// Platform detection.
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    // Targeting Windows.
    #define VKS_PLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on Windows systems."
    #endif
#elif defined(__linux__) || defined(__gnu_linux__)
    // Targeting Linux.
    #define VKS_PLATFORM_LINUX 1
    #if defined(__ANDROID__)
        #define VKS_PLATFORM_ANDROID 1
    #endif
#elif defined(__unix__)
    // Catch anything not caught by the above.
    #define VKS_PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
    // Targeting POSIX.
    #define VKS_PLATFORM_POSIX 1
#elif __APPLE__
    // Targeting Apple platforms.
    #define VKS_PLATFORM_APPLE 1
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        // Targeting iOS Simulator.
        #define VKS_PLATFORM_IOS           1
        #define VKS_PLATFORM_IOS_SIMULATOR 1
    #elif TARGET_OS_IPHONE
        // Targeting iOS device.
        #define VKS_PLATFORM_IOS 1
    #elif TARGET_OS_MAC
    // Other kinds of Mac OS...
    #else
        #error "Unknown Apple platform"
    #endif
#else
    #error "Uknown platform."
#endif

#define NODISCARD [[nodiscard]]

#if !defined(NDEBUG)
    #define VKS_DEBUG 1
#endif
