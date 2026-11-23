#pragma once
#include "audio_bar.h"
#include <cmath>
#include <vector>

namespace bottom_downloads {
struct Key {
    int64_t chat = 0, message = 0;
    bool operator==(const Key& other) const { return chat==other.chat && message==other.message; }
};
enum class Action { Toggle, Lower, Raise, Cancel };
struct Layout {
    audio_bar::Layout bar;
    int count = 0;
    bool carousel = false;
    static constexpr float row_height = 42, header_height = 32, edge_height = 10;
    audio_bar::Rect edge() const { return {bar.left+10,bar.top()-edge_height,940-bar.left,edge_height}; }
    float top() const { return bar.top()-(carousel ? edge_height : 0); }
    audio_bar::Rect list() const { return {bar.left,top()-header_height-row_height*count,960-bar.left,header_height+row_height*count}; }
    audio_bar::Rect back() const { auto r=list(); return {r.x,r.y,r.w,header_height}; }
    audio_bar::Rect row(int index) const { auto r=list(); return {r.x,r.y+header_height+index*row_height,r.w,row_height}; }
    audio_bar::Rect control(int index, Action action) const {
        const auto r = row(index);
        const float end = r.x + r.w;
        switch (action) {
        case Action::Lower: return {end-196,r.y,46,r.h};
        case Action::Raise: return {end-150,r.y,46,r.h};
        case Action::Cancel: return {end-104,r.y,104,r.h};
        default: return {r.x,r.y,r.w-196,r.h};
        }
    }
    Action action(int index,float x,float y) const {
        for (auto a : {Action::Lower,Action::Raise,Action::Cancel})
            if (control(index,a).contains(x,y)) return a;
        return Action::Toggle;
    }
    int hit(float x,float y) const {
        for (int i=0;i<count;++i) if(row(i).contains(x,y)) return i;
        return -1;
    }
    bool gesture(float x,float y,bool downloads) const {
        if(carousel && edge().contains(x,y)) return true;
        if(downloads) return bar.panel().contains(x,y);
        // The title has no music command: a vertical swipe never starts a seek or volume change.
        return carousel && audio_bar::Rect{bar.titleX(),bar.top()+16,bar.titleWidth(),56}.contains(x,y);
    }
};
struct Pointer { bool any=false,present=false; int id=-1; float x=0,y=0; };
struct Result { bool consumed=false,switched=false,toggle_list=false,dismiss=false,toggle_file=false; Key key; Action action=Action::Toggle; };
struct State {
    bool downloads=false,expanded=false,focus=false,owned=false;
    int pointer=-1,index=0;
    Key selected;
    bool previous=false,blocked=false,moved=false;
    float start_x=0,start_y=0,last_x=0,last_y=0;
    int pressed_row=-1;
    Key pressed_key;
    void reconcile(bool music,bool files,const std::vector<Key>& keys) {
        if(!files) { downloads=false; expanded=false; }
        if(!music && files) downloads=true;
        if(!music && !files) focus=false;
        if(!downloads) expanded=false;
        if(keys.empty()) { index=0; selected={}; return; }
        auto it=std::find(keys.begin(),keys.end(),selected);
        index=it==keys.end() ? std::clamp(index,0,int(keys.size())-1) : int(it-keys.begin());
        selected=keys[index];
    }
    void move(int direction,const std::vector<Key>& keys) {
        if(keys.empty()) return;
        index=std::clamp(index+direction,0,int(keys.size())-1); selected=keys[index];
    }
    Result touch(const Layout& layout,const Pointer& p,bool enabled,bool music,bool files,const std::vector<Key>& keys) {
        Result result;
        if(blocked) { result.consumed=true; blocked=p.any; previous=p.any; return result; }
        if(!owned && p.any && !previous && enabled &&
           (expanded || layout.gesture(p.x,p.y,downloads))) {
            owned=true; pointer=p.id; moved=false; start_x=last_x=p.x; start_y=last_y=p.y;
            pressed_row=expanded ? layout.hit(p.x,p.y) : -1;
            pressed_key=pressed_row>=0 && pressed_row<int(keys.size()) ? keys[pressed_row] : Key{};
        }
        if(owned) {
            result.consumed=true;
            if(p.present && p.id==pointer) {
                last_x=p.x; last_y=p.y;
                const float dx=p.x-start_x,dy=p.y-start_y;
                if(std::hypot(dx,dy)>8) moved=true;
                if(enabled && !expanded && music && files && std::abs(dy)>=24 && std::abs(dy)>std::abs(dx)*1.25f) {
                    downloads=!downloads; expanded=false; result.switched=true; blocked=p.any; owned=false;
                }
            } else {
                owned=false; blocked=p.any;
                if(enabled && !moved && !p.any) {
                    if(expanded) {
                        const int hit=layout.hit(last_x,last_y);
                        if(hit==pressed_row && hit>=0 && hit<int(keys.size()) && keys[hit]==pressed_key &&
                           layout.action(hit,start_x,start_y)==layout.action(hit,last_x,last_y)) {
                            selected=pressed_key; index=hit; result.key=pressed_key; result.toggle_file=true; result.action=layout.action(hit,last_x,last_y);
                        } else if(pressed_row<0) result.dismiss=true;
                    } else if(downloads && layout.bar.panel().contains(last_x,last_y)) result.toggle_list=true;
                }
            }
        }
        previous=p.any;
        return result;
    }
};
}
