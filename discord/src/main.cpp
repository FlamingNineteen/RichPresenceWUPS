#include "core.hpp"

#if _WIN32
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        Config config = cmdLineArgs(__argc, __argv);

        if (config.winlogs) SetConsole();

        const wchar_t* CLASS_NAME = L"WURPTrayClass";
        WNDCLASSW wc              = {};
        wc.lpfnWndProc            = WindowProc;
        wc.hInstance              = hInstance;
        wc.lpszClassName          = CLASS_NAME;
        RegisterClassW(&wc);

        HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"WURP Tray", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

        nid.cbSize           = sizeof(NOTIFYICONDATAW);
        nid.hWnd             = hwnd;
        nid.uID              = 1;
        nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
        wcscpy_s(nid.szTip, L"Wii U Rich Presence");
        Shell_NotifyIconW(NIM_ADD, &nid);

        std::thread worker([config]() {
            coreLogic(config);
            PostThreadMessageW(GetCurrentThreadId(), WM_QUIT, 0, 0);
        });

        MSG msg = {};
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (worker.joinable()) worker.join();

        return 0;
    }
#else
    int main(int argc, char* argv[]) {
        coreLogic(cmdLineArgs(argc, argv));
        return 0;
    }
#endif
