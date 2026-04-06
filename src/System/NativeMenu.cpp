#include <System/NativeMenu.h>

#include <Core/WideString.h>

#include <memory>
#include <vector>

#include <windows.h>

namespace Vortex {

struct System::MenuItem::Impl {
    HMENU handle = nullptr;
    bool isMenuBar = false;
};

System::MenuItem::MenuItem(Impl* impl) : myImpl(impl) {}

namespace {

static constexpr wchar_t kNativeMenuHookPropName[] =
    L"ArrowVortex.NativeMenuHook";

struct WindowHookState {
    HWND hwnd = nullptr;
    WNDPROC originalProc = nullptr;
    NativeMenu::ActionHandler actionHandler = nullptr;
    void* userData = nullptr;
};

class NativeMenuBackend {
   public:
    System::MenuItem* createMenu(bool isMenuBar) {
        HMENU handle = isMenuBar ? CreateMenu() : CreatePopupMenu();
        auto menu = std::unique_ptr<System::MenuItem>(new System::MenuItem(
            new System::MenuItem::Impl{handle, isMenuBar}));
        auto* rawMenu = menu.get();
        myMenus.push_back(std::move(menu));
        return rawMenu;
    }

    void addSeparator(System::MenuItem* menu) {
        if (!menu) return;
        AppendMenuW(getHandle(menu), MF_SEPARATOR, 0, nullptr);
    }

    void addItem(System::MenuItem* menu, int item, const std::string& text) {
        if (!menu) return;
        AppendMenuW(getHandle(menu), MF_STRING, item, Widen(text).c_str());
    }

    void addSubmenu(System::MenuItem* menu, System::MenuItem* submenu,
                    const std::string& text, bool grayed) {
        if (!menu || !submenu) return;
        UINT flags = MF_STRING | MF_POPUP | (grayed ? MF_GRAYED : 0);
        AppendMenuW(getHandle(menu), flags,
                    reinterpret_cast<UINT_PTR>(getHandle(submenu)),
                    Widen(text).c_str());
    }

    void replaceSubmenu(System::MenuItem* menu, int pos,
                        System::MenuItem* submenu, const std::string& text,
                        bool grayed) {
        if (!menu || !submenu) return;
        UINT flags =
            MF_BYPOSITION | MF_STRING | MF_POPUP | (grayed ? MF_GRAYED : 0);
        DeleteMenu(getHandle(menu), pos, MF_BYPOSITION);
        InsertMenuW(getHandle(menu), pos, flags,
                    reinterpret_cast<UINT_PTR>(getHandle(submenu)),
                    Widen(text).c_str());
    }

    void setChecked(System::MenuItem* menu, int item, bool checked) {
        if (!menu) return;
        CheckMenuItem(getHandle(menu), item,
                      MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
    }

    void setEnabled(System::MenuItem* menu, int item, bool enabled) {
        if (!menu) return;
        EnableMenuItem(getHandle(menu), item,
                       MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
    }

    void attachMenuBar(System::MenuItem* menuBar,
                       NativeMenu::WindowHandle window,
                       NativeMenu::ActionHandler handler, void* userData) {
        HWND hwnd = getWindow(window);
        if (!hwnd || !menuBar) return;

        WindowHookState* state = getOrCreateHookState(hwnd);
        if (!state) return;

        state->actionHandler = handler;
        state->userData = userData;

        SetMenu(hwnd, getHandle(menuBar));
        DrawMenuBar(hwnd);
    }

    void detachMenuBar(NativeMenu::WindowHandle window) {
        HWND hwnd = getWindow(window);
        if (!hwnd) return;

        SetMenu(hwnd, nullptr);
        DrawMenuBar(hwnd);

        for (auto it = myHooks.begin(); it != myHooks.end(); ++it) {
            WindowHookState* state = it->get();
            if (state->hwnd != hwnd) continue;

            if (state->originalProc) {
                SetWindowLongPtrW(
                    hwnd, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(state->originalProc));
            }
            RemovePropW(hwnd, kNativeMenuHookPropName);
            myHooks.erase(it);
            return;
        }

        RemovePropW(hwnd, kNativeMenuHookPropName);
    }

    void shutdown() {
        for (auto& hook : myHooks) {
            if (!hook->hwnd) continue;
            if (hook->originalProc) {
                SetWindowLongPtrW(
                    hook->hwnd, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(hook->originalProc));
            }
            RemovePropW(hook->hwnd, kNativeMenuHookPropName);
        }
        myHooks.clear();
        myMenus.clear();
    }

   private:
    static HMENU getHandle(System::MenuItem* menu) {
        return menu && menu->myImpl ? menu->myImpl->handle : nullptr;
    }

    static HWND getWindow(NativeMenu::WindowHandle window) {
        if (window.kind != NativeMenu::WindowKind::Win32) return nullptr;
        return reinterpret_cast<HWND>(window.value);
    }

    static LRESULT CALLBACK menuWindowProc(HWND hwnd, UINT msg, WPARAM wp,
                                           LPARAM lp) {
        auto* state = reinterpret_cast<WindowHookState*>(
            GetPropW(hwnd, kNativeMenuHookPropName));
        if (state) {
            switch (msg) {
                case WM_MENUCHAR:
                    return MNC_CLOSE << 16;

                case WM_COMMAND:
                    if (state->actionHandler) {
                        state->actionHandler(LOWORD(wp), state->userData);
                    }
                    return 0;
            }

            if (state->originalProc) {
                return CallWindowProcW(state->originalProc, hwnd, msg, wp, lp);
            }
        }

        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    WindowHookState* getOrCreateHookState(HWND hwnd) {
        for (auto& hook : myHooks) {
            if (hook->hwnd == hwnd) return hook.get();
        }

        auto state = std::make_unique<WindowHookState>();
        state->hwnd = hwnd;
        SetPropW(hwnd, kNativeMenuHookPropName, state.get());
        state->originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            hwnd, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&NativeMenuBackend::menuWindowProc)));

        auto* rawState = state.get();
        myHooks.push_back(std::move(state));
        return rawState;
    }

    std::vector<std::unique_ptr<System::MenuItem>> myMenus;
    std::vector<std::unique_ptr<WindowHookState>> myHooks;
};

NativeMenuBackend& getBackend() {
    static NativeMenuBackend backend;
    return backend;
}

}  // anonymous namespace

System::MenuItem* System::MenuItem::create() {
    return getBackend().createMenu(false);
}

void System::MenuItem::addSeperator() { getBackend().addSeparator(this); }

void System::MenuItem::addItem(int item, const std::string& text) {
    getBackend().addItem(this, item, text);
}

void System::MenuItem::addSubmenu(System::MenuItem* submenu,
                                  const std::string& text, bool grayed) {
    getBackend().addSubmenu(this, submenu, text, grayed);
}

void System::MenuItem::replaceSubmenu(int pos, System::MenuItem* submenu,
                                      const std::string& text, bool grayed) {
    getBackend().replaceSubmenu(this, pos, submenu, text, grayed);
}

void System::MenuItem::setChecked(int item, bool checked) {
    getBackend().setChecked(this, item, checked);
}

void System::MenuItem::setEnabled(int item, bool enabled) {
    getBackend().setEnabled(this, item, enabled);
}

namespace NativeMenu {

System::MenuItem* createMenuBar() { return getBackend().createMenu(true); }

void attachMenuBar(System::MenuItem* menuBar, WindowHandle window,
                   ActionHandler handler, void* userData) {
    getBackend().attachMenuBar(menuBar, window, handler, userData);
}

void detachMenuBar(WindowHandle window) { getBackend().detachMenuBar(window); }

void shutdown() { getBackend().shutdown(); }

}  // namespace NativeMenu

}  // namespace Vortex