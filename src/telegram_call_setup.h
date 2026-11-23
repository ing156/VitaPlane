#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <jansson.h>

namespace telegram_call {

struct Server {
    uint8_t reflector_id = 0;
    std::string host;
    uint16_t port = 0;
    std::string username;
    std::string password;
    bool turn = false;
};

struct Setup {
    std::array<uint8_t, 256> key{};
    std::string version;
    std::string custom_parameters;
    std::vector<Server> servers;
    int max_layer = 0;
    bool allow_p2p = false;
};

json_t* protocol();
bool parse_ready(const json_t* state, Setup& setup);

}
