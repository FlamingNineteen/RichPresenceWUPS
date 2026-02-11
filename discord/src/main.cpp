#include <thread>

#if defined(__linux__) || defined(__APPLE__)
	#include "unix.hpp"
#elif _WIN32
    #include "win.hpp"
#endif

std::string repo = "flamingnineteen/richpresencewups-db";
std::thread tthread(checkIdle);

int main(int argc, char* argv[]) {
    int i = 2;
    while (i < argc) {
        // More commands can be added here in the future
        if (std::strcmp(argv[i - 1], "repo") == 0) {
            repo = argv[i];
            fmt::println("Using repository {}.", repo);
        }
        i++;
    }
    discordSetup();
    discord::RPCManager::get().initialize();

    gameLoop(repo);

    runIdleLoop = false;
	if (tthread.joinable()) {
		tthread.join();
	}

    discord::RPCManager::get().shutdown();
    return 0;
}
