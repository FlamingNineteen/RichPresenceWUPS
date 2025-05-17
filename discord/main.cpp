#define DISCORDPP_IMPLEMENTATION
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

bool IsFileUpdated(HINTERNET hFtpSession, const std::wstring& remoteFile, FILETIME& lastModifiedTime) {
    WIN32_FIND_DATAW findData;
    HINTERNET hFind = FtpFindFirstFileW(hFtpSession, remoteFile.c_str(), &findData, 0, 0);
    if (!hFind) {
        std::wcerr << L"Failed to retrieve file metadata. Error: " << GetLastError() << std::endl;
        return false;
    }

    bool updated = CompareFileTime(&findData.ftLastWriteTime, &lastModifiedTime) != 0;
    if (updated) {
        lastModifiedTime = findData.ftLastWriteTime; // Update the last modified time
    }

    InternetCloseHandle(hFind);
    return updated;
}

// FTP server function
int ReadFileFromFTP(const std::wstring& server, const std::wstring& remoteFile, std::string& fileContent) {
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
    char buffer[4096];
    DWORD bytesRead;
    fileContent.clear();

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fileContent.append(buffer, bytesRead);
    }

    // Clean up
    InternetCloseHandle(hFile);
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    std::cout << "File read successfully: " << fileContent << std::endl;
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
    std::signal(SIGINT, signalHandler);
    std::cout << "Initializing Discord SDK...\n";

    // Create our Discord Client
    auto client = std::make_shared<discordpp::Client>();

    // Set up logging callback
    // client->AddLogCallback([](auto message, auto severity) {
    //   std::cout << "[" << EnumToString(severity) << "] " << message << std::endl;
    // }, discordpp::LoggingSeverity::Info);

    // Set up status callback to monitor client connection
    client->SetStatusChangedCallback([client](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
      std::cout << "Status changed: " << discordpp::Client::StatusToString(status) << std::endl;

      if (status == discordpp::Client::Status::Ready) {
        std::cout << "Client is ready! You can now call SDK functions.\n";

      } else if (error != discordpp::Client::Error::None) {
        std::cerr << "Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
      }
    });

    // Generate OAuth2 code verifier for authentication
    auto codeVerifier = client->CreateAuthorizationCodeVerifier();

    // Set up authentication arguments
    discordpp::AuthorizationArgs args{};
    args.SetClientId(APPLICATION_ID);
    args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
    args.SetCodeChallenge(codeVerifier.Challenge());

    // Begin authentication process
    client->Authorize(args, [client, codeVerifier](auto result, auto code, auto redirectUri) {
      if (!result.Successful()) {
        std::cerr << "Authentication Error: " << result.Error() << std::endl;
        return;
      } else {
        std::cout << "Authorization successful! Getting access token...\n";

        // Exchange auth code for access token
        client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
          [client](discordpp::ClientResult result,
          std::string accessToken,
          std::string refreshToken,
          discordpp::AuthorizationTokenType tokenType,
          int32_t expiresIn,
          std::string scope) {
            std::cout << "Access token received! Establishing connection...\n";
            // Next Step: Update the token and connect
            client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,  accessToken, [client](discordpp::ClientResult result) {
              if(result.Successful()) {
                std::cout << "Token updated, connecting to Discord...\n";
                client->Connect();
              }
            });
        });
      }
    });

    // -----------------------------------------------------------------------

    FILE *fptr;

    // IPv4 address
    std::ifstream file("server.txt");
    bool exists = file.good();
    std::string ipStr;
    bool connected = false;
    if(exists) {
      fptr = fopen("server.txt", "r");
      char ipBuffer[15];
      char* ip = fgets(ipBuffer, 15, fptr);
      ipStr = ip;
      fclose(fptr);
      std::cout << "--------------------------------\nLoaded IP address: " << ipStr << "\n";
    } else {
      std::cout << "--------------------------------\nThanks for using Wii U Rich Presence.\nThis plugin is dependent on the ftpiiu plugin. Confirm that it is enabled.\n\nPlease input your Wii U's IP address.\nThis can be found in ftpiiu's plugin configuration settings.\nDo not include the port number.\nType here: ";
      std::cin >> ipStr;
      fptr = fopen("server.txt", "w");
      fprintf(fptr, "%s", ipStr.c_str());
      fclose(fptr);
      std::cout << "Address saved to server.txt\n";
    }

    // Load the timeset.txt file
    std::ifstream fil("timeset.txt");
    exists = fil.good();
    int timeStr;
    if(exists) {
      fptr = fopen("timeset.txt", "r");
      char timeBuffer[3];
      char* time = fgets(timeBuffer, 3, fptr);
      timeStr = std::stoi(time);
      fclose(fptr);
      std::cout << "Loaded time offset: " << timeStr << "\n--------------------------------\n";
    } else {
      fptr = fopen("timeset.txt", "w");
      fprintf(fptr, "0");
      fclose(fptr);
      timeStr = 0;
      std::cout << "If the duration you are playing the game looks wrong when viewed in Discord, change the number in the timeset.txt file to the hours you want to offset by (eg. -1 for 1 hour behind current time).\n--------------------------------\n";
    }
    
    std::wstring server     = std::wstring(ipStr.begin(), ipStr.end());
    std::wstring remoteFile = L"/fs/vol/external01/rich_presence.txt";
    std::string local       = "";
    int i                   = 0;
    int b                   = 0;
    std::string read        = "";
    std::string players[8]  = {"Singleplayer","Local 2-Player","Local 3-Player","Local 4-Player","Local 5-Player","Local 6-Player","Local 7-Player","Local 8-Player"};
    std::string out[3]      = {"","",""};
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
            timestamps.SetStart(AdjustEpochToUTC(std::stoi(out[1])+timeStr*3600));
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
        
  return 0;
}
