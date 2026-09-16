#include <thread>

#if defined(__linux__) || defined(__APPLE__)
	#include "unix/unix.hpp"
#elif _WIN32
    #include "win/win.hpp"
#endif

void coreLogic(int argc, char* argv[]) {
    std::thread tthread(checkIdle);

    struct {
        // The image repository. Do not include https:// or http:// at the beginning of string.
        std::string repo = "raw.githubusercontent.com/flamingnineteen/richpresencewups-db/main";
        
        // The application ID of the Discord app to connect to.
        std::string app_id = "1353248127469228074";

        // The port to bind to.
        uint16_t port = 5005;
    } config;

    // Check for command line arguments
    int i = 1;
    while (i < argc) {
        if (i + 1 < argc) {
            if (std::strcmp(argv[i], "--repo") == 0 || std::strcmp(argv[i], "-r") == 0) {
                config.repo = argv[i+1];
                fmt::println("Using repository {}", config.repo);
            }
            else if (
                std::strcmp(argv[i], "--app-id") == 0 || std::strcmp(argv[i], "-a") == 0) {
                config.port = std::stoi(argv[i+1]);
                fmt::println("Using application id {}", config.app_id);
            }
            else if (std::strcmp(argv[i], "--port") == 0 || std::strcmp(argv[i], "-p") == 0) {
                config.port = std::stoi(argv[i+1]);
                fmt::println("Using port {}", config.port);
            }
            i+=1;
        }
        i+=1;
    }
    
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
