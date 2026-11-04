"""Run the production edit menu action and keyboard opening/closing with UI stubs."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()

def block(marker):
    begin = source.index(marker)
    brace = source.index('{', begin)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

pos = source.index('tg.canEditMessage(m[message_index].id, false)')
menu = source[source.rfind('        if (has_message', 0, pos):source.index(';', pos) + 1]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
enum class InputKind { None, Message };
constexpr unsigned SCE_CTRL_START = 1, SCE_CTRL_CROSS = 2;
std::string tr(const char* key) { return key; }
struct Item { const char* key; unsigned button; int special; };
struct Active { bool on = false; bool active() const { return on; } } circle_recorder, recorder;
struct Client {
    bool allowed = true, topics = false, editing = true, request = true;
    int64_t requested = 0;
    std::string initial = "Existing я😀";
    InputKind kind = InputKind::Message;
    bool canEditMessage(int64_t, bool) { return allowed; }
    bool topicsOpen() { return topics; }
    bool requestEdit(int64_t id) { requested = id; return request; }
    bool editingMessage() { return editing; }
    InputKind inputKind() { return kind; }
    std::string inputPrompt() { return "Edit message"; }
    bool inputSecret() { return false; }
    std::string inputInitial() { return initial; }
} tg;
struct Keyboard {
    bool on = false, save = false; std::string initial;
    bool active() { return on; }
    bool submitted() { return false; }
    bool cancelled() { return false; }
    void open(const std::string&, bool, const std::string& value, bool message, bool editing) {
        on = true; initial = value; save = editing; assert(message);
    }
    void close() { on = false; }
} kb;
int main() {
    bool has_message = true, photo_open = false, video_fullscreen = false, stickers_open = false;
    bool gifs_open = false, reactions_open = false, search_open = false, settings_open = false, edit_keyboard = false;
    int message_index = 0; unsigned pressed = 0;
    struct Message { int64_t id; }; std::vector<Message> m{{77}};
    std::vector<Item> v;
    auto menu = [&] { v.clear();
'''
fixture += menu + '\n};\n'
fixture += r'''
    menu(); assert(v.size() == 1 && v[0].special == 37);
    for (bool* hidden : {&photo_open, &video_fullscreen, &stickers_open, &gifs_open, &reactions_open,
                         &search_open, &circle_recorder.on, &recorder.on, &tg.topics}) {
        *hidden = true; menu(); assert(v.empty()); *hidden = false;
    }
    tg.allowed = false; menu(); assert(v.empty()); tg.allowed = true;
    has_message = false; menu(); assert(v.empty()); has_message = true;
    Item item{"menu.edit_message", 0, 37};
    int64_t chat_menu_message = 77; std::string browser_error; bool radial_flow = true;
    auto activate = [&] {
'''
fixture += block('else if (item.special == 37)').replace('else if', 'if', 1) + '\n};\n'
fixture += r'''
    activate(); assert(tg.requested == 77 && !radial_flow && browser_error.empty());
    tg.request = false; activate(); assert(browser_error == "message.edit_unavailable");
    auto open = [&] {
'''
fixture += block('if (!circle_recorder.active() && !recorder.active() && !settings_open && !(pressed & SCE_CTRL_START) && tg.inputKind()')
fixture += '\n};\nauto cancel = [&] {\n' + block('if (edit_keyboard && !tg.editingMessage())') + '\n};\n'
fixture += r'''
    open(); assert(kb.on && kb.save && kb.initial == "Existing я😀" && edit_keyboard);
    tg.editing = false; cancel(); assert(!kb.on && !edit_keyboard);
    tg.initial.clear(); open(); assert(kb.on && !kb.save && kb.initial.empty()); kb.close();
    pressed = SCE_CTRL_START; open(); assert(!kb.on);
}
'''
with tempfile.TemporaryDirectory(prefix='vitaplane-edit-ui-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Edit UI: supported menu, modal exclusions, stable target, prefilled Save keyboard and cancellation PASS')
