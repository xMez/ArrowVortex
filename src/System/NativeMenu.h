#pragma once

#include <System/System.h>

namespace Vortex::NativeMenu {

enum class WindowKind {
    Win32,
};

struct WindowHandle {
    WindowKind kind = WindowKind::Win32;
    void* value = nullptr;
};

using ActionHandler = void (*)(int actionId, void* userData);

System::MenuItem* createMenuBar();
void attachMenuBar(System::MenuItem* menuBar, WindowHandle window,
                   ActionHandler handler, void* userData);
void detachMenuBar(WindowHandle window);
void shutdown();

}  // namespace Vortex::NativeMenu