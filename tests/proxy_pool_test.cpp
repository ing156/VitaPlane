#include "proxy_pool.h"
#include <cassert>
#include <iostream>

int main() {
    constexpr int64_t S=1000000;
    std::vector<ProxyConfig> entries(3);
    for (int i=0;i<3;++i) { entries[i].server="proxy"+std::to_string(i)+".example";entries[i].secret="test"; }
    ProxyPool p;p.configure(entries,0,true,true,S,15);
    assert(p.tick(S,ProxyPool::Connection::Connecting)==-1);
    assert(p.nextCheck()==0);p.result(0,10,S);
    assert(p.nextCheck()==1);p.result(1,80,S);
    assert(p.nextCheck()==2);p.result(2,20,S);assert(!p.analyzing());
    for (int i=2;i<16;++i) assert(p.tick(i*S,ProxyPool::Connection::Connecting)==-1);
    assert(p.tick(16*S,ProxyPool::Connection::Connecting)==2);
    p.switched(2,16*S);
    for (int i=17;i<31;++i) assert(p.tick(i*S,ProxyPool::Connection::Connecting)==-1);
    assert(p.tick(31*S,ProxyPool::Connection::Connecting)==1); // Previous failed proxy is cooling down despite its old ping.
    p.switched(1,31*S);
    for (int i=32;i<100;++i) assert(p.tick(i*S,ProxyPool::Connection::Ready)==-1); // No churn toward faster healthy entries.
    assert(p.tick(110*S,ProxyPool::Connection::Connecting)==-1); // Resume invalidates old measurements and grace period.
    for (const auto& h:p.health) assert(h.state!=ProxyPool::State::Available);
    for (int i=111;i<128;++i) assert(p.tick(i*S,ProxyPool::Connection::Offline)==-1);
    assert(!p.analyzing());
    p.analyze();assert(p.nextCheck()==0);p.result(0,-1,128*S);
    assert(p.nextCheck()==1);p.result(1,-1,128*S);
    assert(p.nextCheck()==2);p.result(2,-1,128*S);
    for (int i=129;i<160;++i) assert(p.tick(i*S,ProxyPool::Connection::Connecting)==-1); // No direct fallback.
    p.configure(entries,1,true,false,160*S);p.analyze();
    assert(p.nextCheck()==0);p.cancelAnalysis();p.result(0,1,161*S);
    assert(p.health[0].state==ProxyPool::State::Unknown); // Cancelled generation cannot publish a result.
    p.configure({},0,true,true,162*S);assert(p.active==-1 && p.tick(163*S,ProxyPool::Connection::Connecting)==-1);
    entries.resize(40);p.configure(entries,100,true,true,164*S);assert(p.entries.size()==ProxyPool::Limit && p.active==15);
    entries.resize(3);
    for (int delay:ProxyPool::SwitchSeconds) {
        ProxyPool timed;timed.configure(entries,0,true,true,S,delay);
        assert(timed.tick(S,ProxyPool::Connection::Connecting)==-1);
        for (int i=0;i<3;++i) { assert(timed.nextCheck()==i);timed.result(i,10+i,S); }
        for (int t=2;t<=delay;++t) assert(timed.tick(t*S,ProxyPool::Connection::Connecting)==-1);
        assert(timed.tick((delay+1)*S,ProxyPool::Connection::Connecting)==1);
        timed.configure(entries,0,true,false,S,delay);
        for (int t=1;t<=delay+2;++t) assert(timed.tick(t*S,ProxyPool::Connection::Connecting)==-1);
    }
    assert(ProxyPool::switchIndex(-1)==1 && ProxyPool::switchIndex(999)==1);
    for (int i=0;i<5;++i) {
        const int x=ProxyListLayout::IntervalLeft+i*ProxyListLayout::IntervalPitch;
        const int y=ProxyListLayout::Top+2*ProxyListLayout::Pitch+ProxyListLayout::IntervalTop;
        assert(ProxyListLayout::intervalHit(x,y,2,7)==i);
        assert(ProxyListLayout::intervalHit(x+ProxyListLayout::IntervalWidth-1,y+25,2,7)==i);
        assert(ProxyListLayout::intervalHit(x+ProxyListLayout::IntervalWidth,y,2,7)==-1);
        assert(ProxyListLayout::intervalHit(x,y-1,2,7)==-1);
        assert(ProxyListLayout::intervalHit(x,y+26,2,7)==-1);
    }
    assert(ProxyListLayout::intervalHit(649,180,2,7)==-1 && ProxyListLayout::intervalHit(920,180,2,7)==-1);
    const int count=ProxyListLayout::Fixed+ProxyPool::Limit;
    for (int total=ProxyListLayout::Fixed;total<=count;++total) {
        for (int selected=0;selected<total;++selected) {
            if (selected==2) continue;
            const int first=ProxyListLayout::first(selected,total,false);
            for (int slot=0;slot<ProxyListLayout::Visible;++slot) {
                const int visible=first+slot;
                const int expected=visible<total-1 ? ProxyListLayout::logicalRow(visible,false) : -1;
                const int y=ProxyListLayout::Top+slot*ProxyListLayout::Pitch;
                assert(expected!=2);
                assert(ProxyListLayout::hit(24,y,selected,total,false)==expected);
                assert(ProxyListLayout::hit(935,y+31,selected,total,false)==expected);
                assert(ProxyListLayout::hit(24,y+32,selected,total,false)==-1);
            }
            assert(ProxyListLayout::next(selected,1,total,false)!=2);
            assert(ProxyListLayout::next(selected,-1,total,false)!=2);
            assert(ProxyListLayout::intervalHit(660,180,selected,total,false)==-1);
        }
    }
    assert(ProxyListLayout::next(1,1,count,false)==3);
    assert(ProxyListLayout::next(3,-1,count,false)==1);
    for (int selected=0;selected<count;++selected) {
        int first=ProxyListLayout::first(selected,count);
        for (int row=0;row<ProxyListLayout::Visible;++row) {
            int expected=first+row<count?first+row:-1,y=ProxyListLayout::Top+row*ProxyListLayout::Pitch;
            assert(ProxyListLayout::hit(24,y,selected,count)==expected);
            assert(ProxyListLayout::hit(935,y+31,selected,count)==expected);
            assert(ProxyListLayout::hit(24,y+32,selected,count)==-1);
            assert(ProxyListLayout::hit(936,y,selected,count)==-1);
        }
        assert(ProxyListLayout::hit(24,103,selected,count)==-1);
        assert(ProxyListLayout::hit(24,428,selected,count)==-1);
    }
    assert(ProxyListLayout::hit(25,104+8*36,0,ProxyListLayout::Fixed)==-1);
    for (int i=0;i<4;++i) {
        assert(ProxyListLayout::footerHit(24+i*228,500)==i);
        assert(ProxyListLayout::footerHit(24+i*228+223,539)==i);
        assert(ProxyListLayout::footerHit(24+i*228+224,520)==-1);
    }
    assert(ProxyListLayout::footerHit(24,540)==-1);
    std::cout<<"Proxy pool: sequential health, failure grace, latency selection, cooldown, offline/resume, stale results and shared list geometry PASS\n";
}
