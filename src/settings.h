#pragma once
#include "diag_log.h"
#include "scroll_repeat.h"
#include "keyboard_languages.h"
#include "proxy_config.h"
#include "proxy_pool.h"
#include "secure_store.h"
#include "vault.h"
#include "ui_themes.h"
#include "user_fonts.h"
#include <jansson.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <string>

struct AppSettings {
    int retention = 2, capacity = 2, theme = 1, font = 5, request_batch_size = 64;
    int opacity = 75;       // percent of the surface PNGs (rows, message cards) over the wallpaper, 5% steps
    int avatar_shape = 0;   // 0 rounded square, 1 circle
    int chat_open_position = 0; // Latest, Telegram read position, device read position.
    bool unread_counter = false;
    bool hide_muted_unread = false;
    bool combined_view = false; // Keep the classic full-screen navigation by default.
    int video_quality = 0; // Maximum: minimum, up to 360p, 480p or 720p.
    int scroll_speed = 0;   // Normal, Fast, Very fast: 2, 4, 6 pages per second.
    std::vector<std::string> keyboard_languages; // Empty = English + supported console language.
    int keyboard_split = 1; // Small, medium (default), wide.
    bool custom_keyboard = false; // Use the in-app keyboard instead of the Vita IME.
    bool show_threads = false;
    bool user_colors = true;
    bool gyroscope_enabled = true;
    bool show_archive = true;  // Archive tab in the chat list folders
    unsigned log_mask = diag_log::kStartupMask; // Proxy diagnostics start automatically; other logs are opt-in.
    static constexpr int kSpoilerResetChoices = 4;
    int spoiler_reset = 3; // 0 off, 1 chat exit, 2 app leave, 3 both.
    bool spoilerHideChat() const { return (spoiler_reset & 1) != 0; }
    bool spoilerHideApp() const { return (spoiler_reset & 2) != 0; }
    bool recording_shortcuts = true; // L/R open the video/voice recording dialogs from chat history.
    bool allow_vpk_install = false; // Explicit opt-in for installing packages downloaded from chats.
    bool check_updates = true; // look for a newer GitHub release shortly after startup
    // With the vault on: ask for the key again after this much inactivity (index into lockMinutes),
    // and after sleep or leaving the app with the PS button.
    int auto_lock = 3;
    bool lock_on_leave = true;
    bool call_low_latency = false;
    bool proxy_enabled = false;
    std::string proxy_server, proxy_secret;
    int proxy_port = 443;
    int proxy_type = 0; // ProxyKind: 0 MTProto, 1 SOCKS5, 2 HTTP CONNECT, 3 WEB
    std::string proxy_username, proxy_password;
    std::vector<ProxyConfig> proxies;
    int active_proxy=-1;
    bool proxy_auto_switch=true;
    int proxy_switch_seconds=10;
    void selectProxy(int index) {
        active_proxy=index;
        const ProxyConfig c=index>=0 && index<int(proxies.size())?proxies[index]:ProxyConfig{};
        proxy_type=int(c.kind);proxy_server=c.server;proxy_port=c.port;proxy_secret=c.secret;
        proxy_username=c.username;proxy_password=c.password;
    }
    bool storeProxyDraft(int index) {
        const auto c=proxyConfig();
        if (!c.complete() || c.server.size()>(c.kind==ProxyKind::Web?384u:253u) || c.secret.size()>1023 ||
            c.username.size()>255 || c.password.size()>255 || c.server.find('\0')!=std::string::npos ||
            c.secret.find('\0')!=std::string::npos || c.username.find('\0')!=std::string::npos || c.password.find('\0')!=std::string::npos) return false;
        if (index<0) {
            for (size_t i=0;i<proxies.size();++i) if (same_proxy(c,proxies[i])) { selectProxy(i);return true; }
            if (proxies.size()>=ProxyPool::Limit) return false;
            index=proxies.size();proxies.push_back(c);
        } else if (index<int(proxies.size())) proxies[index]=c;
        else return false;
        selectProxy(index);return true;
    }
    void eraseProxy(int index) {
        if (index<0 || index>=int(proxies.size())) return;
        proxies.erase(proxies.begin()+index);
        selectProxy(proxies.empty()?-1:active_proxy>index?active_proxy-1:std::min(active_proxy,int(proxies.size())-1));
    }
    ProxyConfig proxyConfig() const {
        ProxyConfig c;
        c.kind = proxy_kind_from_int(proxy_type); c.server = proxy_server; c.port = c.kind == ProxyKind::Web ? 443 : proxy_port;
        c.secret = proxy_secret; c.username = proxy_username; c.password = proxy_password;
        return c;
    }
    int cache_storage = 0;
    bool download_ask = false;
    std::string download_last_dir;
    std::string download_dir = "ux0:downloads/vitagram";
    std::string font_file; // Empty selects the built-in font chain.
    std::string language; // "" = follow the console language, otherwise a code from app0:lang/*.lang
    // load() found a settings.json not sealed by the vault although the vault is on: it was replaced
    // outside VitaPlane (or never re-saved after switching encryption on) and has been ignored.
    bool rejected = false;
    // load() read a settings.json (sealed with the vault key when the vault is on).
    bool loaded = false;
    static int days(int i) { const int values[] = {1, 7, 30, 90, 0}; return values[i]; }
    static int megabytes(int i) { const int values[] = {128, 256, 512, 1024, 2048, 0}; return values[i]; }
    static int requestBatchSize(int i) { const int values[] = {4, 8, 16, 32, 64}; return values[std::max(0, std::min(4, i))]; } // Оптимизация: сдвинул значения вправо (прежн. индекс 1→теперь значение 32)
    static int requestBatchIndex(int value) {
        const int values[] = {4, 8, 16, 32, 64};
        int best = 0;
        for (int i = 1; i < 5; ++i) if (std::abs(values[i] - value) < std::abs(values[best] - value)) best = i;
        return best;
    }
    static constexpr int kLockChoices = 7;
    static int lockMinutes(int i) { const int values[] = {0, 1, 2, 5, 10, 15, 30}; return values[std::max(0, std::min(kLockChoices - 1, i))]; }
    static constexpr int kMinOpacity = 20, kMaxOpacity = 100, kOpacityStep = 5;
    // The theme alone is also kept in a plain file: with the vault on, settings.json is sealed, yet the
    // unlock screens (shown before the key is entered) must already use the chosen theme.
    static int themeChoices() { return ui_themes::count(); }
    bool isDark() const { return ui_themes::get(theme).base != 0; }
    static constexpr const char* kThemeHint = "ux0:data/vitagram/theme.txt";
    static int themeHint(int fallback) {
        int value = fallback;
        if (FILE* file = std::fopen(kThemeHint, "r")) {
            char text[160]{};
            const size_t size = std::fread(text, 1, sizeof(text) - 1, file);
            if (size > 0 && size < sizeof(text) - 1) {
                if (std::strncmp(text, "file:", 5) == 0) {
                    const std::string filename(text + 5, size - 5);
                    value = ui_themes::find_file(filename);
                    std::fclose(file);
                    return value;
                }
                char* end = nullptr;
                const long parsed = std::strtol(text, &end, 10);
                const bool digits = end != text;
                while (*end && std::isspace((unsigned char)*end)) ++end;
                if (digits && end == text + size && parsed >= 0 && parsed < 16) value = ui_themes::from_saved_id(parsed);
            }
            std::fclose(file);
        }
        return value;
    }
    void saveThemeHint() const {
        if (themeHint(-1) == theme) return;
        if (FILE* file = std::fopen(kThemeHint, "w")) {
            if (const auto* custom = ui_themes::custom(theme)) std::fprintf(file, "file:%s", custom->filename.c_str());
            else std::fprintf(file, "%d", ui_themes::saved_id(theme));
            std::fclose(file);
        }
    }
    float fontScale() const { return 0.7f + font * 0.1f; }
    void load() {
        // Sealed when the vault is on; the vault must be unlocked first, or this reads defaults.
        // Always starts from the defaults, so nothing read earlier survives a refused file.
        static constexpr const char* path = "ux0:data/vitagram/settings.json";
        *this = AppSettings();
        const bool strict = vault::settings_must_be_sealed();
        std::string text;
        if (!vault::read_file(path, 1 << 20, text, nullptr, nullptr, strict ? vault::Require::Vault : vault::Require::Any)) {
            struct stat info{};
            rejected = strict && stat(path, &info) == 0 && !vault::is_vault_sealed(path);
            return;
        }
        json_error_t error{};
        json_t* root = json_loadb(text.data(), text.size(), 0, &error);
        if (!json_is_object(root)) { if (root) json_decref(root); return; }
        loaded = true;
        auto value = [&](const char* key, int fallback, int maximum) {
            json_t* v = json_object_get(root, key);
            return json_is_integer(v) ? std::max(0, std::min(maximum, (int)json_integer_value(v))) : fallback;
        };
        retention = value("retention", 2, 4); capacity = value("capacity", 2, 5);
        theme = ui_themes::from_saved_id(json_is_integer(json_object_get(root,"theme")) ? json_integer_value(json_object_get(root,"theme")) : 1);
        if (const char* file = json_string_value(json_object_get(root, "theme_file"))) theme = ui_themes::find_file(file);
        if (json_t* v = json_object_get(root, "video_quality"))
            if (json_is_integer(v) && json_integer_value(v) >= 0 && json_integer_value(v) <= 3)
                video_quality = (int)json_integer_value(v);
        chat_open_position = value("chat_open_position", 0, 2);
        if (json_t* v = json_object_get(root, "combined_view"))
            if (json_is_boolean(v)) combined_view = json_is_true(v);
        if (json_t* v = json_object_get(root, "unread_counter"))
            if (json_is_boolean(v)) unread_counter = json_is_true(v);
        if (json_t* v = json_object_get(root, "hide_muted_unread"))
            if (json_is_boolean(v)) hide_muted_unread = json_is_true(v);
        font = value("font_size", 5, 6);
        request_batch_size = value("request_batch_size", 64, 64);
        opacity = std::max(kMinOpacity, std::min(kMaxOpacity, (value("opacity", 75, 100) + kOpacityStep / 2) / kOpacityStep * kOpacityStep));
        avatar_shape = value("avatar_shape", 0, 1);
        if (json_t* v = json_object_get(root, "scroll_speed"))
            if (json_is_integer(v) && json_integer_value(v) >= 0 && json_integer_value(v) < ScrollRepeat::kSpeedChoices)
                scroll_speed = (int)json_integer_value(v);
        if (json_t* v = json_object_get(root, "keyboard_split"))
            if (json_is_integer(v) && json_integer_value(v) >= 0 && json_integer_value(v) <= 2)
                keyboard_split = (int)json_integer_value(v);
        if (json_t* v = json_object_get(root, "custom_keyboard"))
            if (json_is_boolean(v)) custom_keyboard = json_is_true(v);
        if (json_t* languages = json_object_get(root, "keyboard_languages")) {
            if (json_is_array(languages)) {
                for (size_t i = 0; i < json_array_size(languages); ++i)
                    if (const char* code = json_string_value(json_array_get(languages, i))) keyboard_languages.emplace_back(code);
                keyboard_languages = keyboard_languages::normalize(keyboard_languages);
            }
        }
        cache_storage = value("cache_storage", 0, 1);
        auto_lock = value("auto_lock", 3, kLockChoices - 1);
        if (json_t* v = json_object_get(root, "lock_on_leave"))
            if (json_is_boolean(v)) lock_on_leave = json_is_true(v);
        if (json_t* v = json_object_get(root, "spoiler_reset")) {
            if (json_is_integer(v) && json_integer_value(v) >= 0 && json_integer_value(v) < kSpoilerResetChoices)
                spoiler_reset = (int)json_integer_value(v);
        } else {
            // Preserve choices made by the earlier two-switch UI, then save only the new mode.
            bool chat = true, app = true;
            if (json_t* v = json_object_get(root, "spoiler_hide_chat"))
                if (json_is_boolean(v)) chat = json_is_true(v);
            if (json_t* v = json_object_get(root, "spoiler_hide_app"))
                if (json_is_boolean(v)) app = json_is_true(v);
            spoiler_reset = (chat ? 1 : 0) | (app ? 2 : 0);
        }
        if (json_t* v = json_object_get(root, "recording_shortcuts"))
            if (json_is_boolean(v)) recording_shortcuts = json_is_true(v);
        if (json_t* v = json_object_get(root, "allow_vpk_install"))
            if (json_is_boolean(v)) allow_vpk_install = json_is_true(v);
        if (json_t* v = json_object_get(root, "user_colors"))
            if (json_is_boolean(v)) user_colors = json_is_true(v);
        show_threads = json_is_true(json_object_get(root, "show_threads"));
        if (json_t* v = json_object_get(root, "check_updates")) check_updates = json_is_true(v);
        if (json_t* v = json_object_get(root, "gyroscope_enabled"))
            if (json_is_boolean(v)) gyroscope_enabled = json_is_true(v);
        if (json_t* v = json_object_get(root, "show_archive"))
            if (json_is_boolean(v)) show_archive = json_is_true(v);
        if (json_t* v = json_object_get(root, "log_mask"))
            if (json_is_integer(v)) log_mask = (unsigned)json_integer_value(v) & diag_log::kAll;
        call_low_latency = json_is_true(json_object_get(root, "call_low_latency"));
        proxy_enabled = json_is_true(json_object_get(root, "proxy_enabled"));
        proxy_port = value("proxy_port", 443, 65535);
        proxy_type = value("proxy_type", 0, 3);
        auto string_value = [&](const char* key) {
            const char* v = json_string_value(json_object_get(root, key));
            return std::string(v ? v : "");
        };
        proxy_server = string_value("proxy_server");
        // Secrets are encrypted at rest; plain-text values from older versions are re-saved encrypted.
        bool migrate = false;
        auto secret_value = [&](const char* key) {
            std::string stored = string_value(key), plain;
            secure_store::reveal(stored, plain); // Undecryptable value (other console): forget it.
            if (!stored.empty() && !secure_store::is_protected(stored) && secure_store::available()) migrate = true;
            return plain;
        };
        proxy_secret = secret_value("proxy_secret");
        proxy_username = secret_value("proxy_username");
        proxy_password = secret_value("proxy_password");
        proxy_auto_switch=!json_is_false(json_object_get(root,"proxy_auto_switch"));
        proxy_switch_seconds=ProxyPool::SwitchSeconds[ProxyPool::switchIndex(
            json_integer_value(json_object_get(root,"proxy_switch_seconds")))];
        if (auto* list=json_object_get(root,"proxies");json_is_array(list)) {
            const auto count=std::min(json_array_size(list),ProxyPool::Limit);
            for (size_t i=0;i<count;++i) {
                auto* item=json_array_get(list,i);ProxyConfig c;
                auto field=[&](const char* key,size_t limit,bool protected_value) {
                    const auto* v=json_object_get(item,key);
                    const char* text=json_string_value(v);
                    if (!text || std::strlen(text)!=json_string_length(v) || json_string_length(v)>limit*4+128) return std::string{};
                    std::string result=text;
                    if (protected_value) {
                        std::string plain;secure_store::reveal(result,plain);
                        if (!result.empty() && !secure_store::is_protected(result) && secure_store::available()) migrate=true;
                        result=std::move(plain);
                    }
                    return result.size()<=limit?result:std::string{};
                };
                const auto type=json_integer_value(json_object_get(item,"type"));
                c.kind=proxy_kind_from_int(type>=0 && type<=3?int(type):0);
                c.server=field("server",384,false);c.secret=field("secret",1023,true);
                c.username=field("username",255,true);c.password=field("password",255,true);
                const auto port=json_integer_value(json_object_get(item,"port"));
                c.port=c.kind==ProxyKind::Web?443:port>=1 && port<=65535?int(port):0;
                proxies.push_back(std::move(c));
            }
            selectProxy(proxies.empty()?-1:value("active_proxy",0,int(proxies.size())-1));
        } else if (!proxy_server.empty() || !proxy_secret.empty() || !proxy_username.empty() || !proxy_password.empty()) { proxies.push_back(proxyConfig());active_proxy=0; }
        language = string_value("language");
        font_file = string_value("font_file");
        if (!user_fonts::valid_filename(font_file)) font_file.clear();
        if (json_t* v = json_object_get(root, "download_ask")) download_ask = json_is_true(v);
        download_last_dir = string_value("download_last_dir");
        std::string saved_download_dir = string_value("download_dir");
        if (!saved_download_dir.empty() && saved_download_dir != "ux0:download/vitagram" &&
            saved_download_dir != "ux0:/download/vitagram") download_dir = saved_download_dir;
        json_decref(root);
        saveThemeHint();
        if (migrate) save();
    }
    bool save() const {
        bool secrets_ok=true;
        auto protect=[&](const std::string& plain) {
            auto stored=secure_store::protect(plain);
            if (!plain.empty() && stored.empty()) secrets_ok=false;
            return stored;
        };
        json_t* root = json_pack("{s:i,s:i,s:i,s:i,s:i,s:b}", "retention", retention, "capacity", capacity,
            "theme", ui_themes::custom(theme) ? 1 : ui_themes::saved_id(theme), "font_size", font, "request_batch_size", request_batch_size, "show_threads", show_threads);
        if (const auto* custom = ui_themes::custom(theme))
            json_object_set_new(root, "theme_file", json_string(custom->filename.c_str()));
        json_object_set_new(root, "gyroscope_enabled", json_boolean(gyroscope_enabled));
        json_object_set_new(root, "user_colors", json_boolean(user_colors));
        json_object_set_new(root, "show_archive", json_boolean(show_archive));
        json_object_set_new(root, "opacity", json_integer(opacity));
        json_object_set_new(root, "avatar_shape", json_integer(avatar_shape));
        json_object_set_new(root, "chat_open_position", json_integer(chat_open_position));
        json_object_set_new(root, "combined_view", json_boolean(combined_view));
        json_object_set_new(root, "unread_counter", json_boolean(unread_counter));
        json_object_set_new(root, "hide_muted_unread", json_boolean(hide_muted_unread));
        json_object_set_new(root, "video_quality", json_integer(video_quality));
        json_object_set_new(root, "scroll_speed", json_integer(scroll_speed));
        json_object_set_new(root, "keyboard_split", json_integer(keyboard_split));
        json_object_set_new(root, "custom_keyboard", json_boolean(custom_keyboard));
        json_t* languages = json_array();
        for (const auto& code : keyboard_languages::normalize(keyboard_languages))
            json_array_append_new(languages, json_string(code.c_str()));
        json_object_set_new(root, "keyboard_languages", languages);
        json_object_set_new(root, "log_mask", json_integer(log_mask));
        json_object_set_new(root, "spoiler_reset", json_integer(spoiler_reset));
        json_object_set_new(root, "recording_shortcuts", json_boolean(recording_shortcuts));
        json_object_set_new(root, "allow_vpk_install", json_boolean(allow_vpk_install));
        json_object_set_new(root, "check_updates", json_boolean(check_updates));
        json_object_set_new(root, "auto_lock", json_integer(auto_lock));
        json_object_set_new(root, "lock_on_leave", json_boolean(lock_on_leave));
        json_object_set_new(root, "call_low_latency", json_boolean(call_low_latency));
        json_object_set_new(root, "proxy_enabled", json_boolean(proxy_enabled));
        json_object_set_new(root, "proxy_server", json_string(proxy_server.c_str()));
        json_object_set_new(root, "proxy_port", json_integer(proxy_port));
        json_object_set_new(root, "proxy_secret", json_string(protect(proxy_secret).c_str()));
        json_object_set_new(root, "proxy_type", json_integer(proxy_type));
        json_object_set_new(root, "proxy_username", json_string(protect(proxy_username).c_str()));
        json_object_set_new(root, "proxy_password", json_string(protect(proxy_password).c_str()));
        json_t* proxy_list=json_array();
        const bool legacy=proxies.empty() && (!proxy_server.empty() || !proxy_secret.empty() || !proxy_username.empty() || !proxy_password.empty());
        const auto saved_proxies=legacy?std::vector<ProxyConfig>{proxyConfig()}:proxies;
        for (size_t i=0;i<std::min(saved_proxies.size(),ProxyPool::Limit);++i) {
            const auto& c=saved_proxies[i];
            json_array_append_new(proxy_list,json_pack("{s:i,s:s,s:i,s:s,s:s,s:s}","type",int(c.kind),"server",c.server.c_str(),
                "port",c.port,"secret",protect(c.secret).c_str(),
                "username",protect(c.username).c_str(),"password",protect(c.password).c_str()));
        }
        json_object_set_new(root,"proxies",proxy_list);
        json_object_set_new(root,"active_proxy",json_integer(legacy?0:active_proxy));
        json_object_set_new(root,"proxy_auto_switch",json_boolean(proxy_auto_switch));
        json_object_set_new(root,"proxy_switch_seconds",json_integer(proxy_switch_seconds));
        json_object_set_new(root, "cache_storage", json_integer(cache_storage));
        json_object_set_new(root, "download_ask", json_boolean(download_ask));
        json_object_set_new(root, "download_last_dir", json_string(download_last_dir.c_str()));
        json_object_set_new(root, "download_dir", json_string(download_dir.c_str()));
        json_object_set_new(root, "language", json_string(language.c_str()));
        json_object_set_new(root, "font_file", json_string(font_file.c_str()));
        if (!secrets_ok) { json_decref(root);return false; }
        saveThemeHint();
        char* dumped = json_dumps(root, JSON_COMPACT);
        json_decref(root);
        if (!dumped) return false;
        bool ok = vault::write_file_atomic("ux0:data/vitagram/settings.json", dumped);
        std::free(dumped);
        if (ok) vault::note_settings_saved(); // sealed now: from here on a missing file is noticed
        return ok;
    }
};

// After load() with the vault on: the proxy policy cannot be confirmed - settings.json was refused,
// or it should exist (saved under this vault) but could not be read, or the vault state itself is
// unsigned. The network must then wait for the user to save the proxy settings (F14).
inline bool proxy_policy_unknown(const AppSettings& settings) {
    return vault::settings_must_be_sealed() && !settings.loaded &&
           (settings.rejected || vault::settings_expected() || vault::state_unauthenticated());
}

