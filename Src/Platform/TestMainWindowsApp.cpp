#if BX_PLATFORM_EMSCRIPTEN
#include <bgfx/platform.h>
// #if !BX_PLATFORM_WINDOWS
#include <bx/mutex.h>
#include <bx/handlealloc.h>
#include <bx/os.h>
#include <bx/thread.h>
#include <bx/timer.h>
#include <bx/uint32_t.h>
#include <iostream>
#include <bx/allocator.h>

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "WindowHandle.h"
#include "MainApp.h"
#include "../Contanst/AppConst.h"
#include "../Imgui/DearImgui.h"
#include "../Input/MouseState.h"
#include "../Input/InputSystem.h"

namespace Tiga
{
#pragma region Struct
    struct Msg
    {
        Msg()
            : mX(0), mY(0), mWidth(0), mHeight(0), mFlags(0), mFlagsEnabled(false)
        {
        }

        int32_t mX;
        int32_t mY;
        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mFlags;
        std::string mTitle;
        bool mFlagsEnabled;
    };

    enum
    {
        WM_USER_WINDOW_CREATE = WM_USER,
        WM_USER_WINDOW_DESTROY,
        WM_USER_WINDOW_SET_TITLE,
        WM_USER_WINDOW_SET_FLAGS,
        WM_USER_WINDOW_SET_POS,
        WM_USER_WINDOW_SET_SIZE,
        WM_USER_WINDOW_TOGGLE_FRAME,
        WM_USER_WINDOW_MOUSE_LOCK,
    };

    typedef std::vector<WCHAR> WSTRING;
    inline WSTRING UTF8ToUTF16(const char *utf8_str)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
        WSTRING utf16(len);
        MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, utf16.data(), len);
        return utf16;
    }
#pragma endregion Struct



#pragma region Attributes
    Application *gApplication;
    HWND gHwnd[CONFIG_MAX_WINDOWS];
    Input::MouseState gMouseState;
    bx::Mutex gLock;
    bx::HandleAllocT<CONFIG_MAX_WINDOWS> gWindowAlloc;
    bool gIsAppInited;

#pragma endregion Attributes

    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    int32_t Run(Application *app, int argc, const char *const *argv)
    {
        gApplication = app;

#pragma region Windows Class
        HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);

        WNDCLASSEX wndclass;
        wndclass.cbSize = sizeof(WNDCLASSEX);
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = WndProc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = instance;
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wndclass.lpszMenuName = 0;
        wndclass.lpszClassName = "Tiga";
        RegisterClassEx(&wndclass);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int clientWidth = 800;
        int clientHeight = 600;
        RECT windowRect;
        SetRect(&windowRect, (screenWidth / 2) - (clientWidth / 2),
                (screenHeight / 2) - (clientHeight / 2),
                (screenWidth / 2) + (clientWidth / 2),
                (screenHeight / 2) + (clientHeight / 2));

        // WS_THICKFRAME to resize
        DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        AdjustWindowRectEx(&windowRect, style, FALSE, 0);
        HWND hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, "Tiga", "Tiga", style,
                                   windowRect.left, windowRect.top,
                                   windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                                   NULL, NULL, instance, 0);

        HDC hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 32;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, pixelFormat, &pfd);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
#pragma endregion Windows Class

#pragma region bgfx init
        // set platform window data
        bgfx::PlatformData pd;
        bx::memSet(&pd, 0, sizeof(pd));
        pd.nwh = hwnd;
        bgfx::setPlatformData(pd);

        bgfx::renderFrame();

        bgfx::Init init;
        init.type = bgfx::RendererType::Direct3D12;
        init.vendorId = BGFX_PCI_ID_NONE;
        init.resolution.width = clientWidth;
        init.resolution.height = clientHeight;
        init.resolution.reset = BGFX_RESET_VSYNC;
        bgfx::init(init);

        // Set view 0 clear state.
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
#pragma endregion bgfx init

        CreateGUI();
        app->Initialize();

        MSG msg;
        DWORD lastTick = GetTickCount();
        while (true)
        {
            bgfx::renderFrame();
#pragma region Events
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
#pragma endregion Events

#pragma region Update
            DWORD thisTick = GetTickCount();
            float deltaTime = float(thisTick - lastTick) * 0.001f;
            lastTick = thisTick;
            app->Update(deltaTime);
#pragma endregion Update

#pragma region Render
            float aspect = (float)clientWidth / (float)clientHeight;
            app->Render(aspect);
#pragma endregion Render

#pragma region IMGUI Update
            uint8_t button = (gMouseState.mButtons[Input::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) |
                             (gMouseState.mButtons[Input::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) |
                             (gMouseState.mButtons[Input::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0);
            BeginGUIFrame(gMouseState.mX, gMouseState.mY, button, gMouseState.mZ, clientWidth, clientHeight);
            app->OnGUI();
            EndGUIFrame();
#pragma endregion IMGUI Update
        }

        if (gApplication != 0)
        {
            std::cout << "Expected application to be null on exit\n";
            delete gApplication;
        }

        return (int)msg.wParam;
    }

#pragma region Events
    LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
    {
        if (!gIsAppInited)
        {
            return DefWindowProc(hwnd, iMsg, wParam, lParam);
        }

        switch (iMsg)
        {
        case WM_USER_WINDOW_CREATE:
        {
            Msg *msg = (Msg *)wParam;
            HWND hwnd = CreateWindowW(L"Tiga", UTF8ToUTF16(msg->mTitle.c_str()).data(),
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      msg->mX, msg->mY, msg->mWidth, msg->mHeight, NULL, NULL,
                                      (HINSTANCE)GetModuleHandle(NULL), 0);
                                      
        }
        break;
        case WM_CLOSE:
            if (gApplication != 0)
            {
                gApplication->Shutdown();
                ShutdownGUI();
                // Shutdown bgfx.
                bgfx::shutdown();
                gApplication = 0;
                DestroyWindow(hwnd);
            }
            else
            {
                std::cout << "Already shut down!\n";
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }

        return DefWindowProc(hwnd, iMsg, wParam, lParam);
    }

#pragma endregion Events

#pragma region Events Handler
#pragma endregion Events Handler

#pragma region WindowHandle

    WindowHandle CreateWindowHandle(int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags = WINDOW_FLAG_NONE, const char *title = "")
    {
        bx::MutexScope scope(gLock);
        WindowHandle handle = {gWindowAlloc.alloc()};

        if (UINT16_MAX != handle.idx)
        {
            Msg *msg = new Msg;
            msg->mX = x;
            msg->mY = y;
            msg->mWidth = width;
            msg->mHeight = height;
            msg->mTitle = title;
            PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_CREATE, handle.idx, (LPARAM)msg);
        }

        return handle;
    }

    void DestroyWindow(WindowHandle handle)
    {
        if (UINT16_MAX != handle.idx)
        {
            PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_DESTROY, handle.idx, 0);

            bx::MutexScope scope(gLock);
            gWindowAlloc.free(handle.idx);
        }
    }

    void SetWindowPos(WindowHandle handle, int32_t x, int32_t y)
    {
        Msg *msg = new Msg;
        msg->mX = x;
        msg->mY = y;
        PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_SET_POS, handle.idx, (LPARAM)msg);
    }

    void SetWindowSize(WindowHandle handle, uint32_t width, uint32_t height)
    {
        PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_SET_SIZE, handle.idx, (height << 16) || (width & 0xffff));
    }

    void SetWindowFlags(WindowHandle handle, uint32_t flags, bool enabled)
    {
        Msg *msg = new Msg;
        msg->mFlags = flags;
        msg->mFlagsEnabled = enabled;
        PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_SET_FLAGS, handle.idx, (LPARAM)msg);
    }

    void ToggleFullscreen(WindowHandle handle)
    {
        PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_TOGGLE_FRAME, handle.idx, 0);
    }
    void SetMouseLock(WindowHandle handle, bool lock)
    {
        PostMessage(gHwnd[kMainWindowIndex], WM_USER_WINDOW_MOUSE_LOCK, handle.idx, lock);
    }

#pragma endregion WindowHandle
} // namespace Tiga

#endif