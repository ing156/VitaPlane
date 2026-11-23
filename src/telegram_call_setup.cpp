#include "telegram_call_setup.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <string_view>

#include <openssl/evp.h>

namespace telegram_call {
namespace {
constexpr std::array<const char*, 2> kVersions = {"11.0.0", "10.0.0"};
constexpr size_t kMaxServers = 64;

std::string_view value(const json_t* object, const char* name) {
    const auto* property = json_object_get(object, name);
    const auto* text = json_string_value(property);
    return text ? std::string_view(text, json_string_length(property)) : std::string_view{};
}

bool flag(const json_t* object, const char* name) {
    return json_is_true(json_object_get(object, name));
}

template <size_t N>
bool read_bytes(std::string_view encoded, std::array<uint8_t, N>& bytes) {
    constexpr size_t max_encoded = (N + 2) / 3 * 4;
    if (encoded.size() != max_encoded) return false;
    constexpr size_t padding = (3 - N % 3) % 3;
    size_t actual_padding = 0;
    for (auto i = encoded.rbegin(); i != encoded.rend() && *i == '='; ++i) ++actual_padding;
    if (actual_padding != padding) return false;
    std::array<uint8_t, max_encoded / 4 * 3> decoded{};
    const int size = EVP_DecodeBlock(decoded.data(), reinterpret_cast<const uint8_t*>(encoded.data()), int(encoded.size()));
    if (size != int(N + padding)) return false;
    std::copy_n(decoded.begin(), N, bytes.begin());
    return true;
}

bool reflector_id(const json_t* server, int64_t& id) {
    const auto encoded = value(server, "id");
    if (encoded.empty()) return false;
    const auto result = std::from_chars(encoded.data(), encoded.data() + encoded.size(), id);
    return result.ec == std::errc{} && result.ptr == encoded.data() + encoded.size();
}

std::string hex(const std::array<uint8_t, 16>& bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const uint8_t byte : bytes) {
        out += digits[byte >> 4];
        out += digits[byte & 15];
    }
    return out;
}
}

json_t* protocol() {
    auto* versions = json_array();
    for (const auto* version : kVersions) json_array_append_new(versions, json_string(version));
    return json_pack("{s:s,s:b,s:b,s:i,s:i,s:o}", "@type", "callProtocol", "udp_p2p", 1,
                     "udp_reflector", 1, "min_layer", 65, "max_layer", 92, "library_versions", versions);
}

bool parse_ready(const json_t* state, Setup& setup) {
    if (value(state, "@type") != "callStateReady") return false;
    Setup ready;
    if (!read_bytes(value(state, "encryption_key"), ready.key)) return false;
    const auto* negotiated = json_object_get(state, "protocol");
    const auto* versions = json_object_get(negotiated, "library_versions");
    const auto* version = json_string_value(json_array_get(versions, 0));
    if (!version || std::find(kVersions.begin(), kVersions.end(), std::string_view(version)) == kVersions.end()) return false;
    const auto max_layer = json_integer_value(json_object_get(negotiated, "max_layer"));
    if (max_layer < 65) return false;
    ready.max_layer = int(std::min<json_int_t>(max_layer, 92));
    ready.version = version;
    size_t index;
    json_t* item;
    ready.allow_p2p = flag(state, "allow_p2p");
    const auto custom = value(state, "custom_parameters");
    if (custom.size() > 16384) return false;
    ready.custom_parameters = custom;

    const auto* servers = json_object_get(state, "servers");
    if (!json_is_array(servers) || json_array_size(servers) > kMaxServers) return false;
    std::vector<int64_t> reflector_ids;
    json_array_foreach(servers, index, item) {
        const auto* type = json_object_get(item, "type");
        if (value(type, "@type") == "callServerTypeTelegramReflector") {
            int64_t id = 0;
            if (!reflector_id(item, id)) return false;
            reflector_ids.push_back(id);
        }
    }
    std::sort(reflector_ids.begin(), reflector_ids.end());
    reflector_ids.erase(std::unique(reflector_ids.begin(), reflector_ids.end()), reflector_ids.end());
    json_array_foreach(servers, index, item) {
        const auto host = value(item, "ip_address");
        const auto raw_port = json_integer_value(json_object_get(item, "port"));
        if (host.empty() || host.size() > 253 || raw_port < 1 || raw_port > 65535) continue;
        const auto port = uint16_t(raw_port);
        const auto* type = json_object_get(item, "type");
        const auto kind = value(type, "@type");
        if (kind == "callServerTypeTelegramReflector" && !flag(type, "is_tcp")) {
            int64_t id = 0;
            std::array<uint8_t, 16> tag{};
            if (!reflector_id(item, id) || !read_bytes(value(type, "peer_tag"), tag)) return false;
            const auto position = std::lower_bound(reflector_ids.begin(), reflector_ids.end(), id) - reflector_ids.begin();
            ready.servers.push_back({uint8_t(position + 1), std::string(host), port, "reflector", hex(tag), true});
        } else if (kind == "callServerTypeWebrtc") {
            const auto user = value(type, "username"), password = value(type, "password");
            if (flag(type, "supports_stun")) ready.servers.push_back({0, std::string(host), port, {}, {}, false});
            if (flag(type, "supports_turn") && !user.empty() && !password.empty() && user.size() <= 256 && password.size() <= 256) {
                ready.servers.push_back({0, std::string(host), port, std::string(user), std::string(password), true});
            }
        }
    }
    if (!ready.allow_p2p && std::none_of(ready.servers.begin(), ready.servers.end(), [](const Server& server) { return server.turn; })) return false;
    setup = std::move(ready);
    return true;
}

}
