#ifndef NDEBUG
#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#include <System/System.h>
#include <System/Debug.h>
#include <System/File.h>
#include <System/NativeMenu.h>
#ifdef _WIN32
#include <System/OpenGL.h>
#endif

#include <Core/StringUtils.h>
#include <Core/Shader.h>
#include <Core/Input.h>

#include <Editor/Editor.h>
#include <Editor/Menubar.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <chrono>
#include <thread>
#include <bitset>
#include <vector>
#include <ctime>
#include <algorithm>
#include <list>
#include <numeric>
#ifdef _WIN32
#include <windef.h>
#include <winuser.h>
#include <windows.h>
#endif
#undef ERROR

#ifdef _WIN32
// Enable visual styles.
#pragma comment(linker, \
                "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace Vortex {

std::chrono::duration<double> deltaTime;  // Defined in <Core/Core.h>

namespace {

static std::string sRunDir;
static std::string sExeDir;

// ================================================================================================
// Helper functions

static int GetKeyflags(const std::bitset<Key::MAX_VALUE>& keyState) {
    int flags = 0;
    if (keyState[Key::SHIFT_L] || keyState[Key::SHIFT_R])
        flags |= Keyflag::SHIFT;
    if (keyState[Key::CTRL_L] || keyState[Key::CTRL_R]) flags |= Keyflag::CTRL;
    if (keyState[Key::ALT_L] || keyState[Key::ALT_R]) flags |= Keyflag::ALT;
    return flags;
}

// Holds the result of an SDL3 file dialog callback.
struct FileDialogResult {
    fs::path path;
    int filterIndex = -1;
    SDL_Semaphore* semaphore = nullptr;
};

// Callback invoked by SDL3 when the user completes a file dialog.
static void SDLCALL FileDialogCallback(void* userdata,
                                       const char* const* filelist,
                                       int filter) {
    auto* result = static_cast<FileDialogResult*>(userdata);
    if (filelist && *filelist) {
        result->path = fs::u8path(*filelist);
    }
    result->filterIndex = filter;
    SDL_SignalSemaphore(result->semaphore);
}

// Shows a native SDL3 open/save file dialog. Blocks until the user responds.
static fs::path ShowFileDialog(const std::string& title, const fs::path& path,
                               const std::vector<FileFilter>& filters,
                               int* index, bool save,
                               SDL_Window* sdlWindow = nullptr) {
    // Build an array of SDL_DialogFileFilter from our FileFilter structs.
    std::vector<SDL_DialogFileFilter> sdlFilters;
    sdlFilters.reserve(filters.size());
    for (const auto& f : filters) {
        sdlFilters.push_back({f.name.c_str(), f.pattern.c_str()});
    }

    // Determine the default location from the input path.
    std::string defaultLocation;
    if (!path.empty()) {
        defaultLocation = path.string();
    }

    // Set up a semaphore so we can block until the callback fires.
    FileDialogResult result;
    result.semaphore = SDL_CreateSemaphore(0);

    if (save) {
        SDL_ShowSaveFileDialog(
            FileDialogCallback, &result, sdlWindow,
            sdlFilters.empty() ? nullptr : sdlFilters.data(),
            static_cast<int>(sdlFilters.size()),
            defaultLocation.empty() ? nullptr : defaultLocation.c_str());
    } else {
        SDL_ShowOpenFileDialog(
            FileDialogCallback, &result, sdlWindow,
            sdlFilters.empty() ? nullptr : sdlFilters.data(),
            static_cast<int>(sdlFilters.size()),
            defaultLocation.empty() ? nullptr : defaultLocation.c_str(), false);
    }

    // Pump events while waiting so the dialog can dispatch its callback on the
    // main thread. A hard SDL_WaitSemaphore would deadlock here because the
    // callback is delivered through the SDL event loop on Windows.
    while (!SDL_WaitSemaphoreTimeout(result.semaphore, 0)) {
        SDL_PumpEvents();
        SDL_Delay(1);
    }
    SDL_DestroySemaphore(result.semaphore);

    // SDL3 filter index is 0-based; the interface uses 1-based indices.
    if (index) {
        *index = (result.filterIndex >= 0) ? result.filterIndex + 1 : 0;
    }

    gSystem->setWorkingDir(gSystem->getExeDir());
    return result.path;
}

// ================================================================================================
// Keycode translation table SDL3 -> Vortex

static Key::Code SDLKeycodeToVortex(SDL_Keycode keycode) {
    switch (keycode) {
        // Letters
        case SDLK_A:
            return Key::A;
        case SDLK_B:
            return Key::B;
        case SDLK_C:
            return Key::C;
        case SDLK_D:
            return Key::D;
        case SDLK_E:
            return Key::E;
        case SDLK_F:
            return Key::F;
        case SDLK_G:
            return Key::G;
        case SDLK_H:
            return Key::H;
        case SDLK_I:
            return Key::I;
        case SDLK_J:
            return Key::J;
        case SDLK_K:
            return Key::K;
        case SDLK_L:
            return Key::L;
        case SDLK_M:
            return Key::M;
        case SDLK_N:
            return Key::N;
        case SDLK_O:
            return Key::O;
        case SDLK_P:
            return Key::P;
        case SDLK_Q:
            return Key::Q;
        case SDLK_R:
            return Key::R;
        case SDLK_S:
            return Key::S;
        case SDLK_T:
            return Key::T;
        case SDLK_U:
            return Key::U;
        case SDLK_V:
            return Key::V;
        case SDLK_W:
            return Key::W;
        case SDLK_X:
            return Key::X;
        case SDLK_Y:
            return Key::Y;
        case SDLK_Z:
            return Key::Z;

        // Numbers
        case SDLK_0:
            return Key::DIGIT_0;
        case SDLK_1:
            return Key::DIGIT_1;
        case SDLK_2:
            return Key::DIGIT_2;
        case SDLK_3:
            return Key::DIGIT_3;
        case SDLK_4:
            return Key::DIGIT_4;
        case SDLK_5:
            return Key::DIGIT_5;
        case SDLK_6:
            return Key::DIGIT_6;
        case SDLK_7:
            return Key::DIGIT_7;
        case SDLK_8:
            return Key::DIGIT_8;
        case SDLK_9:
            return Key::DIGIT_9;

        // Symbols
        case SDLK_GRAVE:
            return Key::ACCENT;
        case SDLK_MINUS:
            return Key::DASH;
        case SDLK_EQUALS:
            return Key::EQUAL;
        case SDLK_LEFTBRACKET:
            return Key::BRACKET_L;
        case SDLK_RIGHTBRACKET:
            return Key::BRACKET_R;
        case SDLK_SEMICOLON:
            return Key::SEMICOLON;
        case SDLK_APOSTROPHE:
            return Key::QUOTE;
        case SDLK_BACKSLASH:
            return Key::BACKSLASH;
        case SDLK_COMMA:
            return Key::COMMA;
        case SDLK_PERIOD:
            return Key::PERIOD;
        case SDLK_SLASH:
            return Key::SLASH;

        // Control keys
        case SDLK_ESCAPE:
            return Key::ESCAPE;
        case SDLK_SPACE:
            return Key::SPACE;
        case SDLK_TAB:
            return Key::TAB;
        case SDLK_CAPSLOCK:
            return Key::CAPS;
        case SDLK_RETURN:
            return Key::RETURN;
        case SDLK_BACKSPACE:
            return Key::BACKSPACE;
        case SDLK_PAGEUP:
            return Key::PAGE_UP;
        case SDLK_PAGEDOWN:
            return Key::PAGE_DOWN;
        case SDLK_HOME:
            return Key::HOME;
        case SDLK_END:
            return Key::END;
        case SDLK_INSERT:
            return Key::INSERT;
        // case SDLK_DELETE:
        //     return Key::DELETE;
        case SDLK_PRINTSCREEN:
            return Key::PRINT_SCREEN;
        case SDLK_SCROLLLOCK:
            return Key::SCROLL_LOCK;
        case SDLK_PAUSE:
            return Key::PAUSE;

        // Arrow keys
        case SDLK_LEFT:
            return Key::LEFT;
        case SDLK_RIGHT:
            return Key::RIGHT;
        case SDLK_UP:
            return Key::UP;
        case SDLK_DOWN:
            return Key::DOWN;

        // Numpad
        case SDLK_NUMLOCKCLEAR:
            return Key::NUM_LOCK;
        case SDLK_KP_DIVIDE:
            return Key::NUMPAD_DIVIDE;
        case SDLK_KP_MULTIPLY:
            return Key::NUMPAD_MULTIPLY;
        case SDLK_KP_MINUS:
            return Key::NUMPAD_SUBTRACT;
        case SDLK_KP_PLUS:
            return Key::NUMPAD_ADD;
        case SDLK_KP_PERIOD:
            return Key::NUMPAD_SEPERATOR;
        case SDLK_KP_0:
            return Key::NUMPAD_0;
        case SDLK_KP_1:
            return Key::NUMPAD_1;
        case SDLK_KP_2:
            return Key::NUMPAD_2;
        case SDLK_KP_3:
            return Key::NUMPAD_3;
        case SDLK_KP_4:
            return Key::NUMPAD_4;
        case SDLK_KP_5:
            return Key::NUMPAD_5;
        case SDLK_KP_6:
            return Key::NUMPAD_6;
        case SDLK_KP_7:
            return Key::NUMPAD_7;
        case SDLK_KP_8:
            return Key::NUMPAD_8;
        case SDLK_KP_9:
            return Key::NUMPAD_9;

        // Modifiers
        case SDLK_LCTRL:
            return Key::CTRL_L;
        case SDLK_RCTRL:
            return Key::CTRL_R;
        case SDLK_LALT:
            return Key::ALT_L;
        case SDLK_RALT:
            return Key::ALT_R;
        case SDLK_LSHIFT:
            return Key::SHIFT_L;
        case SDLK_RSHIFT:
            return Key::SHIFT_R;
        case SDLK_LGUI:
            return Key::SYSTEM_L;
        case SDLK_RGUI:
            return Key::SYSTEM_R;

        // Function keys
        case SDLK_F1:
            return Key::F1;
        case SDLK_F2:
            return Key::F2;
        case SDLK_F3:
            return Key::F3;
        case SDLK_F4:
            return Key::F4;
        case SDLK_F5:
            return Key::F5;
        case SDLK_F6:
            return Key::F6;
        case SDLK_F7:
            return Key::F7;
        case SDLK_F8:
            return Key::F8;
        case SDLK_F9:
            return Key::F9;
        case SDLK_F10:
            return Key::F10;
        case SDLK_F11:
            return Key::F11;
        case SDLK_F12:
            return Key::F12;
        case SDLK_F13:
            return Key::F13;
        case SDLK_F14:
            return Key::F14;
        case SDLK_F15:
            return Key::F15;

        default:
            return Key::NONE;
    }
}

// ================================================================================================
// Cursor mapping SDL3

static SDL_SystemCursor CursorToSDL(Cursor::Icon c) {
    switch (c) {
        case Cursor::ARROW:
            return SDL_SYSTEM_CURSOR_DEFAULT;
        case Cursor::HAND:
            return SDL_SYSTEM_CURSOR_POINTER;
        case Cursor::IBEAM:
            return SDL_SYSTEM_CURSOR_TEXT;
        case Cursor::SIZE_ALL:
            return SDL_SYSTEM_CURSOR_MOVE;
        case Cursor::SIZE_WE:
            return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case Cursor::SIZE_NS:
            return SDL_SYSTEM_CURSOR_NS_RESIZE;
        case Cursor::SIZE_NESW:
            return SDL_SYSTEM_CURSOR_NESW_RESIZE;
        case Cursor::SIZE_NWSE:
            return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
        default:
            return SDL_SYSTEM_CURSOR_DEFAULT;
    }
}

// ================================================================================================
// SystemImpl :: Debug logging.

static bool LogCheckpoint(bool result, const char* description) {
    if (result) {
        Debug::log("%s :: OK\n", description);
    } else {
        const char* err = SDL_GetError();
        Debug::blockBegin(Debug::ERROR, description);
        Debug::log("SDL error: %s\n", (err && *err) ? err : "(unknown)");
        Debug::blockEnd();
    }
    return !result;
}

#ifdef _WIN32
static void DispatchNativeMenuAction(int actionId, void*) {
    if (gEditor) {
        gEditor->onMenuAction(actionId);
    }
}
#endif

// ================================================================================================
// SystemImpl :: member data.

struct SystemImpl : public System {
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    std::chrono::steady_clock::time_point applicationStartTime;
    Cursor::Icon currentCursor = Cursor::ARROW;
    Cursor::Icon prevCursor = Cursor::ARROW;
    SDL_Cursor* sdlCursors[Cursor::NUM_CURSORS] = {};
    InputEvents myEvents;
    vec2i mousePos = {0, 0};
    vec2i windowSize = {1200, 900};
    std::bitset<Key::MAX_VALUE> keyState;
    std::bitset<Mouse::MAX_VALUE> mouseState;
    std::string windowTitle = "ArrowVortex";
#ifdef _WIN32
    HWND myHWND = nullptr;
#endif
    bool isWindowActive = false;
    bool isTerminated = false;
    bool vsyncEnabled = true;
    int argc = 0;
    char** argv = nullptr;

    // ================================================================================================
    // SystemImpl :: constructor and destructor.

    ~SystemImpl() {
#ifdef _WIN32
        NativeMenu::detachMenuBar({NativeMenu::WindowKind::Win32, myHWND});
        NativeMenu::shutdown();
#endif

        // Destroy the cursors
        for (auto& cursor : sdlCursors) {
            if (cursor) SDL_DestroyCursor(cursor);
        }

        // Destroy the rendering context
        if (glContext) {
            SDL_GL_DestroyContext(glContext);
        }

        // Destroy the window
        if (window) {
            SDL_DestroyWindow(window);
        }

        SDL_Quit();
    }

    SystemImpl() {
        applicationStartTime = Debug::getElapsedTime();

        // Initialize SDL3
        if (LogCheckpoint(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
                          "initializing SDL3"))
            return;

        // Set OpenGL attributes before creating window/context
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Create window with OpenGL context
        window =
            SDL_CreateWindow(windowTitle.c_str(), windowSize.x, windowSize.y,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (LogCheckpoint(window != nullptr, "creating window")) {
            SDL_Quit();
            return;
        }

#ifdef _WIN32
        SDL_PropertiesID props = SDL_GetWindowProperties(window);
        void* hwnd = SDL_GetPointerProperty(
            props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        myHWND = reinterpret_cast<HWND>(hwnd);
#endif

        // Create OpenGL context
        glContext = SDL_GL_CreateContext(window);
        if (LogCheckpoint(glContext != nullptr, "creating OpenGL context")) {
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            return;
        }

        if (LogCheckpoint(SDL_GL_MakeCurrent(window, glContext),
                          "activating OpenGL context")) {
            SDL_GL_DestroyContext(glContext);
            glContext = nullptr;
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            return;
        }

        VortexCheckGlError();

        // Initialize OpenGL settings
        glClearColor(0, 0, 0, 1);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnableClientState(GL_VERTEX_ARRAY);

        VortexCheckGlError();

        // Enable vsync for now, we will disable it later if the settings
        // require it.
        SDL_GL_SetSwapInterval(1);
        VortexCheckGlError();

        // Check shader support
        Shader::initExtension();
        Debug::logBlankLine();

        // Center window on screen
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);

        // Explicitly show and raise the window
        SDL_ShowWindow(window);
        SDL_RaiseWindow(window);

        // Pump events
        SDL_PumpEvents();

        // Create system cursors
        for (int i = 0; i < Cursor::NUM_CURSORS; ++i) {
            sdlCursors[i] = SDL_CreateSystemCursor(
                CursorToSDL(static_cast<Cursor::Icon>(i)));
        }

        isWindowActive = true;
        Debug::log("SDL3 System initialized successfully\n");
    }

    // ================================================================================================
    // SystemImpl :: message loop.

    void forwardArgs() {
        if (!argc || !argv || !gEditor) return;
        Vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(std::string(argv[i]));
        }
        if (args.size() > 0) {
            gEditor->onCommandLineArgs(args.data(), args.size());
        }
    }

    void createMenu() {
#ifdef _WIN32
        auto* menu = NativeMenu::createMenuBar();
        gMenubar->init(menu);
        NativeMenu::attachMenuBar(menu, {NativeMenu::WindowKind::Win32, myHWND},
                                  &DispatchNativeMenuAction, nullptr);
#endif
    }

    void messageLoop() {
        using namespace std::chrono;

        if (!window) {
            Debug::log("CRITICAL: Window is null, cannot enter message loop\n");
            return;
        }

#ifndef NDEBUG
        long long frames = 0;
        auto lowcounts = 0;
        std::list<double> fpsList, sleepList, frameList, inputList, waitList;
        // Adjust frameGuess to your VSync target if you are testing with VSync
        // enabled
        auto frameGuess = 960;
#endif

        Debug::log("Creating editor...\n");
        Editor::create();
        Debug::log("Editor created successfully\n");
        forwardArgs();
        createMenu();
        Debug::log("Entering main loop\n");

        // Frame timing
        auto frameTarget = duration<double>(1.0 / 960.0);
        auto prevTime = Debug::getElapsedTime();
        bool initialRaiseDone = false;

        // Main message loop
        SDL_Event event;

        while (!isTerminated) {
            auto startTime = Debug::getElapsedTime();

            myEvents.clear();

            // Process SDL events
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT:
                        if (gEditor) {
                            gEditor->onExitProgram();
                        }
                        break;

                    case SDL_EVENT_WINDOW_RESIZED:
                        windowSize.x = event.window.data1;
                        windowSize.y = event.window.data2;
                        break;

                    case SDL_EVENT_WINDOW_FOCUS_GAINED:
                        isWindowActive = true;
                        break;

                    case SDL_EVENT_WINDOW_FOCUS_LOST:
                        isWindowActive = false;
                        myEvents.addWindowInactive();
                        keyState.reset();
                        mouseState.reset();
                        break;

                    case SDL_EVENT_MOUSE_MOTION:
                        mousePos.x = static_cast<int>(event.motion.x);
                        mousePos.y = static_cast<int>(event.motion.y);
                        myEvents.addMouseMove(mousePos.x, mousePos.y);
                        break;

                    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                        int x = static_cast<int>(event.button.x);
                        int y = static_cast<int>(event.button.y);
                        Mouse::Code button = Mouse::NONE;

                        switch (event.button.button) {
                            case SDL_BUTTON_LEFT:
                                button = Mouse::LMB;
                                break;
                            case SDL_BUTTON_MIDDLE:
                                button = Mouse::MMB;
                                break;
                            case SDL_BUTTON_RIGHT:
                                button = Mouse::RMB;
                                break;
                            default:
                                break;
                        }

                        if (button != Mouse::NONE) {
                            bool doubleClick = event.button.clicks > 1;
                            myEvents.addMousePress(button, x, y,
                                                   GetKeyflags(keyState),
                                                   doubleClick);
                            mouseState.set(button);
                        }
                        break;
                    }

                    case SDL_EVENT_MOUSE_BUTTON_UP: {
                        int x = static_cast<int>(event.button.x);
                        int y = static_cast<int>(event.button.y);
                        Mouse::Code button = Mouse::NONE;

                        switch (event.button.button) {
                            case SDL_BUTTON_LEFT:
                                button = Mouse::LMB;
                                break;
                            case SDL_BUTTON_MIDDLE:
                                button = Mouse::MMB;
                                break;
                            case SDL_BUTTON_RIGHT:
                                button = Mouse::RMB;
                                break;
                            default:
                                break;
                        }

                        if (button != Mouse::NONE) {
                            myEvents.addMouseRelease(button, x, y,
                                                     GetKeyflags(keyState));
                            mouseState.reset(button);
                        }
                        break;
                    }

                    case SDL_EVENT_MOUSE_WHEEL: {
                        float mx, my;
                        SDL_GetMouseState(&mx, &my);
                        bool scrollUp = event.wheel.y > 0;
                        myEvents.addMouseScroll(scrollUp, static_cast<int>(mx),
                                                static_cast<int>(my),
                                                GetKeyflags(keyState));
                        break;
                    }

                    case SDL_EVENT_KEY_DOWN: {
                        Key::Code keyCode = SDLKeycodeToVortex(event.key.key);
                        if (keyCode != Key::NONE) {
                            bool repeated = event.key.repeat;
                            myEvents.addKeyPress(keyCode, GetKeyflags(keyState),
                                                 repeated);
                            keyState.set(keyCode);
                        }
                        break;
                    }

                    case SDL_EVENT_KEY_UP: {
                        Key::Code keyCode = SDLKeycodeToVortex(event.key.key);
                        if (keyCode != Key::NONE) {
                            myEvents.addKeyRelease(keyCode,
                                                   GetKeyflags(keyState));
                            keyState.reset(keyCode);
                        }
                        break;
                    }

                    case SDL_EVENT_TEXT_INPUT:
                        myEvents.addTextInput(event.text.text);
                        break;

                    case SDL_EVENT_DROP_FILE: {
                        const char* file = event.drop.data;
                        myEvents.addFileDrop(&file, 1, 0, 0);
                        break;
                    }
                }
            }

            // Render frame setup
            glViewport(0, 0, windowSize.x, windowSize.y);
            glLoadIdentity();
            glOrtho(0, windowSize.x, windowSize.y, 0, -1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Reset cursor
            currentCursor = Cursor::ARROW;

#ifndef NDEBUG
            auto inputTime = Debug::getElapsedTime();

            VortexCheckGlError();
#endif

            // Tick editor
            if (gEditor) {
                gEditor->tick();
            }

            // Update cursor if it changed
            if (currentCursor != prevCursor) {
                if (sdlCursors[currentCursor]) {
                    SDL_SetCursor(sdlCursors[currentCursor]);
                }
                prevCursor = currentCursor;
            }

            // Display
            SDL_GL_SwapWindow(window);

#ifndef NDEBUG
            auto renderTime = Debug::getElapsedTime();
#endif

            // After the first frame is rendered, raise the window again as a
            // fallback to ensure it comes to foreground (especially when
            // launched from a debugger)
            if (!initialRaiseDone) {
                SDL_RaiseWindow(window);
                initialRaiseDone = true;
            }

            // Frame timing
            if (vsyncEnabled) {
                while (Debug::getElapsedTime() - prevTime < frameTarget) {
                    std::this_thread::yield();
                }
            }

            // End of frame
            auto curTime = Debug::getElapsedTime();
            double dt =
                std::chrono::duration<double>(curTime - prevTime).count();
            dt = max(0.0, min(dt, 0.25));
            deltaTime = std::chrono::duration<double>(static_cast<float>(dt));
            prevTime = curTime;

#ifndef NDEBUG
            // Do frame statistics
            // Note that these will be wrong with VSync enabled.
            fpsList.push_front(deltaTime.count());
            waitList.push_front(duration<double>(curTime - renderTime).count());
            frameList.push_front(
                duration<double>(renderTime - inputTime).count());
            inputList.push_front(
                duration<double>(inputTime - startTime).count());

            if (abs(deltaTime.count() - 1.0 / static_cast<double>(frameGuess)) /
                    (1.0 / static_cast<double>(frameGuess)) >
                0.01) {
                lowcounts++;
            }
            if (fpsList.size() >= static_cast<size_t>(frameGuess * 2)) {
                fpsList.pop_back();
                frameList.pop_back();
                inputList.pop_back();
                waitList.pop_back();
            }
            auto fpsMin = *std::min_element(fpsList.begin(), fpsList.end());
            auto fpsMax = *std::max_element(fpsList.begin(), fpsList.end());
            auto maxIndex =
                std::distance(fpsList.begin(),
                              std::max_element(fpsList.begin(), fpsList.end()));
            auto siz = fpsList.size();
            auto avg =
                std::accumulate(fpsList.begin(), fpsList.end(), 0.0) / siz;
            auto varianceFunc = [&avg, &siz](double accumulator, double val) {
                return accumulator + (val - avg) * (val - avg);
            };
            auto stddev = sqrt(std::accumulate(fpsList.begin(), fpsList.end(),
                                               0.0, varianceFunc) /
                               siz);
            auto frameAvg =
                std::accumulate(frameList.begin(), frameList.end(), 0.0) /
                frameList.size();
            auto frameMax = frameList.begin();
            std::advance(frameMax, maxIndex);
            auto inputMax = inputList.begin();
            std::advance(inputMax, maxIndex);
            auto waitMax = waitList.begin();
            std::advance(waitMax, maxIndex);
            if (frames % (frameGuess * 2) == 0) {
                Debug::log(
                    "frame total average: %f, frame render average %f, std dev "
                    "%f, lowest FPS %f, highest FPS %f, highest FPS render "
                    "time %f, highest FPS input time %f, highest FPS wait time "
                    "%f, lag frames %d\n",
                    avg, frameAvg, stddev, 1.0 / fpsMax, 1.0 / fpsMin,
                    *frameMax, *inputMax, *waitMax, lowcounts);
                lowcounts = 0;
            }
            frames++;
#endif
        }

        Editor::destroy();
    }

    // ================================================================================================
    // SystemImpl :: clipboard functions.

    bool setClipboardText(const std::string& text) override {
        return SDL_SetClipboardText(text.c_str());
    }

    std::string getClipboardText() const override {
        const char* text = SDL_GetClipboardText();
        return std::string(text ? text : "");
    }

    // ================================================================================================
    // SystemImpl :: dialog boxes.

    Result showMessageDlg(const std::string& title, const std::string& text,
                          Buttons b, Icon i) override {
        // Map icon types to SDL3 message box flags.
        Uint32 sdlFlags = 0;
        switch (i) {
            case I_INFO:
                sdlFlags = SDL_MESSAGEBOX_INFORMATION;
                break;
            case I_WARNING:
                sdlFlags = SDL_MESSAGEBOX_WARNING;
                break;
            case I_ERROR:
                sdlFlags = SDL_MESSAGEBOX_ERROR;
                break;
            default:
                break;
        }

        // Build button arrays for each dialog type.
        SDL_MessageBoxButtonData btnsOk[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, R_OK, "OK"},
        };
        SDL_MessageBoxButtonData btnsOkCancel[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, R_OK, "OK"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, R_CANCEL, "Cancel"},
        };
        SDL_MessageBoxButtonData btnsYesNo[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, R_YES, "Yes"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, R_NO, "No"},
        };
        SDL_MessageBoxButtonData btnsYesNoCancel[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, R_YES, "Yes"},
            {0, R_NO, "No"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, R_CANCEL, "Cancel"},
        };

        const SDL_MessageBoxButtonData* btnData = btnsOk;
        int numButtons = 1;
        switch (b) {
            case T_OK:
                btnData = btnsOk;
                numButtons = 1;
                break;
            case T_OK_CANCEL:
                btnData = btnsOkCancel;
                numButtons = 2;
                break;
            case T_YES_NO:
                btnData = btnsYesNo;
                numButtons = 2;
                break;
            case T_YES_NO_CANCEL:
                btnData = btnsYesNoCancel;
                numButtons = 3;
                break;
            default:
                break;
        }

        SDL_MessageBoxData msgboxData = {};
        msgboxData.flags = sdlFlags;
        msgboxData.window = window;
        msgboxData.title = title.c_str();
        msgboxData.message = text.c_str();
        msgboxData.numbuttons = numButtons;
        msgboxData.buttons = btnData;

        int buttonId = R_CANCEL;
        SDL_ShowMessageBox(&msgboxData, &buttonId);
        return static_cast<Result>(buttonId);
    }

    fs::path openFileDlg(const std::string& title, fs::path filename,
                         const std::vector<FileFilter>& filters) override {
        return ShowFileDialog(title, filename, filters, nullptr, false, window);
    }

    fs::path saveFileDlg(const std::string& title, fs::path filename,
                         const std::vector<FileFilter>& filters,
                         int* index) override {
        return ShowFileDialog(title, filename, filters, index, true, window);
    }

    // ================================================================================================
    // SystemImpl :: misc/get/set functions.

    bool runSystemCommand(const std::string& cmd) override {
        return runSystemCommand(cmd, nullptr, nullptr);
    }

    bool runSystemCommand(const std::string& cmd, CommandPipe* pipe,
                          void* buffer) override {
        bool needPipe = (pipe != nullptr);

        // Launch the command through the platform shell.
#ifdef _WIN32
        const char* args[] = {"cmd.exe", "/c", cmd.c_str(), nullptr};
#else
        const char* args[] = {"sh", "-c", cmd.c_str(), nullptr};
#endif

        SDL_Process* process = SDL_CreateProcess(args, needPipe);
        if (!process) return false;

        if (needPipe) {
            SDL_IOStream* stdinStream = SDL_GetProcessInput(process);
            if (stdinStream) {
                int bytesRead = pipe->read();
                while (bytesRead > 0) {
                    SDL_WriteIO(stdinStream, buffer, bytesRead);
                    bytesRead = pipe->read();
                }
                // Close stdin to signal EOF to the child process.
                SDL_CloseIO(stdinStream);
            }
        }

        SDL_WaitProcess(process, true, nullptr);
        SDL_DestroyProcess(process);
        return true;
    }

    void openWebpage(const std::string& link) override {
        SDL_OpenURL(link.c_str());
    }

    void setWorkingDir(const std::string& path) override {
        std::error_code ec;
        fs::current_path(path, ec);
    }

    void setCursor(Cursor::Icon c) override { currentCursor = c; }

    void disableVsync() override {
        vsyncEnabled = false;
        SDL_GL_SetSwapInterval(0);
    }

    double getElapsedTime() const override {
        return Debug::getElapsedTime(applicationStartTime);
    }

    void* getHWND() const override {
#ifdef _WIN32
        return myHWND;
#else
        return nullptr;
#endif
    }

    std::string getExeDir() const override { return sExeDir; }

    std::string getRunDir() const override { return sRunDir; }

    Cursor::Icon getCursor() const override { return currentCursor; }

    bool isKeyDown(Key::Code key) const override {
        if (key < Key::MAX_VALUE) {
            return keyState.test(key);
        }
        return false;
    }

    bool isMouseDown(Mouse::Code button) const override {
        if (button < Mouse::MAX_VALUE) {
            return mouseState.test(button);
        }
        return false;
    }

    vec2i getMousePos() const override { return mousePos; }

    int getKeyFlags() const override { return GetKeyflags(keyState); }

    void setWindowTitle(const std::string& text) override {
        windowTitle = text;
        if (window) {
            SDL_SetWindowTitle(window, text.c_str());
        }
    }

    vec2i getWindowSize() const override { return windowSize; }

    const std::string& getWindowTitle() const override { return windowTitle; }

    InputEvents& getEvents() override { return myEvents; }

    bool isActive() const override { return isWindowActive; }

    void terminate() override { isTerminated = true; }
};  // SystemImpl.
};  // anonymous namespace

System* gSystem = nullptr;

};  // namespace Vortex
using namespace Vortex;

std::string System::getLocalTime() {
    time_t now = time(nullptr);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

std::string System::getBuildData() {
    std::string date(__DATE__);
    if (date[4] == ' ') date.begin()[4] = '0';
    return date;
}

static void ApplicationStart() {
    // Save the initial working directory.
    char* cwd = SDL_GetCurrentDirectory();
    if (cwd) {
        sRunDir = cwd;
        SDL_free(cwd);
    }

    // Get the executable's directory.
    const char* basePath = SDL_GetBasePath();
    if (basePath) {
        sExeDir = basePath;
    }

    // Set the working directory to the executable's directory.
    if (!sExeDir.empty()) {
        std::error_code ec;
        fs::current_path(sExeDir, ec);
    }

    Debug::openLogFile();
    Debug::log("Starting ArrowVortex :: %s\n", System::getLocalTime().c_str());
    Debug::log("Build: %s\n", System::getBuildData().c_str());
    Debug::logBlankLine();
}

static void ApplicationEnd() {
    Debug::logBlankLine();
    Debug::log("Closing ArrowVortex :: %s", System::getLocalTime().c_str());
}

int main(int argc, char* argv[]) {
    using namespace Vortex;

    ApplicationStart();
#ifndef NDEBUG
    Debug::openConsole();
#endif

    auto* impl = new SystemImpl;
    gSystem = impl;
    impl->argc = argc;
    impl->argv = argv;
    impl->messageLoop();
    delete impl;
    gSystem = nullptr;

    ApplicationEnd();

#ifdef CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
