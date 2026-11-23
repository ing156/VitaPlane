#include "web_proxy_protocol.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <algorithm>
#include <cctype>

namespace web_proxy {
namespace {
int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool alnum(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
}
bool address(const std::string& text, Address& out) {
    Address a;
    const auto slash = text.find('/');
    a.host = text.substr(0, slash);
    if (a.host.size() > 253 || a.host.find('.') == std::string::npos) return false;
    bool letter = false;
    size_t label = 0;
    for (size_t i = 0; i <= a.host.size(); ++i) {
        if (i == a.host.size() || a.host[i] == '.') {
            if (!label || label > 63 || a.host[i-label] == '-' || a.host[i-1] == '-') return false;
            label = 0;
        } else {
            unsigned char c = a.host[i];
            if (!alnum(c) && c != '-') return false;
            if (std::isalpha(c)) letter = true;
            a.host[i] = std::tolower(c); ++label;
        }
    }
    if (!letter) return false; // DNS names only; IP literals are not WEB proxy hosts.
    if (slash != std::string::npos) {
        a.path = text.substr(slash+1);
        if (a.path.empty() || a.path.size() > 128) return false;
        bool first = true;
        for (unsigned char c : a.path) {
            if (c == '/') { if (first) return false; first = true; }
            else { if (!alnum(c) && (first || (c != '-' && c != '_'))) return false; first = false; }
        }
        if (first) return false;
    }
    out = std::move(a); return true;
}
std::string base64url(const unsigned char* data, size_t size) {
    std::string result(4*((size+2)/3), '\0');
    if (size) EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()), data, int(size));
    while (!result.empty() && result.back() == '=') result.pop_back();
    for (auto& c : result) { if (c == '+') c = '-'; else if (c == '/') c = '_'; }
    return result;
}
bool secret(const std::string& text, std::vector<uint8_t>& out, bool& marked) {
    std::vector<uint8_t> bytes;
    if ((text.size() == 32 || text.size() == 34) && std::all_of(text.begin(), text.end(), [](char c) { return hex(c) >= 0; })) {
        for (size_t i=0; i<text.size(); i+=2) bytes.push_back(hex(text[i])*16+hex(text[i+1]));
    } else {
        if (text.size() < 22 || text.size() > 24) return false;
        if (text.find('=')!=std::string::npos && text.size()%4) return false;
        std::string encoded = text;
        while (!encoded.empty() && encoded.back() == '=') encoded.pop_back();
        if (encoded.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") != std::string::npos) return false;
        const std::string canonical = encoded;
        for (auto& c : encoded) { if (c == '-') c = '+'; else if (c == '_') c = '/'; }
        while (encoded.size()%4) encoded += '=';
        bytes.resize(encoded.size()/4*3);
        int size = EVP_DecodeBlock(bytes.data(), reinterpret_cast<const unsigned char*>(encoded.data()), encoded.size());
        if (size < 0) return false;
        for (auto it=encoded.rbegin(); it!=encoded.rend() && *it=='='; ++it) --size;
        bytes.resize(size);
        if (base64url(bytes.data(),bytes.size()) != canonical) return false;
    }
    marked = (bytes.size() == 17 || bytes.size() == 18) && bytes[0] == 0x70;
    if (marked) bytes.erase(bytes.begin());
    if (bytes.size() != 16 && !(bytes.size() == 17 && bytes[0] == 0xdd)) return false;
    out = std::move(bytes); return true;
}
std::string mtproxy_secret(const std::vector<uint8_t>& bytes) {
    std::string text;
    for (auto c : bytes) { text += "0123456789abcdef"[c>>4]; text += "0123456789abcdef"[c&15]; }
    return text;
}
std::string capability(const Address& a, const std::vector<uint8_t>& bytes) {
    const auto context = a.path.empty() ? "tdesktop-web-proxy-bridge-v1\n" + a.host :
        "tdesktop-web-proxy-bridge-v2\n" + a.host + "\n" + a.path;
    unsigned char digest[EVP_MAX_MD_SIZE]; unsigned length = 0;
    if (!HMAC(EVP_sha256(), bytes.data(), bytes.size(), reinterpret_cast<const unsigned char*>(context.data()), context.size(), digest, &length)) return {};
    return base64url(digest, length);
}
bool bearer(const std::string& text) {
    return text.size()==43 && text.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")==std::string::npos;
}
bool bootstrap(const std::string& html, std::string& token) {
    // shortcut: parse the reference relay's literal bootstrap, update if its bridge template changes.
    const std::string marker = "bootstrap=\"";
    auto start = html.find(marker);
    if (start == std::string::npos || html.find(marker,start+marker.size()) != std::string::npos) return false;
    start += marker.size();
    const auto end = html.find('"',start);
    if (end == std::string::npos || !bearer(html.substr(start,end-start))) return false;
    token = html.substr(start,end-start); return true;
}
uint32_t number(const char* p) {
    const auto* b = reinterpret_cast<const unsigned char*>(p);
    return (uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3];
}
std::string number(uint32_t n) {
    std::string s(4,'\0'); for (int i=3;i>=0;--i) { s[i]=n&255; n>>=8; } return s;
}
std::string frame(uint8_t type, uint32_t id, const std::string& payload) {
    std::string s{char(type),char(id>>16),char(id>>8),char(id)};
    return s+number(payload.size())+payload;
}
bool frames(const std::string& batch, std::vector<Frame>& out) {
    if (batch.empty() || batch.size() > MaxBatch) return false;
    std::vector<Frame> decoded;
    for (size_t at=0; at<batch.size();) {
        if (batch.size()-at<8 || decoded.size()>=4096) return false;
        const auto* p = reinterpret_cast<const unsigned char*>(batch.data()+at);
        const uint32_t id=(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3], size=number(batch.data()+at+4);
        if (size>(1u<<20) || size>batch.size()-at-8) return false;
        const uint8_t t=p[0];
        const bool valid = (t==1 || t==3) ? id && !size : t==2 ? id && size :
            t==4 ? id && size==4 && number(batch.data()+at+8)>0 :
            t==5 || t==6 ? !id && size<=64 : t==0x10 ? !id && size==1 && p[8]==1 :
            t==0x11 ? !id && !size : t==0x1f ? !id && size<=256 : false;
        if (!valid) return false;
        decoded.push_back({t,id,batch.substr(at+8,size)}); at+=8+size;
    }
    out = std::move(decoded); return true;
}
}
