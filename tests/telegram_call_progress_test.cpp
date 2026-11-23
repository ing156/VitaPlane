#include "telegram_call_setup.h"

#include <openssl/crypto.h>
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <thread>

namespace {
constexpr size_t kQueueLimit = 64;
using Packet = std::vector<uint8_t>;
const char* field(const json_t* object, const char* key) {
    const auto text = json_string_value(json_object_get(object, key));
    return text ? text : "";
}
bool is(const json_t* object, const char* type) { return std::strcmp(field(object, "@type"), type) == 0; }
int32_t call_id(const json_t* object, const char* key) { return json_integer_value(json_object_get(object, key)); }
Packet decode_packet(const json_t*) { return {1}; }
int64_t sceKernelGetSystemTimeWide() { return 1; }
}

struct TelegramClient {
    enum class AudioCallPhase { Incoming, Dialing, Connecting, Ended, Failed };
    struct CallSession {
        int32_t id = 0;
        int64_t user = 31, connecting_until = 0;
        uint64_t request = 1;
        bool cancelled = false, terminal = false, stop = false, overflow = false, ice_started = false;
        bool is_outgoing = true, accepted = false;
        std::shared_ptr<json_t> early_update;
        std::thread worker;
        std::mutex mutex;
        std::condition_variable wake;
        std::deque<Packet> incoming;
        bool low_latency = false;
        static void run_media(const std::shared_ptr<CallSession>& session, telegram_call::Setup, bool low_latency) {
            session->low_latency = low_latency;
        }
    };
    std::shared_ptr<CallSession> call_ = std::make_shared<CallSession>();
    AudioCallPhase audio_call_phase_ = AudioCallPhase::Dialing;
    struct {
        const char* stage = "CREATE";
        const char* failure = "";
        int64_t td_error = 0;
        unsigned rx = 0;
    } audio_call_diagnostic_;
    bool ready_ = true, logging_out_ = false;
    bool call_low_latency_ = false;
    uint64_t call_generation_ = 1;
    int32_t last_incoming_audio_id_ = 0;
    std::shared_ptr<json_t> sent;
    bool audioCallActive() const { return call_ && !call_->terminal && !call_->cancelled; }
    void sendJson(json_t* request) { sent = {request, json_decref}; }
    bool processAudioCall(void*);
    bool acceptAudioCall();
    void discardAudioCall(bool failed) { audio_call_phase_ = failed ? AudioCallPhase::Failed : AudioCallPhase::Ended; }
    void event(const char* json) {
        auto* root = json_loads(json, 0, nullptr);
        assert(root && processAudioCall(root));
        json_decref(root);
    }
};

// CALL_UPDATE_HANDLER
// ACCEPT_CALL
// SEND_DELAY

struct MessageItem {
    enum class CallRecord { None, Finished, Missed, Declined, Disconnected };
    CallRecord call_record = CallRecord::None;
    bool video_call = false, outgoing = false;
    int call_duration = 0;
    std::string text;
};
namespace i18n { std::string text(const char* key) { return key; } }
auto json_str = field;
int64_t json_i53(const json_t* object, const char* key) { return json_integer_value(json_object_get(object, key)); }
MessageItem parse_record(bool outgoing, bool video, const char* reason, int duration) {
    MessageItem out;
    out.outgoing = outgoing;
    const std::shared_ptr<json_t> data(json_pack("{s:b,s:i,s:{s:s}}", "is_video", video, "duration", duration,
        "discard_reason", "@type", reason), json_decref);
    const auto* content = data.get();
    // PARSE_RECORD
    return out;
}

int main() {
    assert(packet_delay_ms(10, 0.025, 5, 0.010) == 3);
    assert(packet_delay_ms(400, 0.0, 350, 0.0) == 0);
    assert(packet_delay_ms(10, 0.025, 10, 0.025) == -1);
    assert(packet_delay_ms(1, 0.0, 10, 0.025) == -1);
    assert(packet_delay_ms(12, 0.020, 10, 0.025) == -1);
    assert(packet_delay_ms(12, std::numeric_limits<double>::quiet_NaN(), 10, 0.025) == -1);
    using Phase = TelegramClient::AudioCallPhase;
    TelegramClient tg;
    tg.event(R"({"@type":"updateCall","call":{"id":9,"user_id":31,"is_outgoing":true,"state":{"@type":"callStatePending","is_created":true,"is_received":true}}})");
    assert(tg.call_->early_update && tg.call_->id == 0);
    tg.event(R"({"@type":"callId","id":9,"@extra":{"kind":"audio_call_create","generation":1}})");
    assert(!tg.call_->early_update && tg.call_->id == 9 && std::string(tg.audio_call_diagnostic_.stage) == "RING");
    tg.event(R"({"@type":"updateCall","call":{"id":8,"user_id":31,"is_outgoing":true,"state":{"@type":"callStateError","error":{"code":403}}}})");
    assert(tg.audio_call_phase_ == Phase::Dialing && !tg.call_->terminal);
    tg.event(R"({"@type":"updateCall","call":{"id":9,"user_id":31,"is_outgoing":true,"state":{"@type":"callStateExchangingKeys"}}})");
    assert(tg.audio_call_phase_ == Phase::Connecting && std::string(tg.audio_call_diagnostic_.stage) == "KEYS");
    tg.event(R"({"@type":"updateNewCallSignalingData","call_id":9})");
    assert(tg.audio_call_diagnostic_.rx == 1);
    tg.event(R"({"@type":"error","code":406,"@extra":{"kind":"audio_call_signal","generation":0}})");
    assert(tg.audio_call_diagnostic_.td_error == 0);
    tg.event(R"({"@type":"updateCall","call":{"id":9,"user_id":31,"is_outgoing":true,"state":{"@type":"callStateError","error":{"code":406}}}})");
    assert(tg.audio_call_phase_ == Phase::Failed && tg.call_->terminal && tg.audio_call_diagnostic_.td_error == 406);

    tg.call_.reset();
    tg.event(R"({"@type":"updateCall","call":{"id":12,"user_id":31,"is_outgoing":false,"is_video":true,"state":{"@type":"callStatePending"}}})");
    assert(!tg.call_);
    tg.event(R"({"@type":"updateCall","call":{"id":13,"user_id":31,"is_outgoing":false,"state":{"@type":"callStatePending"}}})");
    assert(tg.audio_call_phase_ == Phase::Incoming && tg.call_->id == 13 && !tg.call_->is_outgoing);
    tg.event(R"({"@type":"updateCall","call":{"id":13,"user_id":31,"is_outgoing":false,"state":{"@type":"callStateReady"}}})");
    assert(!tg.call_->worker.joinable() && !tg.call_->accepted && tg.audio_call_phase_ == Phase::Incoming);
    assert(tg.acceptAudioCall());
    assert(is(tg.sent.get(), "acceptCall") && call_id(tg.sent.get(), "call_id") == 13);
    assert(json_is_object(json_object_get(tg.sent.get(), "protocol")));
    assert(tg.call_->accepted && tg.audio_call_phase_ == Phase::Connecting && !tg.acceptAudioCall());
    tg.event(R"({"@type":"error","code":400,"@extra":{"kind":"audio_call_accept","generation":1}})");
    assert(!tg.call_->terminal);
    tg.event(R"({"@type":"error","code":400,"@extra":{"kind":"audio_call_accept","generation":2}})");
    assert(tg.audio_call_phase_ == Phase::Failed && tg.call_->terminal);
    tg.call_.reset();
    tg.event(R"({"@type":"updateCall","call":{"id":13,"user_id":31,"is_outgoing":false,"state":{"@type":"callStatePending"}}})");
    assert(!tg.call_);
    tg.event(R"({"@type":"updateCall","call":{"id":14,"user_id":45,"is_outgoing":false,"state":{"@type":"callStatePending"}}})");
    assert(tg.acceptAudioCall());
    const std::string key = std::string(342, 'A') + "==";
    const std::shared_ptr<json_t> ready(json_pack("{s:s,s:{s:i,s:i,s:{s:s,s:s,s:o,s:b,s:[]}}}",
        "@type", "updateCall", "call", "id", 14, "user_id", 45, "state", "@type", "callStateReady",
        "encryption_key", key.c_str(), "protocol", telegram_call::protocol(), "allow_p2p", 1, "servers"), json_decref);
    tg.call_low_latency_ = true;
    assert(ready && tg.processAudioCall(ready.get()));
    tg.call_low_latency_ = false;
    assert(tg.audio_call_phase_ == Phase::Connecting && tg.call_->worker.joinable() && tg.call_->ice_started);
    tg.call_->worker.join();
    assert(tg.call_->low_latency);
    tg.event(R"({"@type":"updateCall","call":{"id":14,"user_id":45,"is_outgoing":false,"state":{"@type":"callStateDiscarded","reason":{"@type":"callDiscardReasonHungUp"}}}})");
    assert(tg.audio_call_phase_ == Phase::Ended && tg.call_->terminal);

    assert(parse_record(false, false, "callDiscardReasonMissed", -1).text == "call.history_missed");
    assert(parse_record(true, false, "callDiscardReasonMissed", 0).text == "call.history_unanswered");
    const auto completed = parse_record(true, false, "callDiscardReasonHungUp", 21);
    assert(completed.text == "call.history_outgoing" && completed.call_duration == 21);
    assert(parse_record(false, false, "callDiscardReasonDeclined", 0).text == "call.history_declined");
    assert(parse_record(false, true, "callDiscardReasonHungUp", 2).video_call);
}
