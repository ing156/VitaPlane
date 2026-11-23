#include "settings.h"
#include <cassert>
#include <iostream>

static int writes=0;
namespace secure_store {
// Simulate a failed credential encryption operation; no account keys are needed.
std::string protect(const std::string&) { return {}; }
}
namespace vault {
bool write_file_atomic(const std::string&,const std::string&) { ++writes;return true; }
void note_settings_saved() {}
}
int main() {
    AppSettings s;s.proxy_server="proxy.example";s.proxy_secret="test-secret";
    assert(!s.save() && writes==0);
    s.proxy_secret.clear();s.proxy_type=1;s.proxy_password="test-password";
    assert(!s.save() && writes==0);
    s.proxy_password.clear();assert(s.save() && writes==1);
    std::cout<<"Proxy secrets: failed encryption cannot overwrite saved credentials PASS\n";
}
