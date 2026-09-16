#include "core.hpp"

#if _WIN32
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        Config config = cmdLineArgs(__argc, __argv);

        if (config.winlogs) SetConsole();

        coreLogic(config);
        return 0;
    }
#else
    int main(int argc, char* argv[]) {
        coreLogic(cmdLineArgs(argc, argv));
        return 0;
    }
#endif
