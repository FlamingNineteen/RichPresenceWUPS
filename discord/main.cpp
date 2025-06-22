#define DISCORDPP_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <string>
#include <csignal>
#include <fstream>
#include <future>
#include "discordpp.h"
#include "json.hpp"
using json = nlohmann::json;

// Replace with your Discord Application ID
const uint64_t APPLICATION_ID = 1353248127469228074;

// Create a flag to stop the application
std::atomic<bool> running = true;

// Signal handler to stop the application
void signalHandler(int signum) {
    running.store(false);
}

// Callback function to write data to a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
  size_t totalSize = size * nmemb;
  userp->append((char*)contents, totalSize);
  return totalSize;
}

void save(json fjs) {
  std::ofstream ofs("save.json");
  ofs << fjs.dump(4);
  ofs.close();
}

void debug(int d, int s = 0) {
  std::cout << "\nStep " << std::to_string(d);
  std::this_thread::sleep_for(std::chrono::seconds(s));
}

bool WaitForDiscordReady(std::shared_ptr<discordpp::Client> client, int timeoutSeconds = 10) {
    std::promise<bool> readyPromise;
    auto readyFuture = readyPromise.get_future();
    auto callback = [&readyPromise](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
        if (status == discordpp::Client::Status::Ready) {
            readyPromise.set_value(true);
        } else if (error != discordpp::Client::Error::None) {
            readyPromise.set_value(false);
        }
    };
    client->SetStatusChangedCallback(callback);

    // Wait for Ready or Error
    auto start = std::chrono::steady_clock::now();
    while (readyFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
        discordpp::RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(timeoutSeconds)) {
            return false; // Timeout
        }
    }
    return readyFuture.get();
}

bool SyncUpdateToken(std::shared_ptr<discordpp::Client> client, discordpp::AuthorizationTokenType tokenType, const std::string& accessToken) {
    std::promise<bool> promise;
    auto future = promise.get_future();
    client->UpdateToken(tokenType, accessToken, [client, &promise](discordpp::ClientResult result) {
        promise.set_value(result.Successful());
    });
    // Run event loop until done
    while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
        discordpp::RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return future.get();
}

struct RefreshResult {
    bool success;
    std::string accessToken;
    std::string refreshToken;
    discordpp::AuthorizationTokenType tokenType;
    int32_t expiresIn;
    std::string scope;
};

RefreshResult SyncRefreshToken(std::shared_ptr<discordpp::Client> client, const std::string& refreshToken) {
    std::promise<RefreshResult> promise;
    auto future = promise.get_future();
    client->RefreshToken(APPLICATION_ID, refreshToken,
        [client, &promise](discordpp::ClientResult result, std::string accessToken, std::string refreshToken, discordpp::AuthorizationTokenType tokenType, int32_t expiresIn, std::string scope) {
            promise.set_value({result.Successful(), accessToken, refreshToken, tokenType, expiresIn, scope});
        }
    );
    // Run event loop until done
    while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
        discordpp::RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return future.get();
}

bool Authorize(std::shared_ptr<discordpp::Client> client, json &fjs) {

  // Generate OAuth2 code verifier for authentication
  std::promise<bool> authPromise;
  auto authFuture = authPromise.get_future();
  auto codeVerifier = client->CreateAuthorizationCodeVerifier();

  // Set up authentication arguments
  discordpp::AuthorizationArgs args{};
  args.SetClientId(APPLICATION_ID);
  args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
  args.SetCodeChallenge(codeVerifier.Challenge());

  // Begin authentication process
  client->Authorize(args, [client, codeVerifier, &authPromise, &fjs](auto result, auto code, auto redirectUri) {
    if (!result.Successful()) {
      std::cerr << "Authentication Error: " << result.Error() << std::endl;
      authPromise.set_value(false);
    } else {
      std::cout << "Authorization successful! Getting access token...\n";
      client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
        [client, &authPromise, &fjs](discordpp::ClientResult result,
        std::string accessToken,
        std::string refreshToken,
        discordpp::AuthorizationTokenType tokenType,
        int32_t expiresIn,
        std::string scope) {
          if(result.Successful()) {
            std::cout << "Access token received! Establishing connection...\n";
            fjs["cache"]["accessToken"] =  accessToken;
            fjs["cache"]["refreshToken"] = refreshToken;
            fjs["cache"]["expiresIn"] =    expiresIn;
            fjs["cache"]["scope"] =        scope;
            client->UpdateToken(tokenType, accessToken, [client, &authPromise](discordpp::ClientResult result) {
              if(result.Successful()) {
                std::cout << "Token updated, connecting to Discord...\n";
                client->Connect();
                authPromise.set_value(true);
              } else {
                authPromise.set_value(false);
              }
            });
          } else {
            authPromise.set_value(false);
          }
        }
      );
    }
  });

  // Wait for the async process to finish
  while (authFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
      discordpp::RunCallbacks();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return authFuture.get();
}

time_t AdjustEpochToUTC(time_t localEpoch) {
    TIME_ZONE_INFORMATION tzInfo;
    DWORD result = GetTimeZoneInformation(&tzInfo);

    // Use only the standard time bias, ignoring daylight saving time
    int bias = tzInfo.Bias; // Standard time bias

    // Convert bias from minutes to seconds and adjust the Epoch time
    time_t utcEpoch = localEpoch + (bias * 60);
    return utcEpoch;
}

int main() {
  json fjs;
  std::ifstream fif("save.json");
  bool auth =      true;
  bool logging =   false;

  // Get JSON Info
  std::cout << "Retrieving data...\n";
  if (fif.good()) {
    try {
        fjs = json::parse(fif);
        auth = fjs["auth"];
        logging = fjs["logging"];
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse save.json: " << e.what() << "\n" << std::endl;
        fjs = json::parse(R"({"auth":true,"logging":false,"cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
        save(fjs);
        auth = true;
        logging = false;
    }
  } else {
    fjs = json::parse(R"({"auth":true,"logging":false,"cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
    save(fjs);
  }

  std::signal(SIGINT, signalHandler);
  std::cout << "Initializing Discord SDK...\n";

  // Create our Discord Client
  auto client = std::make_shared<discordpp::Client>();

  // Set up logging callback
  if (logging) {
    client->AddLogCallback([](auto message, auto severity) {
      std::cout << "[" << EnumToString(severity) << "] " << message << std::endl;
    }, discordpp::LoggingSeverity::Info);
  }

  // Set up status callback to monitor client connection
  client->SetStatusChangedCallback([client](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
    if (status == discordpp::Client::Status::Ready) {
      std::cout << "Client is ready.\n";
    } else if (error != discordpp::Client::Error::None) {
      std::cerr << "Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
    } else {
      std::cout << "Status changed: " << discordpp::Client::StatusToString(status) << std::endl;
    }
  });
  
  // Authorize
  do {
    if (auth) {
      auth = !Authorize(client, fjs);
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::string accessToken =                     fjs["cache"]["accessToken"];
      std::string refreshToken =                    fjs["cache"]["refreshToken"];
      discordpp::AuthorizationTokenType tokenType = discordpp::AuthorizationTokenType::Bearer;
      int32_t expiresIn =                           fjs["cache"]["expiresIn"];
      std::string scope =                           fjs["cache"]["scope"];
      int step =                                    0;

      if (SyncUpdateToken(client, tokenType, accessToken)) {
        client->Connect();
        if (WaitForDiscordReady(client)) {
          std::cout << "Successfully authenticated with cached token.\n";
          auth = false;
        } else {
          std::cout << "Failed to authenticate with cached token. Refreshing...\n";
          auto refreshResult = SyncRefreshToken(client, refreshToken);
          if (refreshResult.success) {
            std::cout << "Refreshed token.\n";
            fjs["cache"]["accessToken"] = refreshResult.accessToken;
            accessToken = refreshResult.accessToken;
            if (SyncUpdateToken(client, tokenType, accessToken)) {
              client->Connect();
              if (WaitForDiscordReady(client)) {
                std::cout << "Successfully authenticated with refreshed token.\n";
                auth = false;
              } else {
                std::cout << "Refreshed token is invalid.\n";
                auth = true;
              }
            }
          } else {
            std::cout << "Failed to refresh token.\n";
          }
        }
      }
      if (auth) {
        auth = !Authorize(client, fjs);
      }
    }

    if (auth) {
      std::string s;
      std::cout << "Failed to authenticate. Press Enter to try again.\n";
      std::getline(std::cin, s);
    }

    fjs["auth"] = auth;
    save(fjs);
  } while (auth);

  json out;
  char buffer[1024];
  const int PORT          = 5005;
  std::string players[8]  = {"Singleplayer","Local 2-Player","Local 3-Player","Local 4-Player","Local 5-Player","Local 6-Player","Local 7-Player","Local 8-Player"};

  // Keep application running to allow SDK to receive events and callbacks
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed.\n";
    return 1;
  }

  // Create UDP socket
  SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET) {
    std::cerr << "Socket creation failed.\n";
    WSACleanup();
    return 1;
  }

  // Bind to all interfaces on the specified port
  sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    std::cerr << "Bind failed.\n";
    closesocket(sock);
    WSACleanup();
    return 1;
  }

  std::cout << "Listening for Wii U broadcasts on port " << PORT << "...\n";

  while (true) {
    discordpp::RunCallbacks();
    sockaddr_in sender {};
    int senderLen = sizeof(sender);
    int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                        (sockaddr*)&sender, &senderLen);
    if (len == SOCKET_ERROR) {
        std::cerr << "recvfrom failed.\n";
        break;
    }

    buffer[len] = '\0'; // Null-terminate
    std::string message = buffer;
    std::cout << "Received: " << buffer << std::endl;
    out = json::parse(message);

    // Configure rich presence details
    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetDetails(out["app"]);
    activity.SetState(players[out["ctrls"]]);
    discordpp::ActivityAssets assets;
    assets.SetLargeImage("preview");
    activity.SetAssets(assets);
    discordpp::ActivityTimestamps timestamps;
    timestamps.SetStart(AdjustEpochToUTC(out["time"]));
    activity.SetTimestamps(timestamps);
    
    // Update rich presence
    client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
            if(result.Successful()) {
              std::cout << "Rich Presence updated successfully!\n";
            } else {
              std::cout << "Rich Presence update failed\n";
            }
          });
  }

  closesocket(sock);
  WSACleanup();
  return 0;
}