#include "inline_emoji_players.h"
#include <cassert>
#include <cstdio>

struct Player {
    static int live, sources, uploads;
    std::string path;
    int frame=0;
    bool suspended=false;
    explicit Player(bool small) { assert(small); ++live; }
    ~Player() { --live; }
    void setSource(const std::string& value) { path=value; ++sources; }
    void setSuspended(bool value) { suspended=value; }
    void upload() { assert(!path.empty() && !suspended); ++frame; ++uploads; }
    int* texture() { return path.empty() ? nullptr : &frame; }
};
int Player::live=0, Player::sources=0, Player::uploads=0;
int main() {
    InlineEmojiPlayers<Player> pool;
    pool.beginFrame(42,false);
    assert(pool.request(1,"a.tgs") && !pool.texture(1) && Player::sources==0);
    assert(pool.request(1,"a.tgs")); // Repeated instances share one decoder.
    for (int id=2;id<=4;++id) assert(pool.request(id,"b.webm"));
    assert(!pool.request(5,"c.tgs"));
    pool.finishFrame(); assert(Player::live==4 && Player::sources==4 && Player::uploads==4);
    pool.beginFrame(42,false); assert(pool.request(1,"a.tgs") && pool.texture(1));
    pool.finishFrame(); assert(Player::sources==7 && Player::uploads==5); // Offscreen slots cancel.
    pool.beginFrame(42,false); assert(pool.request(1,"new.tgs") && !pool.texture(1));
    pool.finishFrame(); assert(pool.texture(1) && Player::sources==9); // Never show the previous path.
    pool.beginFrame(43,false); assert(pool.request(1,"new.tgs") && !pool.texture(1));
    pool.finishFrame(); assert(pool.texture(1)); // Chat identity also scopes textures.
    const int before_pause=Player::sources;
    const int paused_frame=*pool.texture(1);
    pool.beginFrame(43,true); assert(pool.request(1,"new.tgs") && *pool.texture(1)==paused_frame);
    assert(pool.wantedIds().empty()); pool.finishFrame();
    assert(Player::sources==before_pause && *pool.texture(1)==paused_frame);
    pool.beginFrame(43,false); assert(pool.request(1,"new.tgs")); pool.finishFrame();
    assert(Player::sources==before_pause && *pool.texture(1)>paused_frame);
    for (int frame=0;frame<120;++frame) {
        pool.beginFrame(43,false); assert(pool.request(1,"new.tgs")); pool.finishFrame();
    }
    assert(Player::sources==before_pause && *pool.texture(1)>=paused_frame+121);
    pool.beginFrame(43,false); assert(pool.request(10,"") && !pool.texture(10));
    pool.finishFrame(); assert(!pool.texture(10)); // Wait for a download without starting a decoder.
    pool.beginFrame(43,false); assert(pool.request(10,"loaded.webm")); pool.finishFrame();
    assert(pool.texture(10));
    assert(pool.request(10,"replaced.webm") && !pool.texture(10));
    pool.finishFrame(); assert(pool.texture(10));
    pool.clear(); assert(!pool.texture(10));
    pool.beginFrame(0,false); assert(!pool.request(10,"loaded.webm")); pool.finishFrame();
    pool.shutdown(); assert(Player::live==0);
    std::puts("Inline emoji: fixed slots, shared instances, deferred GPU work, cancellation, paths and chat scoping passed");
}
