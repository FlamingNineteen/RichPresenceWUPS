#pragma once

/**
 * Direct mode: the plugin talks to Discord itself over HTTPS, so no computer
 * application has to run in the background.
 *
 * It uses Discord's "headless sessions" (POST /users/@me/headless-sessions),
 * authorised with an OAuth2 access token that has the
 * `sdk.social_layer_presence` scope. The tokens come from a file on the SD card
 * that is created once with `setup/discord_setup.py`.
 *
 * Headless sessions are deleted by Discord after 20 minutes without an update,
 * so the plugin refreshes the session every few minutes while a title runs.
 *
 * HTTPS is provided by the CURLWrapperModule (libcurlwrapper), which has to be
 * installed in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
 */

#include <curl/curl.h>

#include <atomic>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

#include "json.hpp"

namespace DiscordDirect {

using json = nlohmann::json;

// Never throws on invalid UTF-8 (a title name could contain some)
std::string Dump(const json &j) {
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

const std::string API        = "https://discord.com/api/v10";
const std::string ICON_REPO  = "https://raw.githubusercontent.com/flamingnineteen/richpresencewups-db/main";
const char *USER_AGENT       = "WiiURichPresence (https://github.com/sennecaelen/RichPresenceWUPS)";
const char *TOKEN_FILE_NAME  = "richpresence_discord.json";

// Refresh the access token this long before it expires.
constexpr long long REFRESH_MARGIN = 24 * 3600;

/**
 * Everything the plugin wants to show on Discord.
 */
struct Presence {
    std::string name;     // Title shown as the activity name
    std::string longName; // Full English title, used to find the icon
    std::string nnid;     // Network ID, empty to hide
    std::string network;  // "nn", "pn" or empty
    int ctrls = -2;       // Controller count, -2 to hide
    long long start = 0;  // Start time in Wii U local seconds

    std::string Key() const {
        return name + "|" + longName + "|" + nnid + "|" + network + "|" + std::to_string(ctrls) + "|" + std::to_string(start);
    }
};

struct Tokens {
    std::string clientId;
    std::string clientSecret; // optional, only for non-public applications
    std::string accessToken;
    std::string refreshToken;
    long long expiresAt = 0; // UTC seconds
};

/*
 * State. Only the presence thread touches this while it runs; the main thread
 * only uses it after joining that thread.
 */
bool curlReady = false;
std::string tokenPath;
Tokens tokens;
bool tokensLoaded  = false;
bool needsSetup    = false; // the tokens were rejected, the setup script has to run again
std::string sessionToken;
long long clockOffset = 0; // UTC - Wii U local time, in seconds
bool clockKnown       = false;
std::string titles; // raw titles.json, searched instead of parsed to save memory
bool titlesLoaded = false;
std::string lastError = "Not connected yet";
std::atomic<bool> abortRequests = false;

struct Response {
    bool ok     = false; // the transfer itself worked
    long status = 0;     // HTTP status code
    std::string body;
};

size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    try {
        auto *out = static_cast<std::string *>(userdata);
        if (out->size() + size * nmemb > 256 * 1024) return 0; // nothing we fetch is this big
        out->append(ptr, size * nmemb);
        return size * nmemb;
    } catch (...) {
        return 0; // makes curl stop with an error instead of crashing
    }
}

// Lets a running request be cancelled when the title closes.
int ProgressCallback(void *, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return abortRequests ? 1 : 0;
}

/**
 * Performs an HTTPS request.
 * A request without body and content type is a GET, otherwise a POST.
 * The `Date` header of every response is used to learn the UTC offset of the
 * Wii U clock, which is set to local time.
 */
Response Request(const std::string &url, const std::string &body = "", const std::string &contentType = "", const std::string &bearer = "", long timeout = 8) {
    Response r;
    if (!curlReady) {
        lastError = "CURLWrapperModule is not loaded";
        return r;
    }

    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        lastError = "curl_easy_init failed";
        return r;
    }

    struct curl_slist *headers = nullptr;
    if (!contentType.empty()) headers = curl_slist_append(headers, ("Content-Type: " + contentType).c_str());
    if (!bearer.empty()) headers = curl_slist_append(headers, ("Authorization: Bearer " + bearer).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);

    if (!body.empty() || !contentType.empty()) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        r.ok = true;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.status);

        struct curl_header *date;
        if (curl_easy_header(curl, "Date", 0, CURLH_HEADER, -1, &date) == CURLHE_OK) {
            time_t utc = curl_getdate(date->value, nullptr);
            if (utc > 0) {
                clockOffset = static_cast<long long>(utc) - static_cast<long long>(time(NULL));
                clockKnown  = true;
            }
        }
    }
    else {
        lastError = std::string("Network error: ") + curl_easy_strerror(res);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return r;
}

std::string UrlEncode(const std::string &s) {
    static const char *hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        }
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

long long NowUtc() {
    return static_cast<long long>(time(NULL)) + clockOffset;
}

/**
 * Reads the token file created by the setup script.
 */
bool LoadTokens() {
    std::ifstream f(tokenPath);
    if (!f.is_open()) {
        lastError = "No token file, run the setup script";
        return false;
    }

    try {
        json j               = json::parse(f);
        tokens.clientId      = j.at("client_id").get<std::string>();
        tokens.accessToken   = j.at("access_token").get<std::string>();
        tokens.refreshToken  = j.value("refresh_token", std::string());
        tokens.clientSecret  = j.value("client_secret", std::string());
        tokens.expiresAt     = j.value("expires_at", 0LL);
    } catch (...) {
        lastError = "Token file is invalid, run the setup script";
        return false;
    }

    tokensLoaded = true;
    needsSetup   = false;
    return true;
}

/**
 * Writes the tokens back, so a rotated refresh token survives a reboot.
 */
bool SaveTokens() {
    json j = {
        {"client_id", tokens.clientId},
        {"access_token", tokens.accessToken},
        {"refresh_token", tokens.refreshToken},
        {"expires_at", tokens.expiresAt}};
    if (!tokens.clientSecret.empty()) j["client_secret"] = tokens.clientSecret;

    std::string tmp = tokenPath + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        if (!f.is_open()) return false;
        f << j.dump(4, ' ', false, json::error_handler_t::replace);
        if (!f.good()) return false;
    }

    std::error_code ec;
    std::filesystem::remove(tokenPath, ec);
    std::filesystem::rename(tmp, tokenPath, ec);
    return !ec;
}

/**
 * Trades the refresh token for a new access token.
 */
bool RefreshTokens() {
    if (tokens.refreshToken.empty()) {
        needsSetup = true;
        lastError  = "Access token expired, run the setup script";
        return false;
    }

    std::string body = "grant_type=refresh_token&refresh_token=" + UrlEncode(tokens.refreshToken) +
                       "&client_id=" + UrlEncode(tokens.clientId);
    if (!tokens.clientSecret.empty()) body += "&client_secret=" + UrlEncode(tokens.clientSecret);

    Response r = Request(API + "/oauth2/token", body, "application/x-www-form-urlencoded");
    if (!r.ok) return false;

    if (r.status != 200) {
        // 400/401 mean the refresh token is no longer valid
        if (r.status == 400 || r.status == 401) needsSetup = true;
        lastError = "Token refresh failed (" + std::to_string(r.status) + ")";
        return false;
    }

    try {
        json j             = json::parse(r.body);
        tokens.accessToken = j.at("access_token").get<std::string>();
        if (j.contains("refresh_token")) tokens.refreshToken = j["refresh_token"].get<std::string>();
        tokens.expiresAt = NowUtc() + j.value("expires_in", 604800LL);
    } catch (...) {
        lastError = "Token refresh returned invalid data";
        return false;
    }

    if (!SaveTokens()) lastError = "Could not save refreshed tokens to the SD card";
    return true;
}

/**
 * Fetches the title -> icon list from the image repository.
 */
void LoadTitles() {
    Response r = Request(ICON_REPO + "/titles.json");
    if (!r.ok || r.status != 200) return;
    titles       = std::move(r.body);
    titlesLoaded = true;
}

/**
 * Looks up `"<title>": "<icon>"` in the raw titles.json.
 */
std::string IconUrl(const std::string &longName) {
    const std::string fallback = ICON_REPO + "/icons/menu.jpg";
    if (!titlesLoaded) return fallback;

    std::string key = Dump(json(longName)); // quoted and escaped like in the file
    size_t pos      = titles.find(key);
    while (pos != std::string::npos) {
        size_t i = pos + key.size();
        while (i < titles.size() && isspace(static_cast<unsigned char>(titles[i]))) i++;
        if (i < titles.size() && titles[i] == ':') {
            i++;
            while (i < titles.size() && isspace(static_cast<unsigned char>(titles[i]))) i++;
            if (i < titles.size() && titles[i] == '"') {
                size_t end = titles.find('"', i + 1);
                if (end == std::string::npos) return fallback;
                return ICON_REPO + "/icons/" + titles.substr(i + 1, end - i - 1);
            }
            return fallback;
        }
        pos = titles.find(key, pos + 1);
    }
    return fallback;
}

json BuildActivity(const Presence &p) {
    json activity = {
        {"name", p.name},
        {"application_id", tokens.clientId},
        {"platform", "desktop"},
        {"type", 0},
        {"details", "Playing on the Wii U"},
        {"timestamps", {{"start", (p.start + clockOffset) * 1000}}},
        {"assets", {{"large_image", IconUrl(p.longName)}, {"large_text", p.longName}}}};

    if (!p.nnid.empty()) activity["state"] = "NID: " + p.nnid;

    // "nn" and "pn" are art asset keys; upload images with these names to
    // your Discord application to show them.
    if (!p.network.empty()) {
        activity["assets"]["small_image"] = p.network;
        activity["assets"]["small_text"]  = p.network == "nn" ? "Using Nintendo Network" : "Using Pretendo Network";
    }

    if (p.ctrls > -2) {
        int size              = p.ctrls + 1;
        activity["party"]     = {{"id", "wiiu"}, {"size", {size, size > 4 ? 8 : 4}}};
    }

    return activity;
}

Response PostSession(const json &activity) {
    json body = {{"activities", json::array({activity})}};
    if (!sessionToken.empty()) body["token"] = sessionToken;
    return Request(API + "/users/@me/headless-sessions", Dump(body), "application/json", tokens.accessToken);
}

/**
 * Creates or updates the headless session.
 * @return Whether Discord accepted the presence.
 */
bool UpdateUnsafe(const Presence &p) {
    if (!tokensLoaded && !LoadTokens()) return false;
    if (needsSetup) return false;

    // The first request teaches us the clock offset, which the start time needs.
    if (!titlesLoaded) LoadTitles();
    if (!clockKnown) Request(API + "/gateway");
    if (!clockKnown) return false;

    if (tokens.expiresAt != 0 && NowUtc() > tokens.expiresAt - REFRESH_MARGIN) RefreshTokens();

    json activity = BuildActivity(p);
    Response r    = PostSession(activity);
    if (!r.ok) return false;

    if (r.status == 401) {
        if (!RefreshTokens()) return false;
        r = PostSession(activity);
        if (!r.ok) return false;
    }

    // A rejected session token (expired after 20 minutes, or deleted) is
    // replaced by a new session.
    if (r.status >= 400 && r.status < 500 && r.status != 401 && r.status != 429 && !sessionToken.empty()) {
        sessionToken.clear();
        r = PostSession(activity);
        if (!r.ok) return false;
    }

    if (r.status == 200) {
        try {
            sessionToken = json::parse(r.body).value("token", std::string());
        } catch (...) {
        }
        lastError = "";
        return true;
    }

    if (r.status == 401 || r.status == 403) needsSetup = true;
    lastError = "Discord error " + std::to_string(r.status) + ": " + r.body.substr(0, 60);
    return false;
}

/**
 * Removes the presence from Discord right away instead of waiting for the
 * session to expire.
 */
void ClearUnsafe() {
    if (sessionToken.empty() || !tokensLoaded) return;
    json body = {{"token", sessionToken}};
    Request(API + "/users/@me/headless-sessions/delete", Dump(body), "application/json", tokens.accessToken, 3);
    sessionToken.clear();
}

/**
 * Safe wrappers: an error must never crash the console, it is shown in the
 * config menu instead.
 */
bool Update(const Presence &p) {
    try {
        return UpdateUnsafe(p);
    } catch (const std::exception &e) {
        lastError = std::string("Internal error: ") + e.what();
    } catch (...) {
        lastError = "Internal error";
    }
    return false;
}

void Clear() {
    try {
        ClearUnsafe();
    } catch (...) {
        sessionToken.clear();
    }
}

void Init(const std::string &environmentPath) {
    tokenPath = environmentPath + "/plugins/config/" + TOKEN_FILE_NAME;
    curlReady = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    if (!curlReady) lastError = "CURLWrapperModule is not loaded";
}

void Deinit() {
    if (curlReady) curl_global_cleanup();
    curlReady = false;
}

/**
 * One line for the config menu.
 */
std::string StatusText() {
    if (!curlReady) return "Status: CURLWrapperModule missing";
    std::error_code ec;
    if (!tokensLoaded && !std::filesystem::exists(tokenPath, ec)) return "Status: no token file, run the setup script";
    if (needsSetup) return "Status: Discord login expired, run the setup script";
    if (lastError.empty()) return "Status: connected to Discord";
    return "Status: " + lastError;
}

} // namespace DiscordDirect
