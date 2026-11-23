set(tgcalls_source "${CMAKE_CURRENT_SOURCE_DIR}/../tgcalls/tgcalls")
add_library(tgcalls_vita_probe STATIC)
target_compile_features(tgcalls_vita_probe PRIVATE cxx_std_20)
target_compile_definitions(tgcalls_vita_probe PRIVATE WEBRTC_APP_TDESKTOP TGCALLS_USE_STD_OPTIONAL RTC_ENABLE_VP9)
target_compile_options(tgcalls_vita_probe PRIVATE -Wno-psabi)
target_include_directories(tgcalls_vita_probe PRIVATE "${tgcalls_source}" "${OPENSSL_INCLUDE_DIR}" ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(tgcalls_vita_probe PRIVATE tg_owt tg_owt::librnnoise)
file(GLOB tgcalls_call_sources "${tgcalls_source}/*.cpp" "${tgcalls_source}/v2/*.cpp")
list(FILTER tgcalls_call_sources EXCLUDE REGEX "(Test|CompatImpl|SignalingTranslator|FakeVideoTrackSource|ThreadLocalObject)\\.cpp$")
target_sources(tgcalls_vita_probe PRIVATE ${tgcalls_call_sources}
    "${tgcalls_source}/utils/gzip.cpp"
    "${tgcalls_source}/platform/fake/FakeInterface.cpp"
    "${tgcalls_source}/third-party/json11.cpp"
)

