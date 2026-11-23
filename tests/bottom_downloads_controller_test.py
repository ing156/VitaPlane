"""Compile the main-loop panel routing to protect the existing Triangle action."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
start = main.index('        const unsigned panel_refresh =')
end = main.index('        unsigned int released =', start)
block = main[start:end]
fixture = r'''
#include "bottom_downloads.h"
#include <cassert>
constexpr unsigned SCE_CTRL_TRIANGLE=1, SCE_CTRL_CROSS=2, SCE_CTRL_CIRCLE=4,
    SCE_CTRL_UP=8, SCE_CTRL_DOWN=16, SCE_CTRL_LEFT=32, SCE_CTRL_RIGHT=64,SCE_CTRL_SQUARE=128;
int main() {
    for (bool downloads_show : {false,true}) for(bool expanded : {false,true}) {
        if(!downloads_show && expanded) continue;
        for (bool popup : {false,true}) for(unsigned button : {SCE_CTRL_TRIANGLE,SCE_CTRL_CROSS,SCE_CTRL_CIRCLE,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_SQUARE}) {
            bottom_downloads::State downloads_panel;
            downloads_panel.downloads=downloads_show; downloads_panel.expanded=expanded;
            downloads_panel.focus=downloads_show;
            const bool downloads_modal_input=expanded, player_chord=false;
            unsigned pressed=button,physical_pressed=button;
            bool bg_focus=!downloads_show, bg_speed_open=popup, bg_volume_open=false;
            int bg_speed_index=2,bg_volume=100;
            bool navigation_handled=false;
            struct {} scroll_direction;
            audio_bar::Shoulders bg_shoulders;
            auto bg_set_speed=[](int){};
            auto bg_set_volume=[](int){};
            auto bg_speed=[](){return 100;};
            auto bg_position=[](){return int64_t(0);};
            auto bg_seek=[](int64_t){};
            auto bg_action=[](audio_bar::Control){};
            auto bg_focus_control=audio_bar::Play;
            const std::vector<bottom_downloads::Key> download_keys{{1,2}};
            downloads_panel.reconcile(true,true,download_keys);
            int toggles=0;
            int changes=0;
            auto download_action=[&](bottom_downloads::Key key,bottom_downloads::Action action){
                assert(key==download_keys[0]);
                using bottom_downloads::Action;
                assert(action==(button==SCE_CTRL_SQUARE ? Action::Cancel : button==SCE_CTRL_LEFT ? Action::Lower : Action::Raise));
                ++changes;
            };
            auto toggle_download=[&](bottom_downloads::Key key){assert(key==download_keys[0]);++toggles;};
'''
fixture += block
fixture += r'''
            assert(changes==int(expanded && (button==SCE_CTRL_SQUARE || button==SCE_CTRL_LEFT || button==SCE_CTRL_RIGHT)));
            (void)scroll_direction; (void)navigation_handled;
            assert(downloads_panel.downloads==downloads_show); // No controller key changes carousel pages.
            if(button==SCE_CTRL_TRIANGLE) {
                const bool modal=expanded || popup;
                assert((pressed&SCE_CTRL_TRIANGLE)==(modal ? 0u : SCE_CTRL_TRIANGLE));
                assert((physical_pressed&SCE_CTRL_TRIANGLE)==(modal ? 0u : SCE_CTRL_TRIANGLE));
            }
            if(downloads_show && !popup) {
                if(button==SCE_CTRL_CROSS) {
                    assert(downloads_panel.expanded);
                    assert(toggles==(expanded ? 1 : 0));
                }
                if(button==SCE_CTRL_CIRCLE) assert(!downloads_panel.expanded && !downloads_panel.focus);
            }
        }
    }
}
'''
with tempfile.TemporaryDirectory(prefix='vitagram-bottom-controller-') as work:
    source=Path(work)/'controller.cpp'
    binary=Path(work)/'controller'
    source.write_text(fixture)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',
                    '-I',str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Bottom controller: Triangle reaches main refresh, modals block it, Cross/Circle controls retained: PASS')
