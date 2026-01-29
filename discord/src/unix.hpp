#include "common.hpp"
#include <iostream>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <curl/curl.h>
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using boost::asio::ip::udp;

#include "json.hpp"
using json = nlohmann::json;

#ifdef __linux__
	#include <unistd.h>
#elif __APPLE__
	#include <mach-o/dyld.h>
#endif

time_t adjustEpochToUtc(time_t localEpoch) {
	long timezone_offset = timezone;
	return localEpoch + timezone_offset;
}

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size *nmemb);
    return size *nmemb;
}

std::string fetchRawHtml(std::string server, std::string path) {
	CURL* curl = curl_easy_init();
	if (!curl) return "CURL init failed";

	std::string content;
	std::string url = "https://" + server + path;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "DiscordWiiU/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		return std::string("CURL error: ") + curl_easy_strerror(res);
	}
	return content;
}

json getImageKeys(std::string repo) {
    json images;

    std::string fetch = fetchRawHtml("raw.githubusercontent.com", "/" + repo + "/main/titles.json");
	try {
		images = json::parse(fetch);
		fmt::println("Successfully fetched titles.json!");
	} catch (...) {
		fmt::println("Error fetching titles.json. Using default image.");
	}

    return images;
}

static void gameLoop(std::string repo) {
	std::string msg;

	boost::asio::io_context io_context;

	// Create UDP socket
	// Bind to all interfaces on the specified port
	udp::socket socket = udp::socket(io_context);
	udp::endpoint sender_endpoint;
	char data[1024];

	bool binded = false;
	while (!binded) {
		try {
			socket = udp::socket(io_context, udp::endpoint(udp::v4(), UDP_PORT));
			binded = true;
		}
		catch (...) {
			fmt::println("Failed to bind to UDP port 5005. Is another program using the port? Retrying...");
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
	fmt::println("Successfully binded to UDP port 5005");

    auto& rpc = discord::RPCManager::get();

    json out;
    json images = getImageKeys(repo);
    std::string image;
    char buffer[1024];

    do {
		size_t length = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
		msg = std::string(data, length);

        try {
            out = json::parse(msg);
            if (out["sender"] == "Wii U") {
                
                fmt::println("Received: {}", msg);

                idle = false;
            }

            try {
                image = images[out["long"]];
            } catch (...) {
                image = "oh no it didn't work";
            }

            if (out.contains("img")) { // Update 2.0
                updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"]));
            }
            else { // Update 1.9
                updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, "backwards", adjustEpochToUtc(out["time"]));
            }
        }
        catch (...) {}
    } while (true);

    return;
}
