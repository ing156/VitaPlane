#pragma once
#include "settings.h"
#include <atomic>
#include <pthread.h>

// One pending settings snapshot; the proxy screen blocks edits until completion.
class ProxySettingsSave {
public:
    ~ProxySettingsSave() { if (active_) pthread_join(thread_,nullptr); }
    bool active() const { return active_; }
    bool begin(const AppSettings& settings) {
        if (active_) return false;
        value_=settings;done_=false;
        pthread_attr_t attr;
        int rc=pthread_attr_init(&attr);
        if (rc) return false;
        rc=pthread_attr_setstacksize(&attr,256*1024);
        if (!rc) rc=pthread_create(&thread_,&attr,[](void* data)->void* {
            auto& job=*static_cast<ProxySettingsSave*>(data);
            job.ok_=job.value_.save();job.done_=true;return nullptr;
        },this);
        pthread_attr_destroy(&attr);active_=rc==0;return active_;
    }
    bool finish(AppSettings& settings,bool& ok) {
        if (!active_ || !done_) return false;
        pthread_join(thread_,nullptr);active_=false;settings=std::move(value_);ok=ok_;return true;
    }
private:
    AppSettings value_;
    pthread_t thread_{};
    std::atomic<bool> done_{false};
    bool active_=false,ok_=false;
};
