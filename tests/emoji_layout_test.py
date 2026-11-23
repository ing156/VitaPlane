"""Exercise production message clipping/wrapping with complete emoji clusters."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()


def function(signature, text=source):
    start = text.index(signature)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


fixture = r'''
#include "emoji.h"
#include "custom_emoji.h"
#include "inline_emoji_players.h"
#include <cassert>
#include <sstream>
#include <string>
#include <vector>
struct vita2d_pgf {};
struct LinkSpan { int begin,end; std::string url; };
struct LineLink { int begin,end; std::string url; };
struct MessageItem { std::string text; std::vector<CustomEmojiSpan> custom_emojis; };
namespace performance {
enum class SlowPhase { Ui };
struct SlowScope { SlowScope(SlowPhase,const char*) {} };
}
unsigned rich_text_revision() { return 1; }
int rich_text_width(vita2d_pgf*,float,const char* text) {
    const std::string value(text);
    int width=0;
    for (size_t i=0;i<value.size();i=emoji::next(value,i)) width+=20;
    return width;
}
'''
fixture += r'''
struct vita2d_texture {};
struct AnimationPlayer {
    AnimationPlayer(bool=true) {}
    void setSource(const std::string&) {}
    void setSuspended(bool) {}
    void upload() {}
    vita2d_texture* texture() { return nullptr; }
    bool inline_emoji_=true, suspended_=true, stopping_=false;
    unsigned generation_=7;
    int64_t waitInline(unsigned generation);
};
AnimationPlayer* waiting_player=nullptr;
int64_t pause_clock=1000;
int pause_ticks=0, pause_action=0;
int64_t sceKernelGetSystemTimeWide() { return pause_clock; }
void sceKernelDelayThread(unsigned us) {
    pause_clock+=us;
    if (++pause_ticks==3) {
        if (pause_action==0) waiting_player->suspended_=false;
        if (pause_action==1) ++waiting_player->generation_;
        if (pause_action==2) waiting_player->stopping_=true;
    }
}
'''
fixture += function('int64_t AnimationPlayer::waitInline(', (root / 'src/animation_player.cpp').read_text())
fixture += r'''
struct TelegramClient {
    bool preview=false;
    std::vector<int64_t> wanted;
    bool customEmojiAnimated(int64_t) { return true; }
    bool customEmojiHasStaticPreview(int64_t) { return preview; }
    std::string customEmojiAnimation(int64_t) { return {}; }
    std::string customReactionIcon(int64_t) { return {}; }
    void updateCustomEmojiAnimations(const std::vector<int64_t>& ids,int32_t) { wanted=ids; }
};
class TextureCache {
    InlineEmojiPlayers<AnimationPlayer> emoji_players_;
    std::vector<int64_t> emoji_assets_;
    bool emoji_paused_=true;
    static constexpr int kMaskNone=0;
    vita2d_texture* get(const std::string&,int,unsigned) { return nullptr; }
public:
'''
fixture += function('void beginEmojiFrame(')
fixture += function('void finishEmojiFrame(')
fixture += function('vita2d_texture* emojiPlayer(')
fixture += function('vita2d_texture* customEmoji(')
fixture += '};\n'
fixture += function('static std::vector<std::vector<LineLink>> map_links_to_lines(')
fixture += function('static std::vector<std::vector<CustomEmojiSpan>> map_custom_emojis(')
fixture += function('static std::string clip(')
fixture += function('static std::vector<std::string> wrap_text(')
emoji_tick = re.search(r'if \([^\n]+\) emoji_load_one\(\);', source).group()
image_tick = re.search(r'if \([^\n]+\) textures.loadOne\(\);', source).group()
fixture += r'''
int main() {
    TextureCache cache; TelegramClient telegram;
    for (bool paused : {false,true}) for (bool previews : {false,true}) {
        telegram.preview=previews; cache.beginEmojiFrame(42,paused);
        for (int i=0;i<80;++i) cache.customEmoji(telegram,1000+i);
        cache.finishEmojiFrame(telegram,0);
        assert(telegram.wanted.size()==(paused ? 0u : previews ? 4u : 80u));
    }
    telegram.preview=false; cache.beginEmojiFrame(42,false);
    for (int i=0;i<80;++i) cache.customEmoji(telegram,1000);
    cache.finishEmojiFrame(telegram,0);
    assert(telegram.wanted==std::vector<int64_t>{1000});
    for (pause_action=0;pause_action<3;++pause_action) {
        AnimationPlayer player; waiting_player=&player; pause_ticks=0;
        assert(player.waitInline(7)==30000 && pause_ticks==3);
    }
    AnimationPlayer normal; normal.inline_emoji_=false; pause_ticks=0;
    assert(normal.waitInline(7)==0 && pause_ticks==0);
    AnimationPlayer superseded; waiting_player=&superseded;
    assert(superseded.waitInline(6)==0 && pause_ticks==0);
    MessageItem message{"я😀 😀\n❤", {{2,6,999}, {7,11,777}, {12,15,555}}};
    const auto mapped=map_custom_emojis(message,{"я😀", "😀", "❤"});
    assert(mapped.size()==3 && mapped[0].size()==1 && mapped[1].size()==1 && mapped[2].size()==1);
    assert(mapped[0][0].begin==2 && mapped[0][0].end==6 && mapped[0][0].id==999);
    assert(mapped[1][0].begin==0 && mapped[1][0].end==4 && mapped[1][0].id==777);
    assert(mapped[2][0].begin==0 && mapped[2][0].end==3 && mapped[2][0].id==555);
    int decoded=0;
    struct Textures { int loaded=0; void loadOne() { ++loaded; } } textures;
    auto emoji_load_one=[&] { ++decoded; };
    for (bool interaction : {false,true}) for (bool keyboard_interaction : {false,true})
    for (bool keyboard_emoji_media : {false,true})
    for (bool keyboard_screen : {false,true}) for (int decode_cooldown : {0,8}) {
        decoded=0; textures.loaded=0;
        auto frame_tail=[&] {
''' + emoji_tick + '\n' + image_tick + r'''
        };
        for (int frame=0;frame<30;++frame) frame_tail();
        const bool allowed=keyboard_screen || (!interaction && !decode_cooldown && !keyboard_interaction);
        assert(decoded==(allowed ? 30 : 0));
        const bool images_allowed=keyboard_emoji_media ? !interaction && !decode_cooldown : !keyboard_interaction && !keyboard_screen;
        assert(textures.loaded==(images_allowed ? 30 : 0));
    }

    for (const char* glyph : {u8"👍🏽",u8"👩🏽‍💻",u8"🇷🇺",u8"1️⃣",u8"👨‍👩‍👧‍👦"}) {
        const std::string emoji(glyph);
        const auto long_word=emoji+emoji+emoji+emoji;
        const auto lines=wrap_text(nullptr,long_word,40);
        assert(lines==std::vector<std::string>({emoji+emoji,emoji+emoji}));
        for (size_t limit=4;limit<emoji.size()+3;++limit) assert(clip(long_word,limit)=="...");
        assert(clip(long_word,emoji.size()+3)==emoji+"...");
    }
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-emoji-layout-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), '-I', sys.argv[1], str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Emoji frame tail: custom keyboard decodes during held gestures; native IME and background cooldown remain protected: PASS')
print('Message emoji layout: production clipping/wrapping preserve complete sequences: PASS')
