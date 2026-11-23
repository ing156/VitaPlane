#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace web_proxy {
constexpr uint32_t Window = 4u << 20;
constexpr size_t MaxBatch = 2u << 20;
struct Address { std::string host, path; };
bool address(const std::string& text, Address& out);
// Share links mark path-aware secrets with 0x70. The marker is not an MTProxy mode byte.
bool secret(const std::string& text, std::vector<uint8_t>& bytes, bool& marked);
std::string base64url(const unsigned char* data, size_t size);
std::string capability(const Address& address, const std::vector<uint8_t>& secret);
std::string mtproxy_secret(const std::vector<uint8_t>& bytes);
bool bootstrap(const std::string& html, std::string& token);
bool bearer(const std::string& token);
struct Frame { uint8_t type; uint32_t stream; std::string payload; };
std::string frame(uint8_t type, uint32_t stream, const std::string& payload = {});
bool frames(const std::string& batch, std::vector<Frame>& out);
uint32_t number(const char* p);
std::string number(uint32_t value);
}
