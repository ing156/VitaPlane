#include "web_proxy_protocol.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace web_proxy;
    Address a; std::vector<uint8_t> bytes; bool marked=false;
    assert(address("PROXY.Example.com",a) && a.host=="proxy.example.com" && a.path.empty());
    assert(secret("000102030405060708090a0b0c0d0e0f",bytes,marked) && !marked);
    assert(capability(a,bytes)=="MHLEY5PmW1GWqJkSrlmJpvJUiLhBH_QKy6yKg8a0JPk");
    assert(secret("AAECAwQFBgcICQoLDA0ODw",bytes,marked));
    assert(mtproxy_secret(bytes)=="000102030405060708090a0b0c0d0e0f");
    assert(secret("dd000102030405060708090a0b0c0d0e0f",bytes,marked));
    assert(capability(a,bytes)=="IpJrt3e7sKtzPyoXy6w-Zj6GGEvsvclN66JzQEfPYLA");
    assert(address("proxy.example.com/dobry-cola-super-app",a));
    assert(capability(a,bytes)=="TGUkZaevsavLbHvlNWipnRoYxgzZ51ioWvbxgGT3wHo");
    assert(secret("cN0AAQIDBAUGBwgJCgsMDQ4P",bytes,marked) && marked);
    assert(mtproxy_secret(bytes)=="dd000102030405060708090a0b0c0d0e0f");
    assert(secret("cAABAgMEBQYHCAkKCwwNDg8",bytes,marked) && marked);
    assert(capability(a,bytes)=="hHz99Xs93EN1j91G9gpNepXwGNNt5YdAFkEVk_LlqdQ");
    assert(address("xn--e1afmkfd.xn--p1ai/AbC/Def_1",a) && a.path=="AbC/Def_1");
    for (const auto* invalid : {"", "https://proxy.example", "proxy.example:443", "127.0.0.1", "proxy", ".proxy.example", "proxy.example.", "-a.example", "a-.example", "a..example", "proxy.example/", "proxy.example/a/", "proxy.example/a//b", "proxy.example/../b", "proxy.example/a%2fb", "proxy.example/_a", "a@proxy.example", "proxy.example/a?b", "proxy.example/a#b"})
        assert(!address(invalid,a));
    for (const auto* invalid : {"", "a", "ee000102030405060708090a0b0c0d0e0f", "AAECAwQFBgcICQoLDA0ODx", "!!!!!!!!!!!!!!!!!!!!!!", "AAAAAAAAAAAAAAAAAAAAAA===="})
        assert(!secret(invalid,bytes,marked));
    std::string token;
    assert(bootstrap("const relayBase=\"/\",bootstrap=\""+std::string(43,'A')+"\",carrierMode=\"https\"",token));
    assert(!bootstrap("bootstrap=\"evil\"",token));
    assert(!bootstrap("bootstrap=\""+std::string(43,'A')+"\";bootstrap=\""+std::string(43,'B')+"\"",token));
    std::vector<Frame> out;
    const auto body=frame(1,1)+frame(2,1,std::string("a\0b",3))+frame(4,1,number(123))+frame(3,1);
    assert(frames(body,out) && out.size()==4 && out[1].payload==std::string("a\0b",3));
    for (const auto& invalid : {std::string(),body.substr(0,body.size()-1),frame(2,0,"data"),frame(2,1),frame(4,1,number(uint32_t{0})),frame(0x10,0,"bad"),frame(0x22,0),frame(3,1,"x"),frame(1,0)})
        assert(!frames(invalid,out));
    assert(!frames(std::string(MaxBatch+1,'\0'),out));
    std::cout << "WEB protocol: capability vectors, canonical hosts/paths/secrets and bounded frames PASS\n";
}
