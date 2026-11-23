#include "telegram_client.h"
#include "telegram_call_setup.h"
#include "vita_call_audio.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <utility>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <psp2/kernel/threadmgr.h>

#include "Instance.h"
#include "StaticThreads.h"
#include "rtc_base/thread.h"
#include "v2/InstanceV2ReferenceImpl.h"

// The SDK uses this size for std::thread workers when no attributes are passed.
extern "C" {
unsigned int _pthread_stack_default_user = 128 * 1024;
}

namespace {
constexpr size_t kPacketLimit = 16384;
constexpr size_t kQueueLimit = 64;
using Packet = std::vector<uint8_t>;
using IceEvent = tgcalls::Descriptor::IceEvent;

int packet_delay_ms(uint64_t sent, double seconds, uint64_t previous_sent, double previous_seconds) {
    if (sent <= previous_sent || !(seconds >= previous_seconds)) return -1;
    const double milliseconds = (seconds - previous_seconds) * 1000 / (sent - previous_sent);
    return milliseconds >= 0 ? int(std::min(milliseconds, 999999.0) + 0.5) : -1;
}
std::atomic<bool> call_engine_started{false};

const char* field(const json_t* object, const char* key) {
    const auto* text = json_string_value(json_object_get(object, key));
    return text ? text : "";
}

bool is(const json_t* object, const char* type) {
    return std::strcmp(field(object, "@type"), type) == 0;
}

int32_t call_id(const json_t* object, const char* key) {
    const auto id = json_integer_value(json_object_get(object, key));
    return id > 0 && id <= INT32_MAX ? int32_t(id) : 0;
}

Packet decode_packet(const json_t* root) {
    const auto* data = json_object_get(root, "data");
    const auto* encoded = json_string_value(data);
    const auto size = json_string_length(data);
    if (!encoded || size == 0 || size % 4 || size > (kPacketLimit + 2) / 3 * 4) return {};
    Packet packet(size / 4 * 3);
    const int decoded = EVP_DecodeBlock(packet.data(), reinterpret_cast<const uint8_t*>(encoded), int(size));
    if (decoded < 0) return {};
    const size_t padding = (encoded[size - 1] == '=') + (encoded[size - 2] == '=');
    if (size_t(decoded) <= padding || size_t(decoded) - padding > kPacketLimit) return {};
    packet.resize(size_t(decoded) - padding);
    return packet;
}
}

struct TelegramClient::CallSession {
    int32_t id = 0;
    int64_t user = 0, connected_at = 0, connecting_until = 0;
    uint64_t request = 0;
    bool cancelled = false, discarded = false, terminal = false, is_outgoing = true, accepted = false;
    std::shared_ptr<json_t> early_update;
    std::thread worker;
    std::mutex mutex;
    std::condition_variable wake;
    std::deque<Packet> incoming, outgoing;
    std::atomic<int> media_state{int(tgcalls::State::WaitInit)};
    std::atomic<bool> stop{false}, finished{false}, overflow{false};
    std::atomic<bool> muted{false};
    bool ice_started = false;
    std::array<std::atomic<int>, int(IceEvent::Error) + 1> ice{};
    std::string iceProgress() const;
    std::array<std::string, 2> quality();
    VitaAudioStats audio;
    tgcalls::Descriptor::AudioRtpStats rtp;
    int send_queue_ms = -1, peak_send_queue_ms = -1;
    int packet_delay_ms_now = -1, peak_packet_delay_ms = -1;
    static void run_media(const std::shared_ptr<CallSession>& session, telegram_call::Setup ready, bool low_latency);
};

std::string TelegramClient::CallSession::iceProgress() const {
    if (!ice_started) return {};
    auto value = [&](IceEvent event) { return ice[int(event)].load(); };
    return "ICE " + std::to_string(value(IceEvent::State)) +
        " L" + std::to_string(value(IceEvent::LocalHost)) + "/" +
        std::to_string(value(IceEvent::LocalStun)) + "/" + std::to_string(value(IceEvent::LocalRelay)) +
        " R" + std::to_string(value(IceEvent::RemoteCandidate)) +
        " SDP" + std::to_string(value(IceEvent::RemoteSdp)) +
        " E" + std::to_string(value(IceEvent::Error));
}

std::array<std::string, 2> TelegramClient::CallSession::quality() {
    if (!ice_started) return {};
    tgcalls::Descriptor::AudioRtpStats stats;
    std::array<int, 4> send_delay;
    {
        std::lock_guard<std::mutex> lock(mutex);
        stats = rtp;
        send_delay = {send_queue_ms, peak_send_queue_ms, packet_delay_ms_now, peak_packet_delay_ms};
    }
    std::array<std::string, 2> lines;
    lines[0] = "PCM IN " + std::to_string(audio.capture_gap_ms.load()) +
        "ms " + std::to_string(audio.capture_late.load()) + "/" + std::to_string(audio.capture_blocks.load()) +
        " OUT " + std::to_string(audio.playout_gap_ms.load()) +
        "ms/" + std::to_string(audio.playout_late.load()) +
        " PK " + std::to_string(audio.capture_peak.load()) +
        " CLIP " + std::to_string(audio.clipped_samples.load());
    if (stats.receivedJitterMs >= 0 || stats.sentJitterMs >= 0) {
        const auto direction = [](int lost, int jitter_ms) {
            return jitter_ms < 0 ? std::string("-") : std::to_string(lost) + "/" + std::to_string(jitter_ms) + "ms";
        };
        lines[1] = "RTP RX " + direction(stats.receivedLost, stats.receivedJitterMs) +
            " TX " + direction(stats.sentLost, stats.sentJitterMs);
        if (stats.receivedJitterMs >= 0 && stats.receivedSamples) {
            char plc[32];
            const double ratio = 100.0 * std::min(stats.concealed, stats.receivedSamples) / stats.receivedSamples;
            std::snprintf(plc, sizeof(plc), " PLC %.1f%%", ratio);
            lines[1] += plc;
            lines[1] += " D" + std::to_string(stats.discarded);
        }
    }
    const auto add_delay = [&](const char* label, int current, int peak) {
        if (peak < 0) return;
        if (lines[1].empty()) lines[1] = "RTP";
        lines[1] += std::string(label) + (current < 0 ? "-" : std::to_string(current)) +
            "/" + std::to_string(peak) + "ms";
    };
    add_delay(" Q", send_delay[0], send_delay[1]);
    add_delay(" S", send_delay[2], send_delay[3]);
    return lines;
}

bool TelegramClient::canStartAudioCall() const {
    if (!ready_ || !account_user_id_ || call_ || logging_out_ || !current_chat_id_) return false;
    for (const auto& chat : known_chats_) {
        if (chat.id != current_chat_id_ || chat.id != chat.user_id || chat.user_id <= 0 || chat.user_id == account_user_id_) continue;
        const auto user = user_presence_.find(chat.user_id);
        return user == user_presence_.end() || !user->second.bot;
    }
    return false;
}

bool TelegramClient::startAudioCall() {
    audio_call_phase_ = AudioCallPhase::Failed;
    audio_call_diagnostic_ = {"CREATE", "LOCAL"};
    if (!canStartAudioCall()) return false;
    call_ = std::make_shared<CallSession>();
    call_->user = current_chat_id_;
    call_->request = ++call_generation_;
    auto* request = json_pack("{s:s,s:I,s:b,s:o,s:o}", "@type", "createCall", "user_id",
        static_cast<json_int_t>(call_->user), "is_video", 0, "protocol", telegram_call::protocol(), "@extra",
        json_pack("{s:s,s:I}", "kind", "audio_call_create", "generation", static_cast<json_int_t>(call_->request)));
    if (!request) { call_.reset(); return false; }
    audio_call_phase_ = AudioCallPhase::Dialing;
    audio_call_diagnostic_.failure = "";
    sendJson(request);
    return true;
}

int64_t TelegramClient::audioCallPeer() const {
    return call_ ? call_->user : 0;
}

bool TelegramClient::acceptAudioCall() {
    if (!audioCallActive() || audio_call_phase_ != AudioCallPhase::Incoming || !ready_ || logging_out_) return false;
    auto* request = json_pack("{s:s,s:i,s:o,s:o}", "@type", "acceptCall", "call_id", call_->id,
        "protocol", telegram_call::protocol(), "@extra",
        json_pack("{s:s,s:I}", "kind", "audio_call_accept", "generation", static_cast<json_int_t>(call_->request)));
    if (!request) return false;
    call_->accepted = true;
    audio_call_phase_ = AudioCallPhase::Connecting;
    audio_call_diagnostic_.stage = "ACCEPT";
    sendJson(request);
    return true;
}

std::string TelegramClient::audioCallDiagnostic() const {
    const auto& info = audio_call_diagnostic_;
    std::string line = std::string("TG ") + info.stage;
    if (*info.failure) line += std::string(" / ") + info.failure;
    if (info.td_error) line += " " + std::to_string(info.td_error);
    if (info.tx || info.rx) line += "  TX " + std::to_string(info.tx) + " / RX " + std::to_string(info.rx);
    return line;
}

std::string TelegramClient::audioCallNetworkDiagnostic() const {
    return call_ ? call_->iceProgress() : audio_call_diagnostic_.ice;
}

const std::array<std::string, 2>& TelegramClient::audioCallQuality() {
    auto& diagnostics = audio_call_diagnostic_;
    const auto now = sceKernelGetSystemTimeWide();
    if (call_ && now >= diagnostics.quality_at) {
        diagnostics.quality = call_->quality();
        diagnostics.quality_at = now + 500000;
    }
    return diagnostics.quality;
}

bool TelegramClient::audioCallActive() const {
    return call_ && !call_->terminal && !call_->cancelled;
}

bool TelegramClient::audioCallMuted() const {
    return call_ && call_->muted.load();
}

void TelegramClient::toggleAudioCallMute() {
    if (!audioCallActive()) return;
    call_->muted = !call_->muted.load();
    call_->wake.notify_one();
}

void TelegramClient::discardAudioCall(bool disconnected) {
    if (!call_) return;
    auto& session = *call_;
    audio_call_phase_ = disconnected ? AudioCallPhase::Failed : AudioCallPhase::Ended;
    session.cancelled = true;
    session.stop = true;
    session.wake.notify_one();
    if (session.id && !session.discarded && ready_ && !logging_out_) {
        session.discarded = true;
        const auto duration = session.connected_at ? (sceKernelGetSystemTimeWide() - session.connected_at) / 1000000 : 0;
        sendJson(json_pack("{s:s,s:i,s:b,s:s,s:i,s:b,s:s}", "@type", "discardCall", "call_id", session.id,
            "is_disconnected", disconnected, "invite_link", "", "duration", int(duration), "is_video", 0, "connection_id", "0"));
        session.terminal = true;
    }
}

void TelegramClient::CallSession::run_media(const std::shared_ptr<CallSession>& session,
                                            telegram_call::Setup ready, bool low_latency) {
    const auto fail = [&] { session->media_state = int(tgcalls::State::Failed); };
    try {
        static const bool registered = tgcalls::Register<tgcalls::InstanceV2ReferenceImpl>();
        if (registered && !session->stop) {
            auto key = std::make_shared<std::array<uint8_t, 256>>(ready.key);
            OPENSSL_cleanse(ready.key.data(), ready.key.size());
            tgcalls::Descriptor setup{.version = ready.version, .initialNetworkType = tgcalls::NetworkType::WiFi,
                                       .encryptionKey = {key, session->is_outgoing}};
            setup.config.enableP2P = ready.allow_p2p;
            setup.config.maxApiLayer = ready.max_layer;
            setup.config.initializationTimeout = 20;
            setup.config.receiveTimeout = 20;
            setup.config.customParameters = std::move(ready.custom_parameters);
            for (auto& server : ready.servers) {
                setup.rtcServers.push_back({server.reflector_id, std::move(server.host), server.port,
                    std::move(server.username), std::move(server.password), server.turn, false});
            }
            setup.lowAudioLatency = low_latency;
            setup.createAudioDeviceModule = [session, low_latency](webrtc::TaskQueueFactory* factory) {
                return CreateVitaAudioDevice(factory, session->audio, low_latency);
            };
            setup.stateUpdated = [session](tgcalls::State state) { session->media_state = int(state); };
            setup.iceEvent = [session](IceEvent event, int result) {
                auto& value = session->ice[int(event)];
                if (event == IceEvent::State || event == IceEvent::RemoteSdp || event == IceEvent::Error) value = result;
                else ++value;
            };
            setup.audioRtpStats = [session](const tgcalls::Descriptor::AudioRtpStats& stats) {
                std::lock_guard<std::mutex> lock(session->mutex);
                session->packet_delay_ms_now = packet_delay_ms(
                    stats.sentPackets, stats.sentDelaySeconds, session->rtp.sentPackets, session->rtp.sentDelaySeconds);
                session->peak_packet_delay_ms = std::max(session->peak_packet_delay_ms, session->packet_delay_ms_now);
                session->rtp = stats;
            };
            setup.sendQueueDelay = [session](int64_t delay_ms) {
                std::lock_guard<std::mutex> lock(session->mutex);
                session->send_queue_ms = int(std::clamp<int64_t>(delay_ms, 0, 999999));
                session->peak_send_queue_ms = std::max(session->peak_send_queue_ms, session->send_queue_ms);
            };
            setup.signalingDataEmitted = [session](const Packet& packet) {
                std::lock_guard<std::mutex> lock(session->mutex);
                if (session->stop) return;
                if (packet.size() > kPacketLimit || session->outgoing.size() >= kQueueLimit) session->overflow = true;
                else session->outgoing.push_back(packet);
            };
            call_engine_started = true;
            auto engine = tgcalls::Meta::Create(ready.version, std::move(setup));
            if (!engine) fail();
            else {
                bool muted = false;
                while (!session->stop && !session->overflow && session->media_state != int(tgcalls::State::Failed)) {
                    const bool requested_mute = session->muted;
                    if (muted != requested_mute) {
                        engine->setMuteMicrophone(requested_mute);
                        muted = requested_mute;
                    }
                    Packet packet;
                    {
                        std::unique_lock<std::mutex> lock(session->mutex);
                        session->wake.wait_for(lock, std::chrono::milliseconds(100), [&] {
                            return session->stop || session->overflow || !session->incoming.empty() || session->muted != muted;
                        });
                        if (!session->stop && !session->incoming.empty()) {
                            packet = std::move(session->incoming.front());
                            session->incoming.pop_front();
                        }
                    }
                    if (!packet.empty()) engine->receiveSignalingData(packet);
                }
                const auto stopped = std::make_shared<std::atomic<bool>>(false);
                engine->stop([stopped](tgcalls::FinalState) { *stopped = true; });
                const auto until = sceKernelGetSystemTimeWide() + 2000000;
                while (!*stopped && sceKernelGetSystemTimeWide() < until) sceKernelDelayThread(10000);
                engine.reset();
            }
        } else if (!registered) fail();
    } catch (const std::exception&) {
        fail();
    }
    OPENSSL_cleanse(ready.key.data(), ready.key.size());
    session->finished = true;
}

bool TelegramClient::processAudioCall(void* opaque) {
    auto* root = static_cast<json_t*>(opaque);
    const auto* extra = json_object_get(root, "@extra");
    const auto* kind = field(extra, "kind");
    const bool creating = std::strcmp(kind, "audio_call_create") == 0;
    if (creating || std::strcmp(kind, "audio_call_accept") == 0) {
        if (!call_ || uint64_t(json_integer_value(json_object_get(extra, "generation"))) != call_->request) return true;
        if (creating && is(root, "callId")) {
            const auto id = call_id(root, "id");
            if (id && !call_->id) {
                call_->id = id;
                audio_call_diagnostic_.stage = "REQUEST";
                if (auto early = std::move(call_->early_update)) processAudioCall(early.get());
            }
            if (call_->cancelled) discardAudioCall(false);
        } else if (is(root, "error")) {
            if (!call_->cancelled) {
                audio_call_phase_ = AudioCallPhase::Failed;
                audio_call_diagnostic_.failure = "TD";
                audio_call_diagnostic_.td_error = json_integer_value(json_object_get(root, "code"));
            }
            call_->terminal = true;
            call_->stop = true;
            call_->wake.notify_one();
        }
        return true;
    }
    if (std::strcmp(kind, "audio_call_signal") == 0) {
        if (is(root, "error") && call_ && uint64_t(json_integer_value(json_object_get(extra, "generation"))) == call_->request) {
            audio_call_diagnostic_.failure = "SIGNAL";
            audio_call_diagnostic_.td_error = json_integer_value(json_object_get(root, "code"));
        }
        return true;
    }
    if (is(root, "updateNewCallSignalingData")) {
        if (!call_ || call_->cancelled || call_id(root, "call_id") != call_->id) return true;
        auto packet = decode_packet(root);
        if (packet.empty()) return true;
        std::lock_guard<std::mutex> lock(call_->mutex);
        if (call_->incoming.size() >= kQueueLimit) call_->overflow = true;
        else {
            call_->incoming.push_back(std::move(packet));
            ++audio_call_diagnostic_.rx;
        }
        call_->wake.notify_one();
        return true;
    }
    if (!is(root, "updateCall")) return false;
    const auto* call = json_object_get(root, "call");
    const auto id = call_id(call, "id");
    const auto user = json_integer_value(json_object_get(call, "user_id"));
    const bool outgoing = json_is_true(json_object_get(call, "is_outgoing"));
    const auto* state = json_object_get(call, "state");
    if (!id || user <= 0) return true;
    if (!call_ && !outgoing && ready_ && !logging_out_ && id != last_incoming_audio_id_ &&
        !json_is_true(json_object_get(call, "is_video")) && is(state, "callStatePending")) {
        call_ = std::make_shared<CallSession>();
        call_->id = id;
        call_->user = user;
        call_->is_outgoing = false;
        call_->request = ++call_generation_;
        last_incoming_audio_id_ = id;
        audio_call_phase_ = AudioCallPhase::Incoming;
        audio_call_diagnostic_ = {"INCOMING", ""};
        return true;
    }
    if (!call_ || user != call_->user || outgoing != call_->is_outgoing) return true;
    // TDLib can emit an update from the call actor before it returns the request ID.
    if (!call_->id) {
        call_->early_update = std::shared_ptr<json_t>(json_incref(root), json_decref);
        return true;
    }
    if (call_->id != id) return true;
    if (is(state, "callStateDiscarded") || is(state, "callStateError")) {
        call_->terminal = true;
        call_->stop = true;
        call_->wake.notify_one();
        if (!call_->cancelled) {
            if (is(state, "callStateError")) {
                audio_call_phase_ = AudioCallPhase::Failed;
                audio_call_diagnostic_.failure = "TD";
                audio_call_diagnostic_.td_error = json_integer_value(json_object_get(json_object_get(state, "error"), "code"));
            } else {
                audio_call_phase_ = AudioCallPhase::Ended;
                const auto* reason = json_object_get(state, "reason");
                if (is(reason, "callDiscardReasonMissed")) audio_call_diagnostic_.failure = "MISSED";
                else if (is(reason, "callDiscardReasonDeclined")) audio_call_diagnostic_.failure = "DECLINED";
                else if (is(reason, "callDiscardReasonDisconnected")) audio_call_diagnostic_.failure = "DISCONNECTED";
            }
        }
    } else if (call_->cancelled) discardAudioCall(false);
    else if (!outgoing && !call_->accepted) {
        // Only the local Accept action may start this device's microphone.
        return true;
    } else if (is(state, "callStatePending") && outgoing) {
        if (json_is_true(json_object_get(state, "is_received"))) audio_call_diagnostic_.stage = "RING";
        else if (json_is_true(json_object_get(state, "is_created"))) audio_call_diagnostic_.stage = "SERVER";
    } else if (is(state, "callStateExchangingKeys")) {
        audio_call_diagnostic_.stage = "KEYS";
        audio_call_phase_ = AudioCallPhase::Connecting;
    } else if (is(state, "callStateReady") && !call_->worker.joinable()) {
        audio_call_diagnostic_.stage = "MEDIA";
        telegram_call::Setup ready;
        if (json_is_true(json_object_get(call, "is_video")) || !telegram_call::parse_ready(state, ready)) {
            audio_call_diagnostic_.failure = "SETUP";
            discardAudioCall(true);
        } else {
            call_->ice_started = true;
            audio_call_phase_ = AudioCallPhase::Connecting;
            call_->connecting_until = sceKernelGetSystemTimeWide() + 25000000;
            try { call_->worker = std::thread(CallSession::run_media, call_, std::move(ready), call_low_latency_); }
            catch (const std::exception&) {
                audio_call_diagnostic_.failure = "THREAD";
                discardAudioCall(true);
            }
        }
        OPENSSL_cleanse(ready.key.data(), ready.key.size());
    }
    return true;
}

void TelegramClient::pumpAudioCall() {
    if (!call_) return;
    auto& session = *call_;
    if (!ready_ || logging_out_) {
        audio_call_phase_ = AudioCallPhase::Ended;
        session.terminal = true;
        session.stop = true;
        session.wake.notify_one();
    }
    if (!session.terminal && !session.cancelled) {
        const auto now = sceKernelGetSystemTimeWide();
        const auto state = session.media_state.load();
        const bool connecting_timed_out = state != int(tgcalls::State::Established) && !session.connected_at &&
            session.connecting_until && now >= session.connecting_until;
        if (session.overflow || state == int(tgcalls::State::Failed) || connecting_timed_out ||
            session.audio.read_result < 0 || session.audio.write_result < 0) {
            if (!*audio_call_diagnostic_.failure) {
                audio_call_diagnostic_.failure = session.overflow ? "QUEUE" :
                    session.audio.read_result < 0 || session.audio.write_result < 0 ? "AUDIO" :
                    connecting_timed_out ? "TIMEOUT" : "ENGINE";
            }
            discardAudioCall(true);
        } else {
            if (!session.connected_at && state == int(tgcalls::State::Established)) {
                session.connected_at = now;
                audio_call_phase_ = AudioCallPhase::Connected;
                audio_call_diagnostic_.stage = "CONNECTED";
            }
            std::deque<Packet> packets;
            {
                std::lock_guard<std::mutex> lock(session.mutex);
                packets.swap(session.outgoing);
            }
            for (const auto& packet : packets) {
                std::string encoded((packet.size() + 2) / 3 * 4 + 1, '\0');
                EVP_EncodeBlock(reinterpret_cast<uint8_t*>(encoded.data()), packet.data(), int(packet.size()));
                sendJson(json_pack("{s:s,s:i,s:s,s:o}", "@type", "sendCallSignalingData", "call_id", session.id,
                    "data", encoded.c_str(), "@extra", json_pack("{s:s,s:I}", "kind", "audio_call_signal",
                    "generation", static_cast<json_int_t>(session.request))));
                ++audio_call_diagnostic_.tx;
            }
        }
    }
    if (session.terminal && (!session.worker.joinable() || session.finished)) {
        if (session.worker.joinable()) session.worker.join();
        audio_call_diagnostic_.ice = session.iceProgress();
        audio_call_diagnostic_.quality = session.quality();
        call_.reset();
    }
}

void TelegramClient::stopAudioCallWorker() {
    if (!call_) return;
    discardAudioCall(false);
    if (call_->worker.joinable()) call_->worker.join();
    call_.reset();
}

bool TelegramClient::shutdownAudioCallTransport() {
    if (!call_engine_started.exchange(false)) return true;
    auto& threads = tgcalls::StaticThreads::getThreads();
    const auto disposed = std::make_shared<std::atomic<bool>>(false);
    threads->getMediaThread()->PostTask([disposed] { *disposed = true; });
    const auto until = sceKernelGetSystemTimeWide() + 2000000;
    while (!*disposed && sceKernelGetSystemTimeWide() < until) sceKernelDelayThread(10000);
    if (!*disposed) return false;
    threads.reset();
    return true;
}
