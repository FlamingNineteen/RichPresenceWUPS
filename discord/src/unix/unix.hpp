#include <curl/curl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../common.hpp"

#ifdef __linux__
	#include <unistd.h>
#elif __APPLE__
	#include <mach-o/dyld.h>
#endif

// Change the recieved time elapsed to epoch
time_t adjustEpochToUtc(time_t localEpoch, bool dst = false) {
	if (dst) {
		struct tm tm;
		localtime_r(&localEpoch, &tm);
		return localEpoch - tm.tm_gmtoff;
	} else {
		long timezone_offset = timezone;
		return localEpoch + timezone_offset;
	}
}

// CURL callback
size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size *nmemb);
    return size *nmemb;
}

// Fetches data with html
std::string fetchRawHtml(std::string server, std::string path) {
	CURL* curl = curl_easy_init();
	if (!curl) return "CURL init failed";

	std::string content;
	std::string url = "https://" + server + path;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "WiiURichPresence/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		return std::string("CURL error: ") + curl_easy_strerror(res);
	}
	return content;
}

// Fetch the image keys from the repository
json getImageKeys(std::string repo) {
    json images;

    std::string fetch = fetchRawHtml(repo + "/titles.json");
	try {
		images = json::parse(fetch);
		fmt::println("Successfully fetched titles.json!");
	} catch (...) {
		fmt::println("Error fetching titles.json. Using default image.");
	}

    return images;
}

// Main loop
void gameLoop(std::string repo, uint16_t port) {
	std::string msg;
	int sock;
	struct sockaddr_in addr;

	// Create UDP socket
	do {	
		sock = socket(AF_INET, SOCK_DGRAM, 0);
	} while (sock < 0);

	// Bind to all interfaces on the specified port
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    addr.sin_port = htons(port);      // Bind to port 5005

	while (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
		fmt::println("Failed to bind to UDP port. Is another program using it? Retrying...");
		std::this_thread::sleep_for(std::chrono::seconds(2));
    }

	fmt::println("Successfully binded to UDP port");

    auto& rpc = discord::RPCManager::get();

    json images = getImageKeys(repo);
    char buffer[1024];

    do {
		// Wait for a message
		socklen_t addr_len = sizeof(addr);
    	ssize_t bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_len);
    
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			msg = buffer;
			
			// Attempt to set Rich Presence
			if (parseJsonAndUpdate(msg, images, repo, adjustEpochToUtc) < 0) {
				fmt::println("Failed to update Rich Presence");
			}
		}
		else {
			fmt::println("Recieved empty message");
		}
    } while (true);

	close(sock);

    return;
}
