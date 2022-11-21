
#include "vulkan/window.hpp"
#include "vulkan/loader.hpp"
#include "vulkan/instance.hpp"

#include <cstring>
#include <iostream>
#include <cstdint>

namespace vks {
    
    #if defined(VKS_PLATFORM_WINDOWS)
    
        LRESULT process_message(HWND handle, std::uint32_t message, WPARAM wparam, LPARAM lparam) {
            std::cout << std::hex << message << std::endl;
            
            // https://www.pinvoke.net/default.aspx/Constants/WM.html
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeydown
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-move
            // https://github.com/travisvroman/kohi/blob/main/engine/src/platform/platform_win32.c
            
            switch (message) {
                // Windows messages:
                case WM_ACTIVATE: {
                    std::cout << "ACTIVATE" << std::endl;
                    return 0;
                }
                
                case WM_ERASEBKGND: {
                    // Non-zero return values signify that the window background is cleared by the application.
                    return 1;
                }
                
                case WM_CLOSE: {
                    // Current window should terminate.
                    std::cout << "CLOSE" << std::endl;
                    return 0;
                }
                
                case WM_DESTROY: {
                    // Current window has been removed from the screen and is being destroyed.
                    std::cout << "DESTROY" << std::endl;
                    return 0;
                }
                
                case WM_MOVE: {
                    // Sent to a window after it has been moved.
                }
                
                case WM_MOVING: {
                    // Sent to a window after
                }
                
                case WM_SIZE: {
                    // Sent to a window after its size was changed.
                }
                
                case WM_SIZING: {
                    // Sent to a window that the user is resizing.
                }
                
                case WM_WINDOWPOSCHANGED: {
                    // Sent to a window whose size, position, or z-order i
                }
                
                // Mouse:
                
                // Keyboard messages:
                
                case WM_SYSKEYDOWN: {
                    // Sent to the window (with keyboard focus) when the ALT key is pressed.
                    std::cout << "WM_SYSKEYDOWN" << std::endl;
                    return 0;
                }
    
                case WM_SYSKEYUP: {
                    // Sent to the window (with keyboard focus) when the ALT key is pressed.
                    std::cout << "WM_SYSKEYUP" << std::endl;
                    return 0;
                }
                
                case WM_KEYDOWN: {
                    // Sent to the window (with keyboard focus) when a non-system key is pressed.
                    std::cout << "WM_KEYDOWN" << std::endl;
                    return 0;
                }
                
                case WM_KEYUP: {
                    // Sent to the window (with keyboard focus) when a non-system key is released.
                    std::cout << "WM_KEYUP" << std::endl;
                    return 0;
                }
                
            }
            
            return DefWindowProc(handle, message, wparam, lparam);
        }
    
    #endif
    
    VulkanWindow::VulkanWindow() {
    }
    
    VulkanWindow::~VulkanWindow() {
    }
    
    VulkanWindow::Builder::Builder(std::shared_ptr<VulkanInstance> instance) : m_instance(instance),
    name_("asdf")
                                                                     #if defined(VKS_PLATFORM_WINDOWS)
                                                                         // fp_vk_create_win32_surface_(reinterpret_cast<typeof(fp_vk_create_win32_surface_)>(detail::vk_get_instance_proc_addr(*instance, "vkCreateWin32SurfaceKHR")))
                                                                     #endif
                                                                     {
    }
    
    VulkanWindow::Builder::~Builder() {
    }
    
    VulkanWindow& VulkanWindow::Builder::build() const {
        #if defined(VKS_PLATFORM_WINDOWS)
            // https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
            
            HINSTANCE instance = GetModuleHandleA(nullptr);
    
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-classes
            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa
            WNDCLASSEXA window_class {
                .cbSize = sizeof(WNDCLASSEXA),
                .style = CS_DBLCLKS,
                .lpfnWndProc = process_message, // Window procedure defines the entry point used to process all the window messages for this class.
                .cbClsExtra = 0,
                .cbWndExtra = 0,
                .hInstance = instance,
                .hIcon = LoadIconA(nullptr, IDI_APPLICATION), // 'Large' icon is displayed in the task-switch window (ALT+TAB) and large icon views in the taskbar / file explorer.
                .hCursor = LoadCursorA(nullptr, IDC_ARROW),
                .hbrBackground = nullptr, // Window must paint its own background on receiving a WM_ERASEBKGND message.
                .lpszMenuName = nullptr,
                .lpszClassName = "vks::VulkanWindow", // Creating a new window class registration.
                .hIconSm = LoadIconA(nullptr, IDI_APPLICATION) // Small icon is displayed in the title bar of the window and in small icon views in the taskbar / explorer.
            };
            
            ATOM result = RegisterClassExA(&window_class);
            if (result == 0) {
                GetLastError(); // TODO:
                throw;
            }
            
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
//            DWORD window_styles = ;
    
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
            DWORD window_extended_styles = WS_EX_ACCEPTFILES | WS_EX_APPWINDOW; // Window accepts drag-and-drop files.
            
//            HWND window = CreateWindowExA(WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME | WS_VISIBLE,
//                                          "vks::VulkanWindow",
//                                          name_, window_styles,         0,
//                                                0,
//                                                800,
//                                                630, NULL, NULL, instance, NULL);
//            if (window == 0) {
//                throw;
//            }
//
//            ShowWindow(window, SW_SHOW);
            
        #endif
    }
    
    VulkanWindow::Builder& VulkanWindow::Builder::with_name(const char* name) {
        if (strlen(name) > 0) {
            name_ = name;
        }
        
        return *this;
    }
    
    VulkanWindow::Builder& VulkanWindow::Builder::with_width(std::uint32_t width) {
        width_ = width;
        return *this;
    }
    
    VulkanWindow::Builder& VulkanWindow::Builder::with_height(std::uint32_t height) {
        height_ = height;
        return *this;
    }
    
}