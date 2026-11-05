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
#include <map>
#include <set>
#include <string>

#include "json.hpp"

namespace DiscordDirect {

using json = nlohmann::json;

// Never throws on invalid UTF-8 (a title name could contain some)
std::string Dump(const json &j) {
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

std::string API              = "https://discord.com/api/v10";
const std::string ICON_REPO  = "https://raw.githubusercontent.com/flamingnineteen/richpresencewups-db/main";
const char *USER_AGENT       = "WiiURichPresence (https://github.com/sennecaelen/RichPresenceWUPS)";
const char *TOKEN_FILE_NAME  = "richpresence_discord.json";

// The original plugin's Discord application. Its art assets ("nn", "pn",
// "preview") are used when your own application doesn't have them.
const std::string ORIGINAL_APP_ID = "1353248127469228074";

// Nintendo Network / Pretendo icons hosted in this repository. Loaded the same
// way as the game icons, which Discord shows reliably.
const std::string NETWORK_ICONS = "https://raw.githubusercontent.com/sennecaelen/RichPresenceWUPS/main/icons/";

// Refresh the access token this long before it expires.
constexpr long long REFRESH_MARGIN = 24 * 3600;

/**
 * Everything the plugin wants to show on Discord.
 */
struct Presence {
    std::string name;     // Title shown as the activity name
    std::string longName; // Full English title, used to find the icon
    std::string nnid;     // Network ID, empty to hide
    std::string network;  // "nn", "pn", "pr" (Protarium) or empty, for the small icon
    std::string realNetwork; // network actually in use, also when its icon is hidden
    int ctrls = -2;       // Controller count, -2 to hide
    long long start = 0;  // Start time in Wii U local seconds

    std::string Key() const {
        return name + "|" + longName + "|" + nnid + "|" + network + "|" + std::to_string(ctrls) + "|" + std::to_string(start) + "|" + realNetwork;
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
std::set<std::string> rejectedFields; // activity fields Discord refused, left out from then on
std::map<std::string, std::string> rejectedReasons;
std::map<std::string, std::string> assetUrls; // art asset name -> image URL
bool assetsLoaded = false;
std::string assetsInfo = "not loaded";  // shown in the config menu
int partyFormat = 0;                     // which party layout Discord accepts, see PartyJson
long long blockedUntil = 0;           // Wii U local time until which Discord asked us to wait

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

/**
 * Reads the art assets of an application (a public endpoint) so they can be
 * used as image URLs.
 */
int LoadAssetsOf(const std::string &appId) {
    // Public endpoint; a token for another application would only get refused
    Response r = Request(API + "/oauth2/applications/" + appId + "/assets");
    if (!r.ok) return -1;
    if (r.status != 200) return static_cast<int>(r.status);
    try {
        json list = json::parse(r.body);
        if (!list.is_array()) return -2;
        for (const json &asset : list) {
            std::string name = asset.value("name", std::string());
            std::string id   = asset.value("id", std::string());
            // Your own application wins over the original one
            if (!name.empty() && !id.empty() && !assetUrls.count(name)) {
                assetUrls[name] = "https://cdn.discordapp.com/app-assets/" + appId + "/" + id + ".png";
            }
        }
        return 200;
    } catch (...) {
        return -2;
    }
}

/**
 * Loads the art assets once per boot. The result is shown in the menu, so a
 * missing icon can be explained.
 */
void LoadAssets() {
    int own      = tokens.clientId.empty() ? 0 : LoadAssetsOf(tokens.clientId);
    int original = LoadAssetsOf(ORIGINAL_APP_ID);
    assetsLoaded = true;

    assetsInfo = "your app " + std::to_string(own) + ", original " + std::to_string(original);
}

/**
 * Discord documents the party size as [current, max], but headless sessions
 * answer "Expected an object". The layouts are tried in order and the first
 * one Discord accepts is kept.
 */
constexpr int PARTY_FORMATS = 3;
json PartyJson(int size, int max) {
    switch (partyFormat) {
        case 0:
            return {{"id", "wiiu"}, {"size", {{"current_size", size}, {"max_size", max}}}};
        case 1:
            return {{"id", "wiiu"}, {"current_size", size}, {"max_size", max}};
        default:
            return {{"id", "wiiu"}, {"size", {{"current", size}, {"max", max}}}};
    }
}

std::string AssetUrl(const std::string &name) {
    auto it = assetUrls.find(name);
    return it != assetUrls.end() ? it->second : "";
}

json BuildActivity(const Presence &p) {
    json activity = {
        {"name", p.name},
        {"application_id", tokens.clientId},
        {"platform", "desktop"},
        {"type", 0},
        {"status_display_type", 0}, // "Playing <game>" in the member list, like the original
        {"details", "Playing on the Wii U"},
        {"timestamps", {{"start", (p.start + clockOffset) * 1000}}},
        {"assets", {{"large_image", IconUrl(p.longName)}, {"large_text", p.longName}}}};

    // Titles that aren't in the icon database get the original "preview" image
    if (activity["assets"]["large_image"] == ICON_REPO + "/icons/menu.jpg" && !AssetUrl("preview").empty()) {
        activity["assets"]["large_image"] = AssetUrl("preview");
    }

    // "NNID: name" under the game, "PNID: name" on Pretendo and Protarium
    const std::string &net = p.realNetwork.empty() ? p.network : p.realNetwork;
    std::string state      = p.nnid.empty() ? "" : (net == "pn" || net == "pr" ? "PNID: " : "NNID: ") + p.nnid;

    // Nintendo Network / Pretendo icon, from the art assets "nn" and "pn"
    if (!p.network.empty()) {
        // Your own "nn"/"pn" art assets win, otherwise the icons from GitHub
        std::string url = AssetUrl(p.network);
        if (url.empty() || url.find("/" + ORIGINAL_APP_ID + "/") != std::string::npos) url = NETWORK_ICONS + p.network + ".png";
        if (!url.empty()) {
            activity["assets"]["small_image"] = url;
            activity["assets"]["small_text"]  = p.network == "nn" ? "Using Nintendo Network" : (p.network == "pr" ? "Using Protarium Network" : "Using Pretendo Network");
        }
    }

    if (p.ctrls > -2 && p.ctrls + 1 >= 1) {
        int size = p.ctrls + 1;
        int max  = size > 4 ? 8 : 4;
        if (rejectedFields.count("party")) {
            // Discord doesn't take a party here; show it the way Discord
            // would have: "NNID: name (1 of 4)"
            state += (state.empty() ? "" : " ") + std::string("(") + std::to_string(size) + " of " + std::to_string(max) + ")";
        }
        else {
            activity["party"] = PartyJson(size, max);
        }
    }

    if (!state.empty()) activity["state"] = state;

    return activity;
}

/**
 * Finds the first entry in Discord's "errors" tree.
 * @param path Set to e.g. "activities.0.party.size".
 * @return The error message, or the top level message.
 */
std::string FirstError(const Response &r, std::string &path) {
    path.clear();
    try {
        json j           = json::parse(r.body);
        std::string text = j.value("message", std::string("unknown"));

        const json *node = j.contains("errors") ? &j["errors"] : nullptr;
        while (node != nullptr && node->is_object() && !node->contains("_errors") && !node->empty()) {
            path += (path.empty() ? "" : ".") + node->begin().key();
            node = &node->begin().value();
        }
        if (node != nullptr && node->is_object() && node->contains("_errors") && (*node)["_errors"].is_array() && !(*node)["_errors"].empty()) {
            const json &e = (*node)["_errors"][0];
            text          = e.value("message", e.value("code", std::string("invalid")));
        }
        return text;
    } catch (...) {
        return r.body.substr(0, 80);
    }
}

std::string DescribeError(const Response &r) {
    std::string path;
    std::string text = FirstError(r, path);
    return "Discord " + std::to_string(r.status) + ": " + (path.empty() ? "" : path + ": ") + text;
}

/**
 * Which activity field an error path points at: "party", "timestamps",
 * "assets.small_image", ... Empty when it's not about the activity.
 */
std::string FieldFromPath(const std::string &path) {
    const std::string prefix = "activities.0.";
    if (path.rfind(prefix, 0) != 0) return "";
    std::string rest  = path.substr(prefix.size());
    std::string first = rest.substr(0, rest.find('.'));
    if (first == "assets" && rest.size() > 7) {
        std::string second = rest.substr(7);
        return "assets." + second.substr(0, second.find('.'));
    }
    return first;
}

// Fields that can never be dropped, Discord needs them
bool IsRequiredField(const std::string &field) {
    return field.empty() || field == "name" || field == "application_id" || field == "type" || field == "platform";
}

void StripRejected(json &activity) {
    for (const std::string &field : rejectedFields) {
        if (field.rfind("assets.", 0) == 0) {
            if (!activity.contains("assets")) continue;
            std::string key = field.substr(7);
            activity["assets"].erase(key);
            if (key == "large_image") activity["assets"].erase("large_text");
            if (key == "small_image") activity["assets"].erase("small_text");
            if (activity["assets"].empty()) activity.erase("assets");
        }
        else {
            activity.erase(field);
        }
    }
}

std::string RejectedList() {
    std::string out;
    for (const std::string &field : rejectedFields) {
        auto it = rejectedReasons.find(field);
        out += (out.empty() ? "" : ", ") + (it != rejectedReasons.end() ? it->second : field);
    }
    return out;
}

/**
 * Seconds Discord wants us to wait after a 429.
 */
long long RetryAfter(const Response &r) {
    try {
        double seconds = json::parse(r.body).value("retry_after", 60.0);
        return static_cast<long long>(seconds) + 1;
    } catch (...) {
        return 60;
    }
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

    // Discord asked us to slow down
    long long now = static_cast<long long>(time(NULL));
    if (now < blockedUntil) return false;

    // The first request teaches us the clock offset, which the start time needs.
    if (!titlesLoaded) LoadTitles();
    if (!assetsLoaded) LoadAssets();
    if (!clockKnown) Request(API + "/gateway");
    if (!clockKnown) return false;

    if (tokens.expiresAt != 0 && NowUtc() > tokens.expiresAt - REFRESH_MARGIN) RefreshTokens();

    json activity = BuildActivity(p);
    StripRejected(activity);
    Response r = PostSession(activity);
    if (!r.ok) return false;

    if (r.status == 401) {
        if (!RefreshTokens()) return false;
        r = PostSession(activity);
        if (!r.ok) return false;
    }

    // Discord refused a field: leave it out (also for later updates) and retry.
    // Limited, so a problem we don't understand can't cause a burst of requests.
    for (int attempt = 0; attempt < 3 && r.status == 400; attempt++) {
        std::string path;
        std::string reason = FirstError(r, path);

        if (path.rfind("token", 0) == 0 && !sessionToken.empty()) {
            sessionToken.clear(); // the session expired, start a new one
        }
        else {
            std::string field = FieldFromPath(path);
            if (IsRequiredField(field) || rejectedFields.count(field)) break;

            if (field == "party" && partyFormat + 1 < PARTY_FORMATS) {
                partyFormat++; // try the next layout
            }
            else {
                rejectedFields.insert(field);
                rejectedReasons[field] = path.substr(13) + ": " + reason;
            }

            // Rebuild, so a refused party moves into the state text
            activity = BuildActivity(p);
            StripRejected(activity);
        }

        r = PostSession(activity);
        if (!r.ok) return false;
    }

    // The session is gone (expired or removed): start a new one, once
    if (r.status == 404 && !sessionToken.empty()) {
        sessionToken.clear();
        r = PostSession(activity);
        if (!r.ok) return false;
    }

    if (r.status == 429) {
        long long wait = RetryAfter(r);
        blockedUntil   = now + wait;
        lastError      = "Discord rate limit, waiting " + std::to_string(wait) + "s";
        return false;
    }

    if (r.status == 200) {
        try {
            std::string token = json::parse(r.body).value("token", std::string());
            if (!token.empty()) sessionToken = token;
        } catch (...) {
        }
        lastError = rejectedFields.empty() ? "" : "connected, Discord refused " + RejectedList();
        return true;
    }

    if (r.status == 401 || r.status == 403) needsSetup = true;
    lastError = DescribeError(r);
    return false;
}

/**
 * Removes the presence from Discord right away instead of waiting for the
 * session to expire.
 */
void ClearUnsafe() {
    if (sessionToken.empty() || !tokensLoaded) return;

    // Skip while Discord asked us to wait; a short timeout keeps app exits quick
    if (static_cast<long long>(time(NULL)) >= blockedUntil) {
        json body = {{"token", sessionToken}};
        Response r = Request(API + "/users/@me/headless-sessions/delete", Dump(body), "application/json", tokens.accessToken, 2);
        if (r.ok && r.status == 429) blockedUntil = static_cast<long long>(time(NULL)) + RetryAfter(r);
    }
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
    if (lastError.rfind("connected", 0) == 0) return "Status: " + lastError;
    if (!lastError.empty() && !rejectedFields.empty()) return "Status: " + lastError + " (refused: " + RejectedList() + ")";
    return "Status: " + lastError;
}

} // namespace DiscordDirect
