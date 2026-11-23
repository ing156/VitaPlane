#include "bottom_downloads.h"
#include <cassert>
#include <cstdio>
using namespace bottom_downloads;
int main() {
    const std::vector<Key> keys{{1,10},{2,20},{3,30}};
    for(float left : {0.f,320.f}) for(float bottom : {494.f,544.f}) {
        Layout layout{{left,bottom},8,true};
        assert(layout.list().y>=36 && layout.list().x==left);
        assert(layout.edge().h==10);
        if(bottom==494) assert(layout.list().y==44);
        assert(layout.row(7).y+layout.row(7).h==layout.top());
        for(int y=0;y<=544;++y) for(int x=0;x<=960;++x) {
            const int index=layout.hit(x,y);
            const bool within=x>=left && x<960 && y>=layout.list().y+Layout::header_height && y<layout.top();
            assert((index>=0)==within);
            if(index>=0) assert(layout.row(index).contains(x,y));
        }
        assert(!layout.gesture(left-1,bottom-1,true));
        assert(!layout.gesture(960,bottom-1,true));
        assert(!layout.gesture(left+5,layout.top()-1,true));
        assert(!layout.gesture(left+15,layout.bar.top()+5,false)); // Music seek is not a carousel gesture.
        assert(!layout.gesture(770,layout.bar.top()+30,false)); // Neither is music volume.
        assert(layout.gesture(left+12,layout.top()+1,false));
        assert(layout.gesture(layout.bar.titleX()+1,layout.bar.top()+20,false));
    }
    Layout layout{{0,544},3,true};
    State state; state.reconcile(true,true,keys);
    auto finger=[&](bool down,float x,float y,int id=4,bool enabled=true) {
        return state.touch(layout,{down,down,id,x,y},enabled,true,true,keys);
    };
    assert(!finger(true,100,200).consumed);
    assert(!finger(true,300,500).consumed); // Chat-origin drag cannot become a panel gesture.
    finger(false,300,500);
    assert(finger(true,300,500).consumed);
    auto result=finger(true,302,465);
    assert(result.switched && result.consumed && state.downloads);
    assert(finger(true,302,450).consumed); // One swipe, one switch.
    assert(finger(false,302,450).consumed);
    assert(state.downloads);
    assert(finger(true,300,500).consumed);
    assert(finger(false,300,500).toggle_list);
    state.expanded=true;
    auto row=layout.row(1);
    assert(finger(true,100,row.y+10).consumed);
    // A new contact does not inherit the captured pointer, and its release cannot toggle a file.
    result=state.touch(layout,{true,true,9,100,row.y+10},true,true,true,keys);
    assert(result.consumed && !result.toggle_file);
    assert(finger(false,100,row.y+10,9).consumed);
    finger(true,100,row.y+10);
    result=finger(false,100,row.y+10);
    assert(result.toggle_file && result.key==keys[1]);
    for (auto action : {Action::Lower,Action::Raise,Action::Cancel}) {
        const auto box=layout.control(1,action);
        finger(true,box.x+2,box.y+2);
        auto r=finger(false,box.x+2,box.y+2);
        assert(r.toggle_file && r.action==action && r.key==keys[1]);
    }
    state.selected=keys[1]; state.reconcile(true,true,{keys[2],keys[1]});
    assert(state.index==1 && state.selected==keys[1]);
    state.reconcile(true,true,{keys[1]}); assert(state.index==0 && state.selected==keys[1]);
    state.move(1,{keys[1]});assert(state.index==0);
    finger(true,100,row.y+10);
    result=state.touch(layout,{false,false,-1,0,0},true,true,true,{keys[0],keys[2]});
    assert(result.consumed && !result.toggle_file); // Completed/reordered rows cannot toggle their replacement.
    finger(true,100,layout.list().y-2);
    assert(finger(false,100,layout.list().y-2).dismiss);
    state.expanded=false;
    finger(true,300,500);
    assert(!finger(false,300,500,4,false).toggle_list); // Hidden during a gesture.
    state.expanded=true; state.reconcile(true,false,{});
    assert(!state.expanded && !state.downloads);
    state.reconcile(false,true,keys); assert(state.downloads);
    state.focus=true;state.reconcile(false,false,{});assert(!state.focus);
    std::puts("Bottom downloads: layout bounds, shared hit geometry, carousel ownership, pointer IDs, stale rows and completion: PASS");
}
