constexpr double VERSION = 2.1;

#include <thread>

#if defined(__linux__) || defined(__APPLE__)
	#include "unix.hpp"
#elif _WIN32
    #include "win.hpp"
#endif

std::string repo = "flamingnineteen/richpresencewups-db";
std::thread tthread(checkIdle);

int main(int argc, char* argv[]) {
    // Check for command line arguments
    int i = 2;
    while (i < argc) {
        if (std::strcmp(argv[i - 1], "repo") == 0) {
            repo = argv[i];
            fmt::println("Using repository {}.", repo);
        }
        else if (std::strcmp(argv[i - 1], "port") == 0) {
            UDP_PORT = std::stoi(argv[i]);
            fmt::println("Using port {}.", UDP_PORT);
        }
        i+=2;
    }

    std::string ver = std::to_string(VERSION + 0.1);
    unsigned short update = getStatusCode("github.com", "/FlamingNineteen/RichPresenceWUPS/releases/tag/v" + ver.replace(ver.find(".") + 2, ver.length() - ver.find(".") - 2, ""));
    if (update >= 200 && update < 300) {
        fmt::println("A new update is available");
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
