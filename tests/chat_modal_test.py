"""Compile the production visibility predicates and exercise modal transitions."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
names = ('tool_window_open', 'chat_modal_open', 'chat_ui_visible', 'tool_menu_visible', 'combined_visible')
predicates = []
for name in names:
    start = main.index('    auto ' + name + ' = [&]() {')
    predicates.append(main[start:main.index('    };', start) + 6])
fixture = r'''
#include "chat_view_layout.h"
#include "audio_call_dialog.h"
#include "history_scroll.h"
#include <cassert>
#include <string>
#include <iostream>
#include <vector>
struct Active { bool on = false; bool active() const { return on; } bool busy() const { return on; } };
struct TelegramClient { struct BotWebApp { enum { Closed, Confirm, Loading, Open }; int phase = Closed; }; };
struct Client {
    struct Message { int64_t id; bool outgoing; };
    std::vector<Message> message_rows;
    bool bot_follow = false, bot_focus = false;
    const auto& messages() const { return message_rows; }
    bool followingBotReplies() const { return bot_follow; }
    bool takeBotReplyFocus() { bool focus = bot_focus; bot_focus = false; return focus; }
    struct Folder { int id; std::string title; };
    std::vector<Folder> items;
    int selected = 0;
    const auto& folders() const { return items; }
    int selectedFolder() const { return selected; }
    bool browse = true;
    int chat = 1;
    TelegramClient::BotWebApp app;
    bool canBrowse() const { return browse; }
    bool topicsOpen() const { return false; }
    int currentChatId() const { return chat; }
    auto& botWebApp() { return app; }
} tg;
int main() {
    struct { bool combined_view = true; } settings;
    bool settings_open = false, photo_open = false, video_fullscreen = false;
    bool search_open = false, stickers_open = false, gifs_open = false, reactions_open = false;
    bool seek_open = false, seek_touch_release = false, unsupported_video_prompt = false;
    bool spoiler_prompt = false, bot_panel = false, confirm_leave = false, calendar_open = false;
    int call_chat = 0;
    std::string pending_web_link;
    struct { bool active = false; } folder_picker;
    struct { bool expanded = false; } downloads_panel;
    Active upload_picker, circle_recorder, recorder, kb, qr_scanner, proxy_save, compose_wheel;
'''
fixture += '\n'.join(predicates)
start = main.index('        if (combined_visible() && !post_open) {')
end = main.index('        // Separate title,', start)
fixture += r'''
    bool post_open = false, combined_list_focus = true;
    struct { bool open = false; } folder_drawer;
    auto tr = [](const char* value) { return std::string(value); };
    auto header_presence = [&]() {
        std::string presence = "user status";
'''
fixture += main[start:end]
fixture += 'return presence; };\n'

fixture += r'''
    bool* events[] = {&search_open, &stickers_open, &gifs_open, &reactions_open,
        &upload_picker.on, &circle_recorder.on, &recorder.on, &kb.on, &qr_scanner.on, &proxy_save.on,
        &seek_open, &seek_touch_release, &unsupported_video_prompt, &spoiler_prompt,
        &bot_panel, &confirm_leave, &compose_wheel.on, &calendar_open, &folder_picker.active, &downloads_panel.expanded};
    const auto base = chat_view::layout(true, true);
    int transitions = 0;
    for (bool combined : {false, true}) for (int chat : {0, 1}) {
        settings.combined_view = combined; tg.chat = chat;
        auto check = [&] {
            assert(chat_modal_open());
            assert(!chat_ui_visible());
            assert(combined_visible() == combined);
            const auto layout = chat_view::layout(combined_visible(), true);
            assert(layout.left() == (combined ? base.left() : 0));
            assert(layout.bubbleWidth() == (combined ? base.bubbleWidth() : 712));
            ++transitions;
        };
        for (auto event : events) {
            *event = true; check();
            // Nested tools, e.g. GIF search/caption keyboard, must keep the same pane.
            for (auto nested : events) if (nested != event) {
                *nested = true; check(); *nested = false; check();
            }
            *event = false;
            assert(!chat_modal_open()); assert(chat_ui_visible());
        }
        call_chat = 42; check(); call_chat = 0;
        for (int phase : {1, 2, 3}) { tg.app.phase = phase; check(); }
        tg.app.phase = 0;
        pending_web_link = "https://example.com"; check(); pending_web_link.clear();
        recorder.on = true; assert(tool_menu_visible());
        kb.on = true; assert(!tool_menu_visible()); kb.on = recorder.on = false;
        upload_picker.on = true; assert(!tool_menu_visible()); upload_picker.on = false;
        for (bool* fullscreen : {&settings_open, &photo_open, &video_fullscreen}) {
            *fullscreen = true; assert(!combined_visible()); *fullscreen = false;
        }
        tg.browse = false; assert(!combined_visible()); tg.browse = true;
    }
    settings.combined_view = true;
    for (bool focus : {false, true}) {
        combined_list_focus = focus;
        const std::string hint = focus ? "chat.focus_list" : "chat.focus_history";
        assert(header_presence() == hint); // No folders loaded yet.
        tg.items = {{0, "Все чаты"}, {4, "Работа"}, {-1, "Архив"}, {9, std::string(200, 'X')}};
        for (int selected : {0, 4, -1, 9, 0, -1, 4}) {
            tg.selected = selected;
            for (bool open : {false, true, false}) {
                folder_drawer.open = open;
                const auto folder = std::find_if(tg.items.begin(), tg.items.end(),
                    [&](const auto& item) { return item.id == selected; });
                assert(header_presence() == (open ? hint : folder->title));
            }
        }
        tg.selected = 123; assert(header_presence() == hint);
        tg.items.clear(); tg.selected = 0;
    }
    tg.items = {{0, "Все чаты"}};
    post_open = true; assert(header_presence() == "user status"); post_open = false;
    settings.combined_view = false; assert(header_presence() == "user status");
    settings.combined_view = true;
    settings_open = true; assert(header_presence() == "user status"); settings_open = false;
    std::cout << "Header folder name: switching, wraparound, archive, long UTF-8 names, drawer and focus: PASS\n";
    {
        using namespace audio_call_dialog;
        constexpr Layout confirm{Stage::Confirm}, incoming{Stage::Incoming}, active{Stage::Active}, done{Stage::Done};
        static_assert(confirm.action(0) == Action::Cancel && confirm.action(1) == Action::Call);
        static_assert(active.action(0) == Action::Mute && active.action(1) == Action::HangUp);
        static_assert(done.action(0) == Action::Close && done.action(1) == Action::None);
        static_assert(incoming.action(0) == Action::Decline && incoming.action(1) == Action::Accept);
        constexpr Card call{30, 70, 330, 64};
        static_assert(call.hit(30, 70) && call.hit(359, 133));
        static_assert(!call.hit(29, 70) && !call.hit(360, 133) && !call.hit(30, 134));
        for (const auto layout : {confirm, incoming, active, done}) {
            assert(layout.x(0) + layout.x(layout.count() - 1) + Layout::button_width == 2 * Layout::center_x);
            assert(layout.x(0) > Layout::left && layout.x(layout.count() - 1) + Layout::button_width < Layout::left + Layout::width);
            int pixels = 0;
            for (int y = 0; y < 544; ++y) for (int x = 0; x < 960; ++x) {
                const int hit = layout.hit(x, y);
                if (hit < 0) continue;
                assert(layout.action(hit) != Action::None);
                assert(x >= layout.x(hit) && x < layout.x(hit) + Layout::button_width);
                assert(y >= Layout::button_y && y < Layout::button_y + Layout::button_height);
                ++pixels;
            }
            assert(pixels == layout.count() * Layout::button_width * Layout::button_height);
        }
        std::cout << "Call dialog: modal visibility, actions, centering and touch boundaries: PASS\n";
    }
    using W = chat_view::ToolWindow;
    for (int y = 0; y <= 544; ++y) for (int x = 0; x <= 960; ++x)
        assert(W::backHit(x,y) == (x >= 816 && x < 944 && y >= 38 && y < 64));
    assert(W::left + W::width == 944);
    std::cout << "Modal events/nesting: " << transitions << " transitions; return hit geometry: PASS\n";
'''
start = main.index('        if (was_at_bottom && !post_open && !photo_open')
end = main.index('        if (const int64_t target = tg.takeMessageJump())', start)
fixture += r'''
    int message_index = 0;
    bool was_at_bottom = false;
    int64_t previous_last = 2;
    HistoryScroll history_scroll;
    auto follow_reply = [&] {
'''
fixture += main[start:end]
fixture += r'''
    };
    tg.message_rows = {{1, false}, {2, false}, {3, false}};
    combined_list_focus = true;
    tg.bot_follow = tg.bot_focus = true;
    follow_reply();
    assert(message_index == 2 && !tg.bot_focus && !combined_list_focus);
    auto id = [&](int i) { return tg.message_rows[i].id; };
    auto height = [](int) { return 100; };
    auto inset = [](int) { return 0; };
    history_scroll.configure(3, 180, message_index, id, height, inset);
    assert(history_scroll.first() == 1 && history_scroll.offset() == 20);
    tg.message_rows.push_back({4, false}); was_at_bottom = true; previous_last = 3;
    follow_reply();
    assert(message_index == 3);
    history_scroll.configure(4, 180, message_index, id, height, inset);
    assert(history_scroll.first() == 2 && history_scroll.offset() == 20);
    message_index = 1; was_at_bottom = false; previous_last = 4;
    tg.message_rows.push_back({5, false}); follow_reply();
    assert(message_index == 1); // Reading older history must win over auto-follow.
    was_at_bottom = true; post_open = true; follow_reply(); assert(message_index == 1);
    post_open = false; photo_open = true; follow_reply(); assert(message_index == 1);
    photo_open = false; tg.bot_follow = false; follow_reply(); assert(message_index == 1);
    tg.message_rows.back().outgoing = true; follow_reply(); assert(message_index == 4);
    tg.message_rows.clear(); tg.bot_follow = tg.bot_focus = true; follow_reply();
    assert(message_index == -1 && history_scroll.first() == 0 && history_scroll.offset() == 0);
    tg.message_rows.push_back({6, false}); previous_last = 0; follow_reply();
    assert(message_index == 0);
    std::cout << "Bot reply focus: immediate/latest, incoming replies, viewport, empty history and manual navigation: PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-chat-modal-') as tmp:
    source = Path(tmp) / 'modal.cpp'
    binary = Path(tmp) / 'modal'
    source.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

# Compile the shared mention-button geometry used by drawing and taps.
start = main.index('struct HistoryMentionButton {')
end = main.index('struct HistoryDateChip {', start)
mention_fixture = r'''
#include <cassert>
#include <cmath>
#include <vector>
struct HistoryRow { int index, top, height; };
struct TelegramClient {
    bool latest = true; int unread = 1;
    std::vector<int> items{1, 2, 3};
    bool historyAtLatest() const { return latest; }
    int unreadMentionCount() const { return unread; }
    const auto& messages() const { return items; }
};
static int history_bottom = 494;
static struct { int top = 71; } history_view;
'''
mention_fixture += main[start:end]
mention_fixture += r'''
int main() {
    TelegramClient tg;
    std::vector<HistoryRow> rows{{1, 100, 100}, {2, 200, 294}};
    assert(!history_mention_button(rows, tg).visible);
    rows.back().height = 295;
    assert(history_mention_button(rows, tg).visible);
    rows.back().height = 294; tg.latest = false;
    auto button = history_mention_button(rows, tg);
    assert(button.visible && button.x == 926 && button.y == history_view.top+34);
    for (int y = 0; y < 544; ++y) for (int x = 0; x < 960; ++x)
        assert(button.hit(x, y) == ((x-button.x)*(x-button.x)+(y-button.y)*(y-button.y) < button.radius*button.radius));
    tg.unread = 0; button = history_mention_button(rows, tg);
    assert(!button.visible && !button.hit(button.x, button.y));
    tg.unread = 1; tg.latest = true; rows.pop_back();
    assert(history_mention_button(rows, tg).visible);
    assert(history_mention_button({}, tg).visible);
    history_bottom = 312;
    assert(history_mention_button({}, tg).y == 105);
    history_view.top = 151;
    button = history_mention_button({}, tg);
    assert(button.y == 185 && button.y-button.radius == history_view.top+8);
    assert(button.x+button.radius == 952);
    assert(!button.hit(930, history_bottom-30));
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-mention-button-') as tmp:
    source = Path(tmp) / 'mention.cpp'
    binary = Path(tmp) / 'mention'
    source.write_text(mention_fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Mention button: latest/older/partial/empty history and every circular hit boundary: PASS')

# Exercise the production typing-label drawing block, including its visibility gate.
start = main.index('                const std::string typing = chat_ui_visible() ?')
end = main.index('                if (chat_ui_visible() && chat_bubble::has_messages_below(', start)
typing_block = main[start:end]
assert main.count('tg.currentTypingStatus()') == 1
fixture = r'''
#include "chat_view_layout.h"
#include <cassert>
#include <string>
#include <vector>
struct Rect { int x,y,w,h; };
static std::vector<Rect> boxes;
static std::vector<std::string> labels;
static std::vector<int> baselines;
namespace ui_skin { enum class Slice { DateChip }; }
void skin_box(ui_skin::Slice,int x,int y,int w,int h,unsigned,bool) { boxes.push_back({x,y,w,h}); }
int rich_text_width(void*,float,const char* s) { return std::string(s).size()*7; }
std::string fit_text(void*,const std::string& s,int width,float) { return s.substr(0,width/7); }
void draw_text(void*,int,int y,const std::string& s,unsigned,float) { labels.push_back(s);baselines.push_back(y); }
int main() {
    chat_view::Layout history_view;
    struct { int index=0,y=71; } date_chip;
    struct { std::string value="Anna is typing...";std::string currentTypingStatus() { return value; } } tg;
    bool visible=true;
    auto chat_ui_visible=[&] { return visible; };
    void* font=nullptr;unsigned selected=1,ink=2;
    auto render=[&] {
        boxes.clear();labels.clear();baselines.clear();
'''+typing_block+r'''
    };
    render();
    assert(boxes.size()==1 && labels[0]==tg.value);
    assert(boxes[0].y==97 && baselines[0]==113);
    assert(boxes[0].x+boxes[0].w/2==history_view.center());
    history_view.combined=true;history_view.top=151;date_chip.y=151;
    render();
    assert(boxes[0].y==177 && boxes[0].x+boxes[0].w/2==history_view.center());
    tg.value=std::string(400,'x');render();
    assert(boxes[0].x>=history_view.left() && boxes[0].x+boxes[0].w<900);
    date_chip.index=-1;render();assert(boxes[0].y==history_view.top);
    visible=false;render();assert(boxes.empty() && labels.empty());
    visible=true;tg.value.clear();render();assert(boxes.empty() && labels.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-typing-label-') as tmp:
    source = Path(tmp) / 'typing.cpp'
    binary = Path(tmp) / 'typing'
    source.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Typing label: below date, Classic/Combined, long names, no-date fallback and hidden/empty states: PASS')

# Exercise the actual history clip call against the outer focus-ring mesh.
start = main.index('auto history_rows = history_layout(')
start = main.index('vita2d_set_clip_rectangle(', start)
clip_call = main[start:main.index(';', start)+1]
fixture = r'''
#include "chat_view_layout.h"
#include "chat_bubble.h"
#include <cassert>
static int clip_top;
static void vita2d_set_clip_rectangle(int, int top, int, int) { clip_top = top; }
int main() {
    for (bool combined : {false, true}) for (int history_bottom : {312, 494, 544}) {
        const auto history_view = chat_view::layout(combined, false);
'''+clip_call+r'''
        for (bool out : {false, true}) for (bool before : {false, true})
            for (bool after : {false, true}) {
                const auto mesh = chat_bubble::shape(history_view.bubbleLeft(), history_view.messageTop(),
                    history_view.bubbleWidth(), 80, out, before, after);
                for (int i = 0; i < mesh.count; ++i) {
                    const auto normal = chat_bubble::outward(mesh, i);
                    assert(mesh.points[i].y + 2 * normal.y >= clip_top);
                }
            }
    }
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-history-clip-') as tmp:
    source = Path(tmp) / 'clip.cpp'
    binary = Path(tmp) / 'clip'
    source.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('History clip: full outer focus ring fits above top-aligned messages: PASS')

# Real badge rendering and rounded contours; all font sizes and both list widths.
start = main.index('static float draw_unread_badge(')
end = main.index('\n}\n', start)+3
badge_code = main[start:end]
radial_view = (root/'src/radial_menu_view.h').read_text()
a = radial_view.index('inline vita2d_color_vertex* vertices(')
b = radial_view.index('inline void triangle(', a)
contour_code = radial_view[a:b]
a = radial_view.index('inline void rounded_box(')
b = radial_view.index('inline void icon(', a)
contour_code += radial_view[a:b]
fixture = r'''
#include "radial_menu.h"
#include "audio_bar.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <climits>
#define RGBA8(r,g,b,a) ((unsigned)(r)|((unsigned)(g)<<8)|((unsigned)(b)<<16)|((unsigned)(a)<<24))
struct vita2d_pgf {};
struct vita2d_color_vertex { float x,y,z; unsigned color; };
enum { SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP };
struct Draw { int mode;std::vector<vita2d_color_vertex> vertices; };
static std::vector<Draw> draws;
struct Label { int x,y;std::string text;unsigned color;float scale; };
static std::vector<Label> labels;
static int speaker_calls=0;
static void draw_audio_control(audio_bar::Control control,float x,float y,bool playing,int volume,unsigned) { assert(control==audio_bar::Mute && x>58 && y==124 && !playing && !volume);++speaker_calls; }
static size_t pool_used=0;
void* vita2d_pool_memalign(size_t n,size_t) { static vita2d_color_vertex v[2048];const auto first=pool_used;pool_used+=n/sizeof(v[0]);assert(pool_used<=2048);return v+first; }
void vita2d_draw_array(int mode,const vita2d_color_vertex* v,size_t n) { draws.push_back({mode,{v,v+n}}); }
namespace radial {
inline unsigned alpha(unsigned color,unsigned value) { return (color&0xffffffu)|(value<<24); }
'''+contour_code+r'''
}
static const radial::Geometry& ui_geometry() { static radial::Geometry g(radial::Config{});return g; }
static float size_scale=1;
static unsigned notification=RGBA8(30,144,255,255);
namespace ui_skin { unsigned color(const char*,unsigned) { return notification; } }
static int scaled_px(int px) { return std::lround(px*size_scale); }
static int rich_text_width(vita2d_pgf*,float scale,const char* text) { return std::lround(std::strlen(text)*28*scale*size_scale); }
static void draw_text(vita2d_pgf*,int x,int y,const std::string& text,unsigned color,float scale) { labels.push_back({x,y,text,color,scale}); }
struct ChatItem { int unread_count=0,unread_mention_count=0;bool marked_unread=false; };
'''+badge_code+r'''
static float cross(vita2d_color_vertex a,vita2d_color_vertex b,float x,float y) { return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x); }
static bool inside(vita2d_color_vertex a,vita2d_color_vertex b,vita2d_color_vertex c,float x,float y) {
    const float p=cross(a,b,x,y),q=cross(b,c,x,y),r=cross(c,a,x,y);
    return (p>0.00001f && q>0.00001f && r>0.00001f)||(p< -0.00001f && q< -0.00001f && r< -0.00001f);
}
static void coverage() {
    assert(draws.size()==10); // Each of the four glow contours and the core is a fan plus a fringe.
    for (int layer=0;layer<5;++layer) {
        const auto& core=draws[layer*2];const auto& fringe=draws[layer*2+1];
        assert(core.mode==SCE_GXM_PRIMITIVE_TRIANGLE_FAN && fringe.mode==SCE_GXM_PRIMITIVE_TRIANGLE_STRIP);
        const auto& v=core.vertices;float left=v[0].x,right=left,top=v[0].y,bottom=top;
        for (auto p:fringe.vertices) {left=std::min(left,p.x);right=std::max(right,p.x);top=std::min(top,p.y);bottom=std::max(bottom,p.y);}
        for (float y=top+.379f;y<bottom;y+=.83f) for (float x=left+.271f;x<right;x+=.83f) {
            int hits=0;
            for (size_t i=1;i+1<v.size();++i) hits+=inside(v[0],v[i],v[i+1],x,y);
            const auto& f=fringe.vertices;
            for (size_t i=0;i+2<f.size();++i) hits+=inside(f[i],f[i+1],f[i+2],x,y);
            assert(hits<=1); // No repeated alpha coverage within a rounded fill or its AA fringe.
        }
    }
}
int main() {
    for (int percent=70;percent<=130;percent+=10) for (bool combined:{false,true})
    for (bool enabled:{false,true}) for (bool mention:{false,true})
    for (int count:{0,1,9,10,128,1024,INT_MAX}) for (unsigned color:{RGBA8(30,144,255,255),RGBA8(255,255,255,255),RGBA8(0,0,0,255)}) {
        size_scale=percent/100.f;notification=color;draws.clear();labels.clear();pool_used=0;
        ChatItem chat;chat.unread_count=count;chat.unread_mention_count=mention?1:0;
        const float right=combined?193.f:929.f,minimum=combined?58.f:96.f;
        const auto left=draw_unread_badge(nullptr,chat,enabled,minimum,right,124,42,RGBA8(242,247,255,255));
        if (!count && !mention) {assert(draws.empty() && labels.empty());continue;}
        assert(left>=minimum-8.01f);
        const auto& core=draws[8].vertices;float width=0,height=0;
        float minx=core[0].x,maxx=minx,miny=core[0].y,maxy=miny;
        for (auto p:core) {minx=std::min(minx,p.x);maxx=std::max(maxx,p.x);miny=std::min(miny,p.y);maxy=std::max(maxy,p.y);}
        width=maxx-minx;height=maxy-miny;
        if (enabled && count) {
            assert(labels.size()==size_t(mention?6:5));
            const auto& number=labels[4];assert(number.text==std::to_string(count));
            assert(number.x>=minx-1 && number.x+rich_text_width(nullptr,number.scale,number.text.c_str())<=maxx+1);
            if (count<10) assert(std::fabs(width-height)<.01f);
            if (count>=10) assert(width>height);
        } else { assert(labels.size()==size_t(mention?1:0));assert(std::fabs(width-12)<.01f); }
        if (mention) assert(labels.back().text=="@" && labels.back().x+rich_text_width(nullptr,.65f,"@")<=minx-7);
    }
    size_scale=1;notification=RGBA8(30,144,255,255);
    for (int count:{7,128}) {
        ChatItem chat;chat.unread_count=count;draws.clear();labels.clear();pool_used=0;
        draw_unread_badge(nullptr,chat,true,58,193,124,42,0xffffffffu);coverage();
    }
    for (int percent=70;percent<=130;percent+=10) for (bool enabled:{false,true})
    for (bool combined:{false,true}) for (bool mention:{false,true}) for (int count:{0,7,128}) {
        size_scale=percent/100.f;draws.clear();labels.clear();pool_used=0;speaker_calls=0;
        ChatItem chat;chat.unread_count=count;chat.unread_mention_count=mention?1:0;
        const auto left=draw_unread_badge(nullptr,chat,enabled,combined?58.f:96.f,combined?193.f:929.f,124,42,0xffffffffu,true);
        assert(draws.empty() && speaker_calls==1 && left>=58);
        assert(labels.size()==size_t(mention?1:0));
        if(mention) assert(labels[0].text=="@");
    }
    ChatItem marked;marked.marked_unread=true;draws.clear();labels.clear();pool_used=0;
    draw_unread_badge(nullptr,marked,true,58,193,124,42,0xffffffffu);assert(draws.size()==10 && labels.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-unread-badge-') as tmp:
    source=Path(tmp)/'badge.cpp';binary=Path(tmp)/'badge'
    source.write_text(fixture)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror',
                    '-I',str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Unread badge: counts/dots/@, Classic/Combined, all font sizes/palettes, INT_MAX and rounded alpha coverage: PASS')
