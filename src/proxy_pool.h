#pragma once
#include "proxy_config.h"
#include <algorithm>
#include <cstdint>
#include <vector>

inline bool same_proxy(const ProxyConfig& a,const ProxyConfig& b) {
    return a.kind==b.kind && a.server==b.server && a.port==b.port && a.secret==b.secret &&
        a.username==b.username && a.password==b.password;
}

// UI-thread policy only. TelegramClient owns the single asynchronous probe slot.
class ProxyPool {
public:
    static constexpr size_t Limit=16;
    inline static constexpr int SwitchSeconds[]={5,10,15,30,60};
    static int switchIndex(int64_t seconds) {
        for (int i=0;i<5;++i) if (SwitchSeconds[i]==seconds) return i;
        return 1;
    }
    enum class State { Unknown, Checking, Available, Unavailable };
    enum class Connection { Ready, Connecting, Offline };
    struct Health { State state=State::Unknown; int milliseconds=-1; int64_t checked=0,retry_after=0; };
    std::vector<ProxyConfig> entries;
    std::vector<Health> health;
    int active=-1;
    bool enabled=false,automatic=true;
    void configure(std::vector<ProxyConfig> list,int selected,bool use,bool auto_switch,int64_t now,int switch_seconds=10) {
        if (list.size()>Limit) list.resize(Limit);
        std::vector<Health> retained(list.size());
        for (size_t i=0;i<list.size();++i) for (size_t j=0;j<entries.size();++j)
            if (same_proxy(list[i],entries[j])) { retained[i]=health[j];if (retained[i].state==State::Checking) retained[i].state=State::Unknown;break; }
        entries=std::move(list);health=std::move(retained);
        active=entries.empty()?-1:std::clamp(selected,0,int(entries.size())-1);
        switch_delay_=SwitchSeconds[switchIndex(switch_seconds)]*Second;
        enabled=use;automatic=auto_switch;bad_since_=0;last_tick_=now;cooldown_=now+switch_delay_;
        cancelAnalysis();next_analysis_=now;
    }
    void analyze(bool manual=true) { cursor_=0;scanning_=!entries.empty();manual_scan_=manual; }
    void cancelAnalysis() {
        scanning_=false;cursor_=0;
        for (auto& h:health) if (h.state==State::Checking) h.state=State::Unknown;
    }
    bool analyzing() const { return scanning_; }
    int nextCheck() {
        if (!scanning_ || cursor_>=int(entries.size())) return -1;
        health[cursor_].state=State::Checking;return cursor_++;
    }
    void result(int index,int milliseconds,int64_t now) {
        if (index<0 || index>=int(health.size()) || health[index].state!=State::Checking) return;
        auto& h=health[index];h.state=milliseconds<0?State::Unavailable:State::Available;
        h.milliseconds=milliseconds;h.checked=now;
        if (cursor_>=int(entries.size())) { scanning_=false;next_analysis_=now+120*Second; }
    }
    // Returns a reachable alternative after sustained failure; never chooses direct networking.
    int tick(int64_t now,Connection connection) {
        if (last_tick_ && now-last_tick_>5*Second) {
            bad_since_=0;cooldown_=now+switch_delay_;next_analysis_=now;
            for (auto& h:health) { h.state=State::Unknown;h.checked=0; }
            cancelAnalysis();
        }
        last_tick_=now;
        if (connection!=Connection::Connecting) bad_since_=0;
        else if (!bad_since_) bad_since_=now;
        if (connection==Connection::Offline && scanning_ && !manual_scan_) cancelAnalysis();
        if (!enabled || !automatic || entries.size()<2 || connection==Connection::Offline) return -1;
        if (!scanning_ && now>=next_analysis_) analyze(false);
        if (!bad_since_ || now-bad_since_<switch_delay_ || now<cooldown_) return -1;
        int best=-1;
        for (size_t i=0;i<health.size();++i) {
            const auto& h=health[i];
            if (int(i)==active || h.state!=State::Available || !h.checked || now-h.checked>120*Second || now<h.retry_after) continue;
            if (best<0 || h.milliseconds<health[best].milliseconds) best=i;
        }
        return best;
    }
    void switched(int index,int64_t now) {
        if (active>=0) health[active].retry_after=now+120*Second;
        active=index;bad_since_=now;cooldown_=now+switch_delay_;
    }
private:
    static constexpr int64_t Second=1000000;
    int cursor_=0;
    bool scanning_=false,manual_scan_=false;
    int64_t bad_since_=0,last_tick_=0,cooldown_=0,next_analysis_=0;
    int64_t switch_delay_=10*Second;
};

// List rendering and hit-testing share these exact bounds, including row gaps.
struct ProxyListLayout {
    static constexpr int Top=104,Pitch=36,Height=32,Visible=9,Fixed=7;
    static constexpr int IntervalLeft=650,IntervalPitch=54,IntervalWidth=50,IntervalTop=3,IntervalHeight=26;
    static int visibleCount(int count,bool automatic) { return count-(automatic?0:1); }
    static int visibleRow(int row,bool automatic) { return row-(!automatic && row>2?1:0); }
    static int logicalRow(int row,bool automatic) { return row+(!automatic && row>=2?1:0); }
    static int next(int row,int direction,int count,bool automatic) {
        row=(row+direction+count)%count;
        if (!automatic && row==2) row=(row+direction+count)%count;
        return row;
    }
    static int intervalHit(int x,int y,int selected,int count,bool automatic=true) {
        if (!automatic) return -1;
        const int top=Top+(2-first(selected,count))*Pitch+IntervalTop;
        if (hit(x,y,selected,count)!=2 || y<top || y>=top+IntervalHeight) return -1;
        const int offset=x-IntervalLeft;
        return offset>=0 && offset<5*IntervalPitch && offset%IntervalPitch<IntervalWidth ? offset/IntervalPitch : -1;
    }
    static int first(int selected,int count,bool automatic=true) {
        return std::clamp(visibleRow(selected,automatic)-Visible+1,0,std::max(0,visibleCount(count,automatic)-Visible));
    }
    static int footerHit(int x,int y) {
        if (x<24 || x>=936 || y<500 || y>=540 || (x-24)%228>=224) return -1;
        return (x-24)/228;
    }
    static int hit(int x,int y,int selected,int count,bool automatic=true) {
        if (x<24 || x>=936 || y<Top || y>=Top+Visible*Pitch || (y-Top)%Pitch>=Height) return -1;
        int index=first(selected,count,automatic)+(y-Top)/Pitch;
        return index<visibleCount(count,automatic)?logicalRow(index,automatic):-1;
    }
};
