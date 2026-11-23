"""Run the production app progress renderer with host drawing recorders."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
start = main.index('static bool vpk_dialog(')
end = main.index('\nstatic void theme_switch_screen(', start)
source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#define RGBA8(r,g,b,a) (unsigned(r) | (unsigned(g)<<8) | (unsigned(b)<<16) | (unsigned(a)<<24))
struct vita2d_pgf {};
constexpr int SCE_CTRL_CROSS = 1, SCE_CTRL_CIRCLE = 2, SCE_TOUCH_PORT_FRONT = 0, SCE_KERNEL_POWER_TICK_DEFAULT = 0;
struct SceCtrlData { unsigned buttons; };
struct SceTouchData { unsigned reportNum; struct { int x, y, id; } report[2]; };
struct Input { unsigned buttons; int x = -1; int y = 430; int id = 1; unsigned count = 1; };
static std::vector<Input> inputs;
static size_t sample;
void sceKernelPowerTick(int) {}
int sceCtrlPeekBufferPositive(int, SceCtrlData* out, int) {
    assert(sample < inputs.size());
    out->buttons = inputs[sample].buttons;
    return 1;
}
int sceTouchPeek(int, SceTouchData* out, int) {
    const auto& in = inputs[sample++];
    out->reportNum = in.x < 0 ? 0 : in.count;
    out->report[0] = {in.x * 2, in.y * 2, in.id};
    return 1;
}
std::string tr(const char* key) { return key; }
std::string fit_text(vita2d_pgf*,const std::string& text,int,float) { return text; }
std::vector<std::string> wrap_text(vita2d_pgf*,const std::string& text,int) { return {text}; }

struct Rect { float x,y,w,h; };
static std::vector<Rect> bars;
static int64_t now;
static float font_size = 1;
static int starts, ends, swaps, backgrounds, labels;
static unsigned palette;
void vita2d_start_drawing() { ++starts; }
void vita2d_clear_screen() {}
void vita2d_end_drawing() { ++ends; }
void vita2d_swap_buffers() { ++swaps; }
int64_t sceKernelGetSystemTimeWide() { return now; }
void vita2d_draw_rectangle(float x,float y,float w,float h,unsigned) { bars.push_back({x,y,w,h}); }
namespace ui_skin {
enum class Slice { Dialog };
void background() { ++backgrounds; }
unsigned surface_color(unsigned) { return palette; }
unsigned color(const char*, unsigned) { return palette; }
}
void skin_box(ui_skin::Slice,float x,float y,float w,float h,unsigned,bool) {
    assert((x == 150 && y == 192 && w == 660 && h == 160) ||
           (x == 70 && y == 48 && w == 820 && h == 448));
}
int rich_text_width(vita2d_pgf*,float scale,const char* text) {
    return std::string(text).size()*10*scale*font_size;
}
void draw_text(vita2d_pgf* font,float x,float y,const std::string& text,unsigned,float scale) {
    ++labels;
    assert(std::isfinite(scale) && scale > 0 && scale <= .85f);
    float width = rich_text_width(font,scale,text.c_str());
    if (y == 255) assert(x >= 179.99f && x+width <= 780.01f);
    else assert(x >= 100 && y < 412 + 52);
}
'''
source += main[start:end]
source += r'''
int main() {
    vita2d_pgf font;
    auto dialog = [&](std::vector<Input> sequence, bool question, bool expected) {
        inputs = std::move(sequence);
        sample = 0;
        assert(vpk_dialog(&font, "Full access warning", question) == expected);
        assert(sample == inputs.size());
    };
    dialog({{1}, {1}, {0}, {1}}, true, true);  // Held Cross cannot accept.
    dialog({{0}, {3}}, true, false);          // Back wins over simultaneous buttons.
    dialog({{0, 500}, {0}, {0, 500}, {0}}, true, true);
    dialog({{0}, {0, 220}, {0}}, true, false);
    dialog({{0}, {0, 740}, {0}, {0, 499}, {0}, {2}}, true, false);
    dialog({{0}, {0, 500, 430, 1, 2}, {0}, {2}}, true, false);
    dialog({{0}, {0, 500}, {0, 500, 464}, {0}, {2}}, true, false);
    dialog({{0}, {0, 360}, {0}}, false, true);

    for (unsigned theme : {0xffeeeeeeu, 0xff101820u, 0xff13579bu})
    for (float size : {.5f, 1.f, 1.5f, 2.f, 3.f})
    for (const std::string& text : {std::string("Installing"), std::string("Установка — インストール"), std::string(2000,'W')})
    for (int percent : {-1, 0, 25, 100, 150})
    for (int64_t time : {0,500000,1000000,1500000,1999999,2000000}) {
        bars.clear(); starts=ends=swaps=backgrounds=labels=0;
        palette=theme; font_size=size; now=time;
        vpk_progress_screen(&font,text,percent);
        assert(starts==1 && ends==1 && swaps==1 && backgrounds==1 && labels==1);
        assert(bars.size()==2);
        auto bg=bars[0], bar=bars[1];
        assert(bg.x==240 && bg.y==296 && bg.w==480 && bg.h==8);
        assert(bar.x>=240 && bar.x+bar.w<=720.01f && bar.y==296 && bar.h==8);
        assert(bar.w==(percent<0 ? 140.f : 480.f*std::clamp(percent,0,100)/100));
        if (percent>=0) assert(bar.x==240);
        else if (time==0 || time==2000000) assert(bar.x==240);
        else if (time==1000000) assert(bar.x==580);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-vpk-progress-') as work:
    cpp = Path(work) / 'test.cpp'
    exe = Path(work) / 'test'
    cpp.write_text(source)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('VPK app progress: bounds, animation, percentage, labels and palettes: PASS')
