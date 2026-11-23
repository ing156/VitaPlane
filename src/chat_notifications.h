#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <jansson.h>

namespace chat_notifications {
inline constexpr const char* scopes[] = {"notificationSettingsScopePrivateChats", "notificationSettingsScopeGroupChats", "notificationSettingsScopeChannelChats"};
inline int scope(json_t* value) {
    const char* type = json_string_value(json_object_get(value, "@type"));
    for (int i = 0; type && i < 3; ++i) if (!std::strcmp(type, scopes[i])) return i;
    return -1;
}
inline int64_t until(json_t* settings) {
    auto* saved = json_object_get(settings, "@mute_until");
    if (json_is_integer(saved)) return std::max<int64_t>(0, json_integer_value(saved));
    const auto seconds = std::clamp<int64_t>(json_integer_value(json_object_get(settings, "mute_for")), 0, INT32_MAX);
    return seconds ? (int64_t)std::time(nullptr)+seconds : 0;
}
inline void freeze(json_t* settings) {
    if (json_is_object(settings)) json_object_set_new(settings, "@mute_until", json_integer(until(settings)));
}
} // namespace chat_notifications
