#include "telegram_call_setup.h"

#include <cassert>
#include <cstring>
#include <memory>

namespace {
void release(json_t* value) { json_decref(value); }
using Json = std::unique_ptr<json_t, decltype(&release)>;

json_t* reflector(const char* id, bool tcp) {
    return json_pack("{s:s,s:s,s:i,s:{s:s,s:s,s:b}}",
        "id", id, "ip_address", "192.0.2.1", "port", 443,
        "type", "@type", "callServerTypeTelegramReflector", "peer_tag", "AAECAwQFBgcICQoLDA0ODw==", "is_tcp", tcp);
}
}

int main() {
    const Json protocol(telegram_call::protocol(), release);
    assert(json_integer_value(json_object_get(protocol.get(), "max_layer")) == 92);
    const auto* versions = json_object_get(protocol.get(), "library_versions");
    assert(json_array_size(versions) == 2);
    assert(std::strcmp(json_string_value(json_array_get(versions, 0)), "11.0.0") == 0);

    const std::string key = std::string(342, 'A') + "==";
    const Json state(json_pack("{s:s,s:s,s:o,s:b,s:[]}", "@type", "callStateReady",
        "encryption_key", key.c_str(), "protocol", json_incref(protocol.get()), "allow_p2p", 0, "servers"), release);
    auto* servers = json_object_get(state.get(), "servers");
    json_array_append_new(servers, reflector("9007199254740995", false));
    json_array_append_new(servers, reflector("9007199254740993", true));
    json_array_append_new(servers, json_pack("{s:s,s:i,s:{s:s,s:b,s:b,s:s,s:s}}", "ip_address", "192.0.2.2",
        "port", 3478, "type", "@type", "callServerTypeWebrtc", "supports_stun", 1, "supports_turn", 1,
        "username", "test", "password", "test"));

    telegram_call::Setup setup;
    assert(telegram_call::parse_ready(state.get(), setup));
    assert(!setup.allow_p2p && setup.version == "11.0.0" && setup.max_layer == 92 && setup.servers.size() == 3);
    assert(setup.servers[0].reflector_id == 2 && setup.servers[0].turn);
    assert(setup.servers[0].password == "000102030405060708090a0b0c0d0e0f");
    assert(!setup.servers[1].turn && setup.servers[2].turn);

    json_object_set_new(state.get(), "encryption_key", json_string("bad"));
    assert(!telegram_call::parse_ready(state.get(), setup));
    json_object_set_new(state.get(), "encryption_key", json_string(key.c_str()));
    json_array_clear(servers);
    assert(!telegram_call::parse_ready(state.get(), setup));
    json_object_set_new(state.get(), "allow_p2p", json_true());
    assert(telegram_call::parse_ready(state.get(), setup));
    json_object_set_new(state.get(), "protocol", json_pack("{s:[s,s],s:i}", "library_versions", "unsupported", "11.0.0", "max_layer", 92));
    assert(!telegram_call::parse_ready(state.get(), setup));
}
