#define DISCORDPP_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>
#include <fstream>
#include <windows.h>
#include <wininet.h>
#include <vector>
#include <ctime>
#include <future>
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

// FTP server function
int ReadFileFromFTP(const std::wstring& server, const std::wstring& remoteFile, std::string& fileContent) {
  std::cout << "\n";  
  
  HINTERNET hInternet = InternetOpenW(L"FTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_RELOAD);
    if (!hInternet) {
        std::wcerr << L"InternetOpen failed. Error: " << GetLastError() << std::endl;
        return 1;
    }

    HINTERNET hFtpSession = InternetConnectW(hInternet, server.c_str(), INTERNET_DEFAULT_FTP_PORT, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
    if (!hFtpSession) {
        std::wcerr << L"InternetConnect failed: " << server << L". Error: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Open the remote file with INTERNET_FLAG_RELOAD to bypass caching
    HINTERNET hFile = FtpOpenFileW(hFtpSession, remoteFile.c_str(), GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        std::wcerr << L"FtpOpenFile failed. Error: " << GetLastError() << std::endl;
        InternetCloseHandle(hFtpSession);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Read the file content into a buffer
    char buffer[400];
    DWORD bytesRead;
    fileContent.clear();

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fileContent.append(buffer, bytesRead);
    }

    // Clean up
    InternetCloseHandle(hFile);
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    std::cout << "File read successfully: " << fileContent;
    return 0;
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
        fjs = json::parse(R"({"server":"","time":0,"auth":true,"logging":false,"cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
        save(fjs);
        auth = true;
        logging = false;
    }
  } else {
    fjs = json::parse(R"({"server":"","time":0,"auth":true,"logging":false,"cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
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

  // IPv4 address
  std::string ip;
  if (fjs["server"] != "") {
    std::cout << "Loaded FTP server address\n";
    ip = fjs["server"];
  } else {
    std::cout << "\nThanks for using Wii U Rich Presence.\nThis plugin is dependent on the ftpiiu plugin. Confirm that it is enabled.\n\nPlease input your Wii U's IP address.\nThis can be found in ftpiiu's plugin configuration settings.\nDo not include the port number.\nType here: ";
    std::cin >> ip;
    fjs["server"] = ip;
    save(fjs);
    std::cout << "The FTP address has been saved.\n";
  }
  
  if (fjs["time"] == 0) {
    std::cout << "If the elapsed time does not display correctly, edit the \"time\" key in save.json to shift the hour (-12 to 12).\n\n";
  }

  std::wstring server     = std::wstring(ip.begin(), ip.end());
  std::wstring remoteFile = L"/fs/vol/external01/rich_presence.txt";
  std::string local       = "";
  std::string read        = "";
  std::string players[8]  = {"Singleplayer","Local 2-Player","Local 3-Player","Local 4-Player","Local 5-Player","Local 6-Player","Local 7-Player","Local 8-Player"};
  std::string out[3]      = {"","",""};
  int i                   = 0;
  int b                   = 0;
  int time                = fjs["time"];
  bool connected          = false;
  // out[0] is the game name. out[1] is the time. out[2] is the number of controllers.

  // Keep application running to allow SDK to receive events and callbacks
  while (true) {
    discordpp::RunCallbacks();
    if (b % 500 == 0) {
      b = 0;
      if (connected) {
        if (ReadFileFromFTP(server, remoteFile, local) == 1) {
          connected = false;
        } else {
          read = "";
          i = 0;
          for(char c: local) {
            if (i == 2) {
              out[2] = c;
              break;
            }
            if (c=='^') {
              out[i] = read;
              i++;
              read = "";
            } else {read += c;}
          }

          // Configure rich presence details
          discordpp::Activity activity;
          activity.SetType(discordpp::ActivityTypes::Playing);
          activity.SetDetails(out[0]);
          activity.SetState(players[std::stoi(out[2])]);
          discordpp::ActivityAssets assets;
          assets.SetLargeImage("preview");
          activity.SetAssets(assets);
          discordpp::ActivityTimestamps timestamps;
          timestamps.SetStart(AdjustEpochToUTC(std::stoi(out[1])+time*3600));
          activity.SetTimestamps(timestamps);

          // Update rich presence
          client->UpdateRichPresence(activity, [out, local](discordpp::ClientResult result) {
            if(result.Successful()) {
              std::cout << "Rich Presence updated successfully!\n" << out[0] << ", " << out[1] << ", " << out[2] << "\n";
            } else {
              std::cout << "Rich Presence update failed\n";
            }
          });
        }
      } else {
        if (ReadFileFromFTP(server, remoteFile, local) == 0) {
          connected = true;
        } else {
          // Clear rich presence when disconnected
          client->ClearRichPresence();
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    b++;
  }
  std::cerr << "Error: Skipped loop!";
  return 0;
}