
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

#include <memory>

template <typename T>
struct ManagedObject : public std::enable_shared_from_this<T> {
    virtual ~ManagedObject() = default;
};

#include <cstdint>

// Unsigned data types.
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

// Signed data types.
typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

// Floating point data types.
typedef float f32;
typedef double f64;

#define DEFINE_ENUM_BITFIELD_OPERATIONS(T)                                          \
    NODISCARD inline constexpr T operator|(T first, T second) {                     \
        using U = std::underlying_type<T>::type;                                    \
        return static_cast<T>(static_cast<U>(first) | static_cast<U>(second));      \
    }                                                                               \
    inline constexpr T& operator|=(T& target, T value) {                            \
        return target = target | value;                                             \
    }                                                                               \
    NODISCARD inline constexpr T operator&(T first, T second) {                     \
        using U = std::underlying_type<T>::type;                                    \
        return static_cast<T>(static_cast<U>(first) & static_cast<U>(second));      \
    }                                                                               \
    inline constexpr T& operator&=(T& target, T value) {                            \
        return target = target & value;                                             \
    }                                                                               \
    NODISCARD inline constexpr T operator^(T first, T second) {                     \
        using U = std::underlying_type<T>::type;                                    \
        return static_cast<T>(static_cast<U>(first) ^ static_cast<U>(second));      \
    }                                                                               \
    inline constexpr T& operator^=(T& target, T value) {                            \
        return target = target ^ value;                                             \
    }                                                                               \
    NODISCARD inline constexpr T operator~(T value) {                               \
        using U = std::underlying_type<T>::type;                                    \
        return static_cast<T>(~static_cast<U>(value));                              \
    }                                                                               \
    NODISCARD inline constexpr bool test(T target, T value) {                       \
        return (target & value) == value;                                           \
    }

