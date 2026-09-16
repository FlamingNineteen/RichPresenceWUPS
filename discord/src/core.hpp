#include <thread>

#if defined(__linux__) || defined(__APPLE__)
	#include "unix/unix.hpp"
#elif _WIN32
    #include "win/win.hpp"
#endif

struct Config {
    // The image repository. Do not include https:// or http:// at the beginning of string.
    std::string repo = "raw.githubusercontent.com/flamingnineteen/richpresencewups-db/main";
    
    // The application ID of the Discord app to connect to.
    std::string app_id = "1353248127469228074";

    // The port to bind to.
    uint16_t port = 5005;

    // Whether to show logs on Windows or not
    bool winlogs = false;
};

Config cmdLineArgs(int argc, char* argv[]) {
    Config config = Config();

    // Check for command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--windows-logs") == 0 || std::strcmp(argv[i], "-w") == 0) {
            config.winlogs = true;
            fmt::println("Enabling Windows logging");
        }
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            fmt::println("Wii U Rich Presence v{}", VERSION);
        }
        else if (i + 1 < argc) {
            if (std::strcmp(argv[i], "--repo") == 0 || std::strcmp(argv[i], "-r") == 0) {
                config.repo = argv[i+1];
                fmt::println("Using repository {}", config.repo);
            }
            else if (
                std::strcmp(argv[i], "--app-id") == 0 || std::strcmp(argv[i], "-a") == 0) {
                config.app_id = argv[i+1];
                fmt::println("Using application id {}", config.app_id);
            }
            else if (std::strcmp(argv[i], "--port") == 0 || std::strcmp(argv[i], "-p") == 0) {
                config.port = std::stoi(argv[i+1]);
                fmt::println("Using port {}", config.port);
            }
            i++;
        }
    }

    return config;
}

void coreLogic(Config config) {
    std::thread tthread(checkIdle);

    discordSetup(config.app_id);
    discord::RPCManager::get().initialize();

    gameLoop(config.repo, config.port);

    runIdleLoop = false;
	if (tthread.joinable()) {
		tthread.join();
	}

    discord::RPCManager::get().shutdown();
    return;
}
