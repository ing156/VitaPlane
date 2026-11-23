# Native checks

HTTPS DNS transport diagnostics (0.39.25): `bash tests/run-td-dns-tests.sh`
also compiles the pinned SocketFd implementation, injects ECONNRESET at its read
boundary and checks the numeric transport event before TDLib wraps the error.
Hook tests check TLS error/verification/reason fields, IDs and errno preservation.
Transfer and proxy-diagnostics suites verify the app bridge builds and the log
remains bounded and worker-written. Physical Vita TCP/TLS behavior still needs
the new proxy.log; these checks do not establish live proxy connectivity.

DNS bootstrap (0.39.23): `bash tests/run-td-dns-tests.sh` also verifies that
dns.google:443 resolves to the fixed resolver IP without getaddrinfo, even when
system DNS fails, while other ports retain normal resolution. The WEB relay
suite rejects every hostname lookup, redirects the numeric bootstrap IP to its
local TLS fixture and verifies fallback success, DNS caching and rejection of a
wrong TLS hostname. No live proxies are used; Vita connectivity still needs a
device check.

DNS diagnostics (0.39.21): `bash tests/run-td-dns-tests.sh` builds the pinned TDLib
IPAddress implementation on the host with the diagnostic hooks enabled. It mocks
only getaddrinfo/freeaddrinfo and verifies success, resolver error/errno, numeric
trace IDs, HTTPS result recording and cancellation. Requires CMake, C++17, OpenSSL
and zlib development files. `bash tests/run-proxy-diagnostics-tests.sh` checks the
new records in the bounded worker-written log. Real Vita DNS, TLS and MTProto
connectivity still require the automatic proxy.log from a device run.

Recovery comparison (0.39.19): `bash tests/run-transfer-tests.sh` verifies that
production TDLib parameters disable all SQLite databases, as in 0.39.05, while
retaining the account DB path. `bash tests/run-tdlib-disabled-db-tests.sh` compiles
the pinned TDLib disable/migration branches for Vita and other platforms, checking
that Vita preserves the existing file index and encryption key. Actual removal
of the actor stalls requires a physical Vita comparison with the uploaded logs.

Direct document downloads (0.39.62): the transfer suite checks a same-directory
rename by inode identity, restoration of a 128 KiB confirmed prefix with a new
TDLib file ID, priority changes and cleanup after cancellation. The bottom-panel
suite exercises touch controls and controller routing. On Vita, compare the
finished document with its source after interrupting and relaunching a download,
and check priority changes with two active files on the same connection.

Byte resume and panel polish (0.39.09; temporarily disabled in 0.39.19):
`bash tests/run-tdlib-file-resume-tests.sh` builds selected pinned TDLib host targets
and reopens SQLite in a separate process, checking the production partial-file
codec, sparse ready masks, part sizes, holes and readable-prefix calculations.
It needs CMake, host C/C++17, OpenSSL development files and the prepared TDLib
headers; it does not simulate a live FileManager/network download.
Vault and media-trash suites check that both TDLib temp trees survive migration,
sealing and stale rotation flags. Bottom-panel/controller and chat-scroll suites
verify the larger reserved edge, eight-row bounds and Triangle refresh routing.
On Vita with the file database enabled, begin a NEW download, close after several chunks, relaunch and
check reused progress and the final file contents; older versions did not save
TDLib part maps. Light/dark/custom palettes, 130% fonts, actual swipes, network
throughput and power-loss durability still require physical-device checks.

Download recovery/panel (0.39.08): `bash tests/run-transfer-tests.sh` checks journal
publication during interaction, waiting for live chats, transient errors/backoff,
bounded attempts, late replies, persisted pause/resume and completion/preflight
isolation. `bash tests/run-bottom-downloads-tests.sh` checks the shared panel/list
geometry, stable file selection, carousel swipe ownership and cancellation.
Run chat-scroll, audio-playback, i18n and performance suites for shared input,
music controls, all 19 catalogs and the numeric document diagnostics.
On Vita check two simultaneous downloads with music: vertical title/edge swipes,
tap/list/controller pause/resume, completion while a row is pressed, app restart,
Classic/Combined, bot toolbar, light/dark/custom palettes and all font sizes.
Actual partial-byte reuse after TDLib exits and card/network speed remain device
checks. The original 0.39.07 logs have no document restore events; they establish
successful launches, not the specific cause of the failed download restoration.

Performance and recovery (0.39.07): `bash tests/run-image-tests.sh` checks target
sizes, JPEG RGBA output, scaled WebP alpha, per-variant retry cooldowns and
preemption of neighboring-avatar work by visible requests without publishing a
cancelled result. `bash tests/run-https-resume-tests.sh` uses the production HTTPS
client against a local TLS server to check ranges, validators, redirects, safe
full-download fallback, malformed headers and truncated transfers. Update core,
updater lifecycle and WebKit add-on suites cover signature reset, saved-prefix
verification and compatibility. Transfer checks cover the encrypted document job
journal, account ownership, fresh file IDs on restoration and asynchronous export
preflight. Host checks do not establish partial TDLib reuse after process exit,
Vita image quality, CDRAM pressure, storage/network throughput or installation.

`bash tests/run-video-tests.sh` also compiles the production video-note routing
with fake decoders: navigation, independent GIF/sticker/upload previews, reopening
without restarting, browsing another note before Play, background seek/volume and
exclusive audio playback. Transfer checks verify that background video downloads
survive navigation and are cancelled when the player closes. Actual decoding,
sound continuity and floating-circle controls across menus still need a physical Vita.

The skin suite holds the palette metadata operation blocked on a fake device:
UI polling must remain responsive and an old result cannot overwrite a newer
manual palette. It also checks asynchronous same-size edits, deletion, invalid
files and theme switches. `run-download-stream-tests.sh` exercises both POSIX
and mocked Vita positioned reads, including concurrent audio/video, interruption,
I/O errors, sparse offsets up to 1 GiB, completion and rename. On real Vita,
check palette changes and deletion, theme switching, long slow 720p downloads,
pausing/seeking, and completed as well as encrypted media. Host mocks cannot
establish kernel file-handle behavior or card throughput.

Streaming video and its audio each use a bounded 4 MiB RAM read-ahead queue
(plus one 256 KiB read in flight). The download-stream suite stalls the reader
while consuming cached data, then checks cancellation and stale-data rejection
after seeking. Real MP4 decoding also runs through the read-ahead path. Storage
stalls longer than the buffered playback time still require a hardware check.

For interrupted video downloads, `run-download-stream-tests.sh` checks that a
missing prefix raises the buffering signal after 500 ms and clears it on data or
cancellation. `run-audio-playback-tests.sh` connects a real growing input to
the video audio ring and checks that sound pauses and resumes with the input.
The Performance log records `stream_read_us` for file reads and `stream_wait_us`
for missing-byte waits; these do not measure network throughput. The cache worker
writes its next report after active video playback releases its storage reservation.
`video_download_bytes_per_s` and `video_prefix_bytes_per_s` sample changes in
TDLib's downloaded count and contiguous readable prefix once per second; zero
can also mean delayed TDLib updates, not necessarily a network outage.
On Vita repeat 720p streaming, pause, seek, slow and interrupted downloads, and
compare the log after stopping playback; audible synchronization and download
throughput require device testing.

`bash tests/run-audio-playback-tests.sh [music.mp3 music.m4a]` uses the native
FFmpeg build (run `bash scripts/build-native-ffmpeg.sh` after updating the filter
configuration). Under ASan/UBSan it checks the audio bar's shared render/touch
geometry in Classic/Combined and above the bot toolbar, dead space and boundaries,
volume/seek clamps, all seven speeds, duration and pitch against a 440 Hz signal,
and filter cancellation. It also runs the real VoicePlayer with mocked Vita output:
pause/seek/replay, speed changes, media-time counters, volume clamping and application,
streaming seek limits, video-audio staying at 1×, output/volume failures and cleanup.
Optional MP3/M4A files exercise the real decoder at 1×/2× and a 30-second seek.
`bash tests/run-transfer-tests.sh` covers previous/next, both ends, all repeat modes,
shuffle without missing/duplicated tracks, preserving the current track, chat changes,
bounded prefetch/cancellation and the 64-track limit. `bash tests/run-ui-mask-tests.sh`
checks moved corner buttons with real light/dark/custom skin resources.
On a physical Vita still check sound quality and CPU load at every speed, live
progressive downloads, touch/controller transitions, loading/paused/finished states,
Classic/Combined, bot toolbar, modal tools, light/dark/custom palettes and all font
sizes. Host checks do not establish on-device rendering or audible performance.

`bash tests/run-chat-scroll-tests.sh` also checks pixel scrolling of variable-height
history rows, one-pixel movement, both limits, button reveal, tall messages,
empty/shrunk history and preserving the visible message ID and partial offset
when older/newer pages arrive. On Vita verify touch tracking in Classic/Combined,
crossing the pane divider, date chips, reply/voice/comment taps, page loading,
all font sizes and Light/Dark/custom palettes.

## Что хранится в tests

- `*_test.cpp`, `*_test.js`, `*_test.py` — код проверок; `run-*.sh` — запуск.
- `*-stubs/`, `stubs/`, `test_language.h` — необходимые заглушки API Vita для ПК.
- [`fixtures/`](fixtures/README.md) — текстовые входные данные, нужные тестам.
  Это не ресурсы приложения; они не включаются в VPK.
- `local/` — личные входные файлы и результаты диагностики, исключённые из Git:
  `media/` (фото, видео, снимки экрана), `logs/`, `dumps/`, `output/`.
  Старые файлы из корня перенесены сюда без изменения содержимого.

Большинство проверок создаёт временные данные в `/tmp` и удаляет их после запуска.
Локальное видео для `run-download-stream-tests.sh` теперь лежит в `local/media/`;
в чистом checkout передайте свой H.264/AAC MP4 аргументом:
`bash tests/run-download-stream-tests.sh /absolute/path/to/video.mp4`.
Аудио для `run-audio-startup-tests.sh` также передаётся аргументами.
Мультимедийные образцы в Git не включаются. Для воспроизводимых проверок
оставляйте необходимые текстовые образцы в `fixtures/`,
а не комментируйте тесты; новый образец нужно явно разрешить в `.gitignore`.


Audio startup diagnostics: enable **Start → Diagnostic logs → Performance** before
opening a music/voice message. Test both Play during downloading and Play on an
already downloaded file; let the sound start (or wait through the delay), then
close playback and allow at least five seconds for the background log writer.
Collect `ux0:data/vitagram/performance.log` and `performance.previous.log` if present.
The `audio_startup` lines correlate `player` and `generation`, with `at_us` on the
same clock as other performance events. UI download/button events use generation 0
and include Telegram's local `file_id`; filenames and media contents are omitted.
`total_us` measures from decode entry, not from Play; subtract `at_us` of `play`
from `first_output_begin` for Play-to-first-submission latency. Pauses/resumes and
ignored Play presses are logged separately. Paired `*_begin`/`*_end` timestamps
separate file attachment, container opening, stream probing, codec initialization,
seeking, audio-port opening, PCM conversion and the first blocking AudioOut call.
First-frame `result` is the sample rate, `decoded_channels`/`decoded_sample_format`
report the decoded layout/format, and `codec_open_begin` reports FFmpeg's codec ID;
other operation-end results retain the API return value. Output timestamps measure
API submission/return, not physical speaker latency. A missing end marker identifies
an unfinished stage; check `audio_startup_events dropped` before drawing conclusions.
`bash tests/run-performance-tests.sh` checks gating, chronology, progress snapshots,
bounded storage, nonblocking producers and background-log integration in ON/OFF builds.

`bash tests/run-audio-startup-tests.sh /path/to/music.mp3 /path/to/music.m4a`
uses native FFmpeg (same setup as the download-stream tests; override
`FFMPEG_NATIVE_BUILD` if needed). Supply audio fixtures larger than 256 KiB.
It checks full decoding/resampling without `avformat_find_stream_info`, matching
PCM against the analyzed baseline and for ordinary/completed-download reads, the first audio block from a
256 KiB prefix before download completion, and a fresh 30-second seek for tracks
longer than 31 seconds. Both reported MP3/M4A files in `release/` pass these checks.
Music now skips stream analysis, so new logs go from `open_input_end` to
`codec_open_begin` without `stream_info_begin`/`stream_info_end`.

`bash tests/run-gxm-guard-tests.sh` checks failed vertex/fragment uniform reservations,
stale output pointers, scene and uniform-write failures, skipped GPU draws and
recovery at the next successful scene. Uses installed VitaSDK headers (set `VITASDK`
if needed), with GXM calls mocked; actual GPU recovery requires a PS Vita check.
It also checks Draw/EndScene failures, suppression of failed buffer swaps, vblank
pacing while skipping frames, and startup-log gating/reset. To investigate a hang
at `1/5 Loading fonts and emoji`, enable **Start → Diagnostic logs → Performance**
on a successful launch, then reproduce and collect `ux0:data/vitagram/startup.log`
before launching again (the next launch replaces it). The log records font/theme
startup steps and up to 16 distinct consecutive GXM errors, without account data.
This does not establish the cause of an intermittent device hang: repeated cold
launches, the loading-screen transition and graphics recovery require a Vita check.
The same suite checks initialization of ordinary vita2d textures against poisoned
heap contents under ASan/UBSan, preserving image/palette ownership and allocation
failures. The installed SDK leaves `gxm_rtgt` and `depth_UID` uninitialized;
its texture destructor reads both, potentially destroying unrelated GPU resources.
The client and updater wrap both ordinary texture constructors; render-target
textures retain their original constructor. Check repeated startup, theme changes,
image/sticker/video playback and camera previews on physical hardware.

`python3 scripts/build-palette-editor.py && node tests/palette_editor_test.js`
checks the offline Windows/Linux editor bundle, all 19 locales, translation keys and
placeholders, browser language selection, JSON/name limits, duplicate keys,
all exported patterns, contrast and palette round trips. Optional Playwright UI
checks cover import/download, Unicode filenames, catalogue selection, all preview
screens and battery states, local catalogue import/replacement and invalid-file
atomicity, and zero external HTTP requests; see
[editor instructions](../tools/palette-editor/README.md).
Border checks cover the unified color list, preserving manual/imported
`border` and `focus` through auto shading, and JSON export/import. Native skin
checks verify that both frame colors change without altering panel centers,
alpha coverage, file reads or texture allocations.
Preview checks exercise eight native views and verify actual draw coordinates:
the five folder tabs, nine 43 px chat rows, status strip, settings viewport,
theme picker and both corner menus. The bundle must contain the unchanged Noto
font and constants from `src/ui_layout.h`; browser tests verify font sizing,
opacity calibration and controller sprites while retaining zero HTTP requests.
History checks include a reply to a visible message: the native date-chip quote,
sender, source text, accent bar, alpha, expanded card and shifted following rows
are checked at 120% and 130% font sizes.
Semantic color checks cover icon/text independence, recolored notification pixels,
active/inactive radio controls in the poll preview, delivery strokes, old-palette
fallbacks, and custom-role presence bits beyond bit 31.

The mask suite also exercises custom theme discovery from
`ux0:data/vitagram/palletes/`: strict bounded JSON, UTF-8 filenames, symlink and
directory rejection, ordering, the 64-theme cap, warm switches without reads or
texture allocation, and separation from the legacy single-palette override.
Vault sweep checks round-trip custom selections by filename, including pre-unlock
hints, reordered numeric IDs and missing-file fallback.

`bash tests/run-ui-mask-tests.sh` runs the real skin loader against PNG masks with
host Vita API stubs and libpng. It checks alpha composition, strict/atomic JSON
palette validation, recoloring without file reads or texture allocations, live
same-size file changes, preset switches, invalid-file fallback, shared battery
sprites and resource cleanup. Address/undefined sanitizers are enabled; system
libpng and Jansson development headers from build-modern are required.
The named presets are loaded from their packaged JSON files; the legacy
override file must not replace them. Vault sweep tests round-trip all sixteen theme
IDs through settings.json and the pre-unlock theme hint, checking light/dark mode.
Mask tests compare the optimized compositor byte-for-byte with the original
formula, report host timings, and verify picker wrapping, scrolling and touch boundaries.
Theme-hint tests cover legacy single digits, two-digit IDs, newlines, invalid
suffixes, overflow and out-of-range values.
Every warm preset switch and swatch lookup must perform zero file reads and zero
GPU texture allocations; VHBB transitions still exercise full loading and cleanup.
VHBB checks verify the two-pixel focus outline, transparent ring interior, at least
3:1 outline contrast against normal/opened/focused rows and 4.5:1 focused text
contrast. Actual nine-slice draw calls cover each pixel once at Classic/Combined
row sizes, folder rows and reduced corner sizes.
Skin checks also inspect rendered corner glyphs and all six battery states for
every masked preset. Battery tones must contrast at least 3:1 with both header
endpoints; live changes to white, black, gray and blue headers repeat the checks
without reads or allocations. Corner ink must contrast at least 4.5:1 with its surface.

`python tests/theme_composition_test.py` checks catalogue color codes, malformed
compositions, contrast rejection, portable recipes, and actual light/dark PNG
generation with the existing UI template. It also checks palette-only exports,
white/alpha mask layers and original icon coverage. It verifies dimensions, nine-slice
insets, atlas coordinates, icon alpha and refusal to overwrite an existing pack.
Use a Python environment with Pillow and CairoSVG; no Vita build or local paint
catalogue is needed. See `assets/VitaGram_UI_Assets_v1/README_RU.md` for usage.

`python tests/wave_backgrounds_test.py` checks that VHBB shares the dark theme's
wave geometry with its own palette, and that both packaged variants match the
renderer as opaque 960×544 RGBA images. Requires Pillow and CairoSVG.

`bash tests/run-scroll-tests.sh` checks held-arrow acceleration, direction and menu
changes, release and resume cancellation, frame stalls, and the limit of one page
per 0.5 seconds in Normal mode across different viewport sizes and frame rates.
All three speed presets are checked against their selected limits of 2, 4 and 6
pages per second. Vault checks verify saved scroll speeds and default fallback.
It also checks left-stick dead-zone hysteresis, D-pad priority, source changes,
and matching acceleration for stick and button holds.

Cache regressions for 0.33.7 block an actual cache read and apply live chat/user
updates and message deletion before it resumes. They also clear the account during
the read and require the late result to be discarded. Old startup indices and
indices without chat positions must trigger rebuilding from the main cache.

Startup checks for 0.33.6 verify the separate lightweight index, atomic interrupted
writes, corrupt-index fallback, incremental UI adoption and live updates between
slices. The transfer harness blocks the full cache read on the real Storage worker
and requires the initial list to appear before releasing it. Offline demand checks
cover history, audio paths, visible avatars, stickers and stale session IDs.

`bash tests/run-media-cache-tests.sh` checks the bounded asynchronous file-validation
queue, replacement and invalidation during stalled stat, duplicate work and overflow.
Transfer tests run the real parser and Storage worker with blocked file I/O: 1002
events, a chat and a new message must reach the UI before storage resumes; a late
file-only result must restore the avatar. These are host regressions, not Vita timings.

`bash tests/run-media-trash-tests.sh` checks the one-time swap of bloated media
directories before TDLib starts, the batched background removal of the old copies,
and flagging a directory above its entry limit for the next start.

`bash tests/run-cache-tests.sh` verifies immutable cache snapshots across chat, poll,
message, folder and sticker mutations; UI updates during a deliberately stalled writer;
interrupted saves preserving the previous file; size limits; and the deliberate exit gesture
that excludes Select-first screenshot input. Transfer checks also exercise continuous-input fairness.

`bash tests/run-performance-tests.sh` checks histogram percentile bounds, interval resets,
queue high-water marks, concurrent accounting including dropped observations, file-event
coalescing byte accounting, log rotation/write failure and the disabled collector build.
Transfer tests additionally verify byte counters against the real worker/parser queues.
These tests do not measure device performance or validate the Vita system counters.

`bash tests/run-history-tests.sh` checks stable selection in mixed photo/video albums,
navigation in both directions, prepended history pages, photo-album merges and deletions.
It uses the application message type and selection code; a configured build-modern
Jansson include directory is required.

`bash tests/run-i18n-tests.sh` checks that every language catalog has the same keys, placeholders and icon tokens, language selection and
English fallback, the `.lang` file format, and that all `tr("key")` keys used in `src/` exist. Tests that compare Russian texts
select it explicitly through `tests/test_language.h`.

`bash tests/run-proxy-tests.sh` checks proxy URLs, real QR decoding and mocked Vita camera lifecycle.
It includes Telegram-style rounded finder rings and data modules with a central
logo, mirrored frames, rotation, perspective, reduced contrast and a strong
brightness gradient. It also checks capture between decode attempts and clearing
stale non-proxy errors after the code leaves the frame. The styled
renderer follows the radii in Telegram Android's `TelegramQRCodeWriter`;
it is a synthetic regression, not a physical-camera test.

`bash tests/run-video-tests.sh` checks circular YUV cropping, the current TDLib
`inputMessageVideoNote` payload, camera switching, microphone failure, cancellation,
the 60-second recording limit and transfer of file ownership to the upload caller.
It checks bounded video/audio queues, slot ownership during a stalled write, overflow,
draining and abort. A paced 30 FPS camera and 32 ms microphone continue capture during
a two-second disk stall; encoding must wait for the final flush. Injected fwrite,
fclose and setvbuf errors must fail the recording. Idle preview must avoid GPU waits.
Rear-camera tests wait for actual RGB preview pixels and check preserved Y/U/V planes
in recorded frames, with the same resolution and 30 FPS settings on both cameras.
It requires C/C++ compilers, pthreads, a host libjansson.so.4, and the Jansson headers
from a configured build-modern tree. Vita camera/GPU/audio calls are replaced with
stubs. The recorder's encoder is stubbed only in its lifecycle test.

To also run the **real** minih264 + AAC + MP4 encoder/decoder test (and the WebM sticker
test), build the app's FFmpeg release for the host (the Vita libraries are not touched):

```bash
bash scripts/build-native-ffmpeg.sh
export FFMPEG_NATIVE_BUILD="$PWD/external/ffmpeg-native/build"
bash tests/run-video-tests.sh
bash tests/run-webm-tests.sh /absolute/path/to/sticker.webm /absolute/path/to/alpha.webm
```

WebM checks cover user-supplied opaque and alpha samples (local copy: `tests/local/media/alpha.webm`)
(the reported white-background sticker): transparent, opaque and partial-alpha
pixels after RGBA conversion, every frame after looping, and malformed alpha
payload rejection. These use the production alpha decoder; Vita GPU blending
and playback performance still need a physical-device check.

The test decodes every video and audio packet and checks square dimensions,
circle masking, non-silent audio, duration, exact timestamps for 90 frames (including
a one-sample interval and a camera-switch gap), keyframes every 60 frames,
cancellation and truncated input cleanup.

These tests do not verify the physical Vita camera, microphone, GPU, encoder
performance or delivery to Telegram. Test on a device: open a chat → L → X;
switch with Triangle; X to stop; Square to play; X to send. Verify front/rear video,
audio synchronization, cancel/reopen, the 60-second limit, and the destination topic.


`bash tests/run-transfer-tests.sh` runs the production Telegram worker and parser
against a fake TDLib endpoint: 6,000 events while UI processing is stopped,
file-progress coalescing without lost control events, continued receive calls,
request recovery after backpressure, staged startup with a deferred cache writer,
parallel voice/video upload tracking, speed/stall/error states and original chat/topic
payloads. No network requests are made. The startup test requires configured API
settings (only local validation is exercised).

Cache cleanup checks cover all 30 retention/size combinations, unlimited limits,
manual cleanup with an open chat, automatic deferral, duplicate suppression,
offline rejection, completion, failure and retry. TDLib replies are simulated;
actual file removal and cache-menu appearance need a physical Vita check.

Status checks cover expiry and dismissal of completed events, chat-scoped upload
results, preservation of active uploads and diagnostic history, TDLib open/close
subscriptions, incoming messages after failed uploads, and explicit list refresh.
Additional cases cover list request timeouts, reconnect retries, ignored late replies,
draft position updates, and draft MTProxy checks with latency and a 15-second timeout.

Upload statistics tests cover sent/failed initial responses, confirmation arriving
before its request reply, correlation through sending_id, sticker sends without
invented byte progress, speed from receipt timestamps, stale file snapshots,
recent-operation selection, and separate UI/diagnostic retention. A queue of 1,000
file events must not delay a send confirmation; background events retain service.
Production worker tests also verify that navigation requests bypass congestion,
background requests wait during interaction, and receive() keeps running.

To include the production AudioRecorder with a real Ogg/Opus encoder, set
`FFMPEG_AUDIO_BUILD` to a native FFmpeg build with avformat, avcodec, avutil,
swresample, the opus encoder, ogg muxer and file protocol enabled. The test covers
microphone errors/restart, PCM conversion, Ogg headers/final granule, cancellation
and ownership of files passed to asynchronous uploads. Hardware audio calls are
mocked; Telegram delivery and throughput still require device testing.

Poll checks in `run-transfer-tests.sh` exercise the production message parser and
vote request builder: regular/quiz polls, single/multiple answers, display order vs
API positions, retraction restrictions, duplicate requests, errors, result refresh,
closed-state updates and persistence of updated polls in the UI cache.

Video quality checks in `run-transfer-tests.sh` verify minimum compatible resolution,
size tie-breaking, unsupported codecs, oversized original fallback, cached variants,
unknown sizes, unavailable variants, the actual download file ID and unchanged video notes.

`run-video-tests.sh` also checks converted-video cache keys, short reads and read
failures. `run-vault-tests.sh` verifies that the key stays the same after moving,
encrypting, decrypting and re-encrypting an actual source file, and rejects
unauthenticated samples from a damaged encrypted source.

`bash tests/run-memory-protection-tests.sh` checks the heap-wiping size policy, recognition
of VitaPlane's gzip crash dumps by the NUL-terminated title ID (other titles and partial
`.psp2dmp.tmp` files included), overwrite-and-delete of only those dumps, classification of
system, own and injected plugin module paths, and that the masked session keys yield the same
database/file keys and a verifiable state signature after a lock and unlock.
With `FFMPEG_NATIVE_BUILD` set, `run-video-tests.sh` also creates and converts
an eight-second H.264/AAC video, reopens it in cache-only mode, and checks that
no conversion runs again, including after moving and encrypting its files.


`bash tests/run-font-tests.sh` loads the packaged fonts with host FreeType and
rasterizes every character in `ⲕⲗυⲙⲁⲧ` and `の一部 宇宙`, plus Cyrillic/Latin controls.
It exercises the production fallback selector and checks the Japanese face is static
Regular. Requires a host libfreetype.so.6 and FreeType headers (system or VitaSDK).
This verifies glyph coverage/rasterization, not the physical Vita GPU output.

The transfer suite also checks visible sender metadata lookup, small-avatar file IDs,
request deduplication/backoff, download completion/invalidation, missing photos,
refresh of cached metadata and avatars for messages sent on behalf of a chat.


`FFMPEG_NATIVE_BUILD=/path/to/native-ffmpeg bash tests/run-webm-tests.sh /path/to/sticker.webm`
reproduces the old `Open: -22` whitelist rejection on a VP9 WebM fixture, then uses
the production container/codec policy to decode every frame, convert to RGBA,
check timestamps, rewind and decode again. By default the fixture is the user-supplied
`assets/sticker.webm`; it is not included in the VPK. Enable matroska demuxer, VP9
decoder, file protocol, swscale and swresample in the native FFmpeg build.
Physical Vita playback/timing and audio output remain device checks.

Transfer tests also check logout cleanup on disk, rejection of late updates, preserved settings, symlink safety and recovery after interrupted cleanup. Font tests include the exact nickname 𝕭𝖆𝖉𝕯𝖗𝖊𝖆𝖒.

Startup-list tests cover empty initial positions, short loadChats pages, atomic replacement, offline errors, stale replies, separate folders, confirmed removals and end-of-list. Font tests also rasterize the exact name ✧Hehestl | *interrogative hehe*.

Sticker-pack icon tests exercise production selection of PNG/JPEG/WebP thumbnails, rejection of animated covers, fallback to sticker previews, lazy metadata requests with 64-bit set IDs, deduplication, stale-response rejection and download completion/invalidation without changing the open sticker grid.

Планировщик TDLib (0.33.2): `bash tests/run-td-scheduler-tests.sh`. Требуются host CMake, C++17, OpenSSL/zlib development packages и зависимости конфигурации TDLib. Сборка помещается в `/tmp/vitagram-td-native` (переопределяется через `TD_NATIVE_TEST_BUILD`). Тест использует настоящий планировщик с Vita-веткой бюджета, управляемые часы, две очереди и таймер; проверяет порядок, отсутствие потерь и завершение актора с остатком событий. Реальные сокеты Vita этим тестом не проверяются. `run-performance-tests.sh` дополнительно проверяет вложенные TD-этапы и режим OFF.

Логирование TDLib (0.33.3): `bash tests/run-td-logging-tests.sh`, те же host-зависимости и `TD_NATIVE_TEST_BUILD`, что у проверки планировщика. Тест компилирует Vita-ветку настоящего `TsCerr.cpp` и подменяет только OS write: ошибка, нулевой результат, частичная запись, освобождение блокировки после ошибки. Проверяется также отсутствие форматирования DEBUG/INFO при уровне 1 и сохранение ERROR. Тест отправок проверяет настройку уровня до создания клиента на том же потоке; тест статусов — 100% без ложного завершения.

Медиакеш (0.33.4): `bash tests/run-media-cache-tests.sh`. Проверяются 500 повторяющихся ссылок с одним stat, отсутствие I/O в режиме UI, доступность RAM-кеша при заблокированном stat, сброс во время проверки, исчезнувшие файлы и сохранение исходного протокольного ответа для отправок. `run-cache-tests.sh` проверяет приоритет ранних живых данных и удаления перед загрузкой кеша, `run-transfer-tests.sh` — фоновое восстановление без сети, однократный запуск, применение на UI и отказ от позднего снимка после очистки аккаунта.

Изображения (0.33.9): `bash tests/run-image-tests.sh`. Нужны host C++17, pthread и development-пакеты libpng/libjpeg/libwebp с pkg-config. Проверяются реальные RGBA-пиксели, встроенное уменьшение большого JPEG, уменьшение PNG, повреждённые и слишком большие файлы, смена видимых запросов, очистка с повторным путём, пауза и остановка фонового декодера. GPU-загрузку и время кадра проверять на Vita.

Текст (0.33.12): `bash tests/run-text-layout-tests.sh`. Проверяются ключи кеша ширины, сброс, вытеснение LRU, пределы числа записей/байтов, обрезка по границам UTF-8 и логарифмическое число измерений длинной строки. `run-font-tests.sh` проверяет реальное покрытие и растеризацию fallback-шрифтов; холодные задержки и GPU-рисование проверять на Vita.

`bash tests/run-chat-scroll-tests.sh` checks pixel dragging in both chat-list layouts,
partial-row hit testing, viewport edges, button selection visibility, page append,
list shrink and folder reset, plus row motion when the list is replaced (a moved chat
slides from its previous row, mid-slide changes, one-screen limit, no motion after a
folder change). Touch sampling and rendering require a PS Vita check.

`bash tests/run-font-tests.sh` also exercises the production text raster cache
with real FreeType and an in-memory Vita texture stub: exact glyph pixels at
multiple sizes, first-use order independence, padding, stride, native pixel
positions, kerning, measurement without GPU allocations and bounded cache reset.
It does not validate Vita GPU output.

`bash tests/run-photo-tests.sh` checks fullscreen photo rotation: gyro restricted to
0° and ±90°, debounce, flat/touch behavior, manual L/R rotation including 180°,
disabled sampling, sampling failure, and reopening the viewer. It also checks stick
zoom from 100% to 500%, pan direction and bounds, dead zones, frame-independent speed,
stall protection, touch priority and pinch limits. Vita motion and GPU
calls are stubbed; physical orientation and themed button rendering require a device.

`bash tests/run-audio-recording-dialog-tests.sh` checks preview/record/encode/review
states, explicit send, cancellation, microphone-level bounds, repeat playback and
ownership of the prepared voice-message file. Shortcut settings save/load are
covered by the vault-sweep tests.

`bash tests/run-spoiler-tests.sh` checks reveal reset on chat/topic/discussion exit,
app resume, selected reset events and cache invalidation signals. The vault-sweep
tests also check spoiler preference save/load, defaults and invalid value types.

Transfer tests also check video spoiler metadata, separate thumbnail download before consent, full alternative-video download after consent and cancellation when consent resets. Animated spoiler dust requires visual verification on Vita.


`bash tests/run-font-tests.sh` also runs the actual language-font loader with
real Korean/Chinese FreeType faces and host texture/scheduler stubs. A blocked
worker exercises stale completion, latest-selection handling, cancellation to
the default font, cached face/atlas reuse, missing/corrupt assets and shutdown
with an outstanding read. The previous active face and revision must remain
unchanged until commit. Host libwebp.so.7 and WebP headers are required as well.
Check rapid language switching and immediate All chats/Archive translation on Vita.

The same command checks all nine controller hint icons with theme-atlas and
fallback PNG sources: alpha 0/128/255, repeated fade resets, original icon colors
and stable measured widths at all seven font sizes. Rapid menu tapping and actual compositing with
Light/Dark/custom palettes still require a physical Vita check.


Language regressions: `run-i18n-tests.sh` checks retained nested/combined text,
raw-string preservation, recipe reset, a worker rendering during a catalog
commit, and every network state in every shipped language without another
hardware poll. `run-transfer-tests.sh` applies all locales to an existing
TelegramClient, checking authorization/status, folders, sticker sections,
custom reaction IDs, polls, documents, media captions, service messages and
reply previews without another TDLib event or outgoing request. Proxy/QR and
video tests also check retained scanner failures and recorder status across
language changes. Rapid switching and glyph appearance still need Vita testing.

Transfer tests cover cached deletion permission reads beyond the refresh interval, preserving rights during refresh, deletion without a second properties request, permission denial and waiting for all loaded album members.

`bash tests/run-upload-picker-tests.sh` checks attachment marking/unmarking for
photos, videos and files, the 10-item limit, path identity across folders and the
latest view, retained selections during refresh/navigation, and explicit review
before sending. Review checks cover Send as the first row, sending from any row,
empty/duplicate-send rejection, file preview/removal offsets and the two Add rows (Multimedia and File). Mixed multimedia browsing checks
photo/video filtering and automatic outgoing types, plus return-menu focus.
It also covers all six sort modes, cycling both ways, preserving
the focused file and marks, equal filenames, and returning to the right folder
after changing sort order in a child directory. The multimedia root shortcut for
latest screenshots must remain first through every sort, refresh and return. Requires VitaSDK headers; filesystem and clock calls are stubbed.
Visual highlighting and physical/touch controls require a PS Vita check.

`bash tests/run-qr-encode-tests.sh` checks QR byte encoding at levels M and H
through version 10, and decodes the actual antialiased login raster with its
rounded patterns and paper-plane badge across different tokens and lengths.
The four-module quiet zone and reduced-size rendering are covered. These are
host checks; phone scanning of the Vita display requires a device check.

`bash tests/run-media-seek-tests.sh` checks fullscreen 10-second seek steps,
hold acceleration, one commit on release, bounds, direction conflicts, stale
message/menu/sleep cancellation and touch-track mapping. The FFmpeg download-stream
suite also checks actual H.264 keyframe pre-roll. These host checks do not validate
controller/touch behavior or decoder startup time on Vita hardware.

`bash tests/run-keyboard-tests.sh` checks the shared vault/text keyboard with
controller and touch events: localized layouts and shift, short/long presses, UTF-8 deletion
and limits, multiline whitespace, submit/cancel and masked secret rendering.
System message input keeps Enter instead of multiline, including direct and
switched Japanese/Korean/Chinese IME routing; custom input retains line breaks.
It also verifies letter/digit previews, their suppression for secrets, the wider
text spacebar and the absence of clear-all through touch or R in ordinary fields.
RU checks cover the 11/11/9 letter rows, е→ё and ь→ъ with touch/controller holds,
uppercase alternatives, delayed release, cancellation, byte limits and previews.
Popup tests exercise the exact 299.999/300 ms boundary, deferred insertion until
release, е/Е→ё/Ё or 4, complete RU number alignment, touch sliding through the
key-to-panel gap, controller Left/Right selection, Circle/Start cancellation,
drag-out/re-entry, repeated release, punctuation choices and password byte limits.
Comma/period remain directly accessible and all digits remain on the symbols layer.
Rendering calls are stubbed; layout and physical input need a PS Vita check.
The vault suite checks persistence and malformed values of the keyboard preference.

Transfer tests also cover delayed Telegram links after chat/topic changes and reopening the same chat, stale topic history, document size updates, download progress after navigation and retries with late errors from an earlier attempt.

Document export tests cover background saving and duplicate comparison with blocked storage while the UI pump continues, repeated completion events, copy progress, cancellation cleanup and account reset.

`bash tests/run-vpk-tests.sh` checks package metadata bounds, rejected title IDs and patch packages, content-ID matching, and homebrew header checksums under ASan/UBSan. Transfer tests check completed export notifications, cached exports and clearing pending offers on logout; vault tests check the default-off installation preference. Native message dialogs and actual installation still need a PS Vita.

Keyboard localization tests cover all 19 shipped catalogs: 15 in-app profiles and
4 native IME routes, carousel selection/cancellation, preserved drafts, remembered
profile choices, console-language defaults, Turkish dotted/dotless case and locale
alternatives. Native IME calls are stubbed: forced languages, UTF-16 drafts,
password options and multiline mode are checked without opening a Vita dialog.

Keyboard case checks cover one-shot Shift, the 350 ms double-tap boundary, Caps
Lock/unlock through touch and controller, cancelled and rejected input, numeric
alternates, deletion, language changes and Turkish case. Context checks include
message starts, sentence punctuation, quotes, Unicode whitespace, newlines, common
abbreviations, decimals, filenames and URL/email interiors; password/exact-input
fields retain manual case. The visual Caps Lock marker still needs device review.

Keyboard cursor checks cover Square backspace, Triangle space, L/R boundaries,
UTF-8 middle edits, newline/alternate insertion, full-draft byte limits and
automatic case at the cursor. Vita rendering and physical feel need device checks.

Keyboard language checks cover EN + console defaults, British English mapping,
unsupported-system fallback, manual lists, the last-language guard, filtered
carousels, direct cycling, native language masks and CJK-only vault fallback.
Settings persistence checks cover migration, malformed arrays and default reset.

Language-key checks cover short-press cycling, the 300 ms threshold, controller
focus retention, touch/controller carousel selection, cancellation, single-language
input, all 19 profiles, seven visible choices, edge scrolling and IME handoff
on release. Switching preserves the draft, cursor and Caps Lock.

Deletion-repeat checks cover immediate input, the 350 ms delay and 60 ms interval,
release at a deadline, touch exit, controller focus changes, masked fields, UTF-8
middle edits, empty input, delayed-frame burst prevention, cancellation, reopening
and simultaneous deletion sources.

Passphrase rendering checks verify on-key controller badges, cursor hints inside
the spacebar, no separate shortcut-help strip, and no exposed passphrase in
either the full keyboard or the vault keyboard-only drawing mode.

`bash tests/run-updater-lifecycle-tests.sh` runs the production updater and separate updater app with mocked HTTPS, graphics and Vita system calls. It checks signed download/staging, signature revalidation at startup, helper installation, application installation/relaunch, cleanup, fallback storage and failures. Telegram VPK installation must remain blocked while a signed update is busy or staged, including pending markers left in either storage folder; explicit local handoff remains available after completion. Test packages use a temporary signing key; the production release key and verifier are unchanged. Actual device promotion still needs a PS Vita.

Transfer tests cover overlapping text styles, UTF-16 offsets after emoji and invalid entity ranges. Text formatting and spoiler controls require visual verification on Vita.

`bash tests/run-transfer-tests.sh` also checks Telegram user colors: built-in and server palettes, light/dark variants, gift accents, live channel changes, reply authors from history/cache/external origins, offline startup and live-over-cache precedence. `bash tests/run-vault-tests.sh` checks that the user-color switch persists and defaults to enabled for missing or invalid values.

`bash tests/run-reaction-tests.sh` checks short Cross on release, the 550 ms
hold threshold, single activation, selection/chat/topic and modal cancellation,
resume gaps, and controller/touch grid mapping with a separate preferred row.
Grid checks include cell edges, non-clickable gaps, empty preferred-row slots,
page offsets and unused cells on the last page.
It also checks short sticker/GIF selection, held Cross/touch previews, release without
sending, and cancellation on changed item/set/search generation/chat/topic, navigation or overlays.
Transfer checks cover full sticker/GIF downloads, completion gating, thumbnail separation
and static/animated sticker format selection. GIF catalog tests check completed full-file
paths for saved and searched GIFs and reject partially downloaded files for playback.
Transfer checks cover preferred/recent ordering, duplicate and Premium filtering,
and the eight-item first-row limit. Physical controls still need a Vita check.

`bash tests/run-radial-menu-tests.sh` checks the 250 ms Square threshold, five
fixed stick sectors, neutral/dead-zone and angular hysteresis, direct L/R modes,
pinned reply IDs, Circle/scope/suspend cancellation, rapid represses and exact-once
confirmation after closing. It checks early-release animation continuity, cached
geometry and actual themed drawing with a bounded frame-pool stub and zero menu
heap allocations over warm frames. Transfer checks cover replies for text,
stickers, inline GIFs, voice, mixed albums/documents, failures and cancellation.
Vita GPU appearance and 60 FPS timing require physical-device verification.

Radial checks also cover repeated L and R toggles in both directions, simultaneous
shoulder presses, rounded contour bounds and curved corners. The real renderer
keeps the 496 px glass wheel and glow inside the viewport, including the badges,
icons and captions, within a bounded GPU frame pool and without warm-frame heap
allocations. `RADIAL_PREVIEW=/tmp/radial.jsonl bash tests/run-radial-menu-tests.sh`
exports native drawing commands for visual review; fonts and compositing on the
physical Vita still require a device check.

Radial smoothing checks bound circle/arc/corner chord errors below 0.05 px,
verify cached unit contour normals, and require one-pixel alpha-coverage strips
on the real rendered shapes while retaining the fixed GPU-pool and zero-heap
checks. Petals, central rings and rounded icon/text boxes use local edge smoothing.

Design checks also cover dark radial glass, adaptive glyph/label contrast,
Light/Dark secondary text and BBR/BRR2 palettes. Palette Studio browser checks
include the ninth radial view, selection, L/R toggles and live theme changes.

Keyboard design regressions verify theme wallpaper/header use, key masks,
independent focus color, readable normal/pressed/disabled labels, accent
glyph contrast and the vault keyboard-only mode. Theme tests keep slots
6/7 assigned to black-blue/black-red without shifting the remaining IDs.

Split-keyboard regressions send real two-contact packets through CustomKeyboard
and Keyboard, including reordered IDs, single-finger tails, jitter, one-sided/
vertical/parallel drags, stale chords, third fingers, all custom layouts,
release typing/submission, repeat deletion and masked vault entry.

Keyboard drawing checks also verify 2 px key borders and one-pixel transparent
coverage fringes on dense rounded contours in both full and split layouts.

`bash tests/run-reaction-tests.sh` also checks quote frame widths and boundaries
for multiline, adjacent and single-line blocks. Transfer checks cover current
reaction counts/selection, sticker/custom emoji set lookup, installation retries,
request deduplication and stale responses after leaving the set.

Chat opening checks in `run-transfer-tests.sh` cover latest/server/device modes, exact read anchors, encrypted-cache position persistence and early-restore precedence, memory-cap acknowledgement, bounded history in both directions, deleted targets, retry and stale responses. `run-vault-tests.sh` covers preference persistence. Loading-screen appearance, physical input and RAM headroom require a Vita check.

Chat opening priority checks cover newer device/server positions, equal or missing markers, and preservation of latest/device-only modes for chats, forum topics and comments.

`bash tests/run-bot-buttons-tests.sh` checks bot markup parsing, complete emoji labels,
shared rendering/touch geometry, non-clickable gaps and unused cells, page bounds,
and 4.5:1 text / 3:1 focus contrast on light, dark and arbitrary custom surfaces.
Classic/Combined panel checks cover the shared toolbar/sheet geometry, animated
opening/closing bounds, empty and last pages, exact cell edges and sidebar misses.
Invoice tests verify cancellation JSON, identifier/type/length validation and
preservation of the incoming request; the official SDK check exercises the
callback, duplicate responses and reopening the same invoice without a payment.
`bash tests/run-transfer-tests.sh` also checks bot-menu availability, keyboard
removal and one-time replies, callback bytes, links, callback alerts, edits and
rejection of stale sources, chats and topics. Check actual font sizes, controller
and touch navigation, every theme, long labels, the last page and callback alerts
on a physical Vita. The Mini App protocol, HTTPS origin checks, palette conversion and bounded JavaScript bridge also have native/Node checks in these scripts. This does not launch the engine or authorize real Mini Apps. See [WebKit build and device scenarios](../docs/webkit-vita.md); the base VPK excludes the engine and offers its separately signed add-on on demand.

`run-bot-buttons-tests.sh` also checks that WebApp stage diagnostics write nothing
by default, reject invalid stage identifiers, append only fixed labels and numeric
codes when Performance logging is enabled, and stop writing when it is disabled.
Real touch delivery, confirmation visibility and engine startup still need a Vita.

Proxy checks cover SOCKS5h and HTTP CONNECT URLs, escaping credentials, invalid
hosts/ports, disabled proxy settings and direct connections with MTProxy. The engine fixture
extracts the real DNS endpoint selection and verifies that custom proxy settings
resolve the proxy rather than the website. Proxy authentication, remote DNS and
page loading through a real proxy still need a physical Vita check.

The same script checks smooth Mini App pointer movement, frame-time limits and
page bounds. Its trace test verifies the fixed-label allowlist, concurrent
deduplication, reset on a new open, diagnostic gating and rejection of arbitrary
text/payload prefixes; unsupported bridge actions use allowlisted names only.
Left-stick calibration, cursor clicks, keyboard isolation and page scrolling
still need a physical Vita check.

Bot menu regression: `run-bot-buttons-tests.sh` checks the animated bottom sheet, touch gaps, last pages and all seven font sizes. `run-transfer-tests.sh` checks that command and text reply activation enqueue real `sendMessage` requests and that Mini App menu entries preserve the bot commands.

WebKit's Vita thread broker can be checked independently with
`bash tests/run-webkit-threading-tests.sh`. It compiles the production broker
against mocked Vita exports and checks complete CPU/VFP register snapshots,
nested suspension, rollback and resume failures, registration lifetime, and
VM thread flags without changing ordinary thread creation. It does not verify
the native ForVM ABI or JavaScript garbage collection on a physical Vita.

`bash tests/run-webkit-memory-tests.sh` requires the prepared WebKit sources and
compiles production `SharedMemoryVita.cpp` and `UnixFileDescriptor.h` with small
WebCore/WTF type shims. It checks RAM budget recovery, full-buffer zeroing,
shared-buffer lifetime and bounds, large messages, invalid/duplicate tickets,
pending attachment cleanup on disconnect, and ordinary descriptor ownership.
It does not run the actual IPC socket loop or the engine on a Vita.

`bash tests/run-webkit-process-tests.sh` checks the real ProcessIdentifier
implementation with isolated UI/Web/Network run-loop ownership, worker access
and concurrent ID generation. It compiles the actual shutdown branches for Vita,
GTK, WPE and other platforms, and the complete Vita process provider with fake
kernel APIs: retries after launch failure, singleton restart refusal, bounded
waits and no deletion of a running thread. Prepared/staged WebKit is required.
These checks do not execute WebKit or the kernel on a physical Vita.

The same script compiles the production Mini App input/rotation methods and the
native Vita mouse/wheel conversion. It checks press/release state, timestamps,
pixel scroll deltas with no horizontal drift, inactive pages, every pixel's
rotation/inverse mapping, portrait viewport dimensions, unchanged orientation
and allocation failure without losing the old surface. Existing photo tests
cover the shared gyro sampling and 350 ms stabilization. Actual page clicks,
scrolling, gyro calibration and resizing still need a Vita check.

Modal touch checks extract the production ownership and frame-end blocking
expressions: repeated press/release sequences remain usable, earlier blocking
is preserved, closing the modal blocks the underlying release, and a Web window
owns touches even with the bot panel closed.

Gesture tests cover tap jitter, swipe deltas, inertial decay, full-height touch
targets, starts outside the page, multiple fingers, changed identifiers and
keyboard/controller cancellation. Input tests repeat every pixel's inverse
mapping in normal and fullscreen modes at 0/±90°, verify horizontal wheel
deltas, viewport sizes, unchanged layouts and surface cleanup when returning
from fullscreen. Physical touch feel, nested scrolling, palette visibility and
fullscreen/keyboard/gyro transitions still require a Vita.

The shutdown fixture also compiles the real Vita view-resize method: the drawing
area receives 960×392 and later sizes, including when it becomes available after
an earlier resize. Passive bridge-event tests accept only the native-free
iframe-ready/swipe-configuration events and leave sensitive actions unsupported.
It also compiles the actual transfer-completion prefix: a cancelled transfer
does not emit an error, and a genuine failure reports its numeric CURLcode.
Trace tests reject out-of-range CURL/HTTP/freeze codes and still exclude raw
payload prefixes, free-form messages and arbitrary trace labels.

After building the experimental WebKit package, run
`python3 tests/check_webkit_pic.py build-modern/src/webview/module/vitagram-webkit build-modern/src/webview/module/vitagram-webkit.velf`.
It compares ARM BASE_PREL/GOT_BREL relocations with the converted Sony ELF,
checks exact target segments and offsets, and rejects missing or duplicate GOT
relocations. This checks the local converter's output without executing the Vita loader.

`bash tests/run-chat-scroll-tests.sh` also checks Classic/Combined chat geometry,
message widths, viewport boundaries, row gaps and animated/dragged row offsets.
`bash tests/run-vault-tests.sh` checks display-mode persistence and fallback to
Classic for missing or malformed preferences. Physical Vita checks still need
both modes, list/history focus, touch scrolling, folder changes, empty/loading
chats, the last row, all font sizes, Light/Dark/custom palettes and full-screen
photo/video viewers and message-pane posts.

The chat-view geometry check also covers search below folders in the combined
sidebar, history beside the search field, pane routing on both sides of the
divider, inactive search/folder gaps and bottom boundaries with/without bot controls.
On Vita, drag each pane while the other has controller focus, then cross the
divider without lifting; verify that only the pane of the initial touch scrolls.

Combined subviews: `run-chat-scroll-tests.sh` checks post/media bounds, voice
waveform endpoints, menu height, topic pages and non-clickable topic/poll gaps
in both layouts, with/without folders. On Vita verify chat loading, retry and far
read-position warnings; empty/loading/last-page forum topics; text, reply, link,
photo-album, voice, video-note and poll posts; their menus and touch controls.
Check Classic/Combined, Light/Dark/custom palettes and every font size, including
returning from fullscreen photos/video and switching chats while a post is open.

Folder drawer host coverage (`bash tests/run-chat-scroll-tests.sh`): easing and reversal, default collapsed state, folder row boundaries/gaps/last page, vertical scrolling, gesture ownership and return to normal scrolling. Physical Vita checks still required: GPU viewport/stencil alignment during the slide, native controls, empty/loading folders, light/dark/custom palettes and font sizes.

`bash tests/run-chat-scroll-tests.sh` also checks consecutive-message grouping and
bubble meshes: incoming/outgoing, first/middle/last, classic/combined widths and
short/tall cards, including consecutive messages more than five minutes apart.
Sender-avatar checks cover one icon per visible group, bottom alignment, viewport
pinning, partially visible groups, empty history and date/service boundaries.
Sampled triangle coverage verifies that translucent fills and
focus outlines never overlap themselves or each other. Physical Vita checks are
still needed for light/dark/custom palettes, font sizes, focus, media/replies,
date/service boundaries and scrolling across loaded history pages.

Mini App URL regression: `run-transfer-tests.sh` exercises an ordinary URL button
through TDLib link classification, bot validation and explicit consent to
`getMainWebApp` / `getWebAppLinkUrl`, preserving launch modes and start parameters.
It checks signed responses, denied write access, ordinary chat links, invalid
bots/modes, errors, cancellation and replies after leaving the source chat.
Real bot launches still require a physical Vita.

### Performance diagnostics (0.38.46)

`bash tests/run-performance-tests.sh` checks counter resets and OFF gating,
thread-local frame attribution, slow-cause retention ahead of enclosing frame/draw
totals, context serialization and constant-time parsed-queue age after priority
selection and coalescing. `bash tests/run-bot-buttons-tests.sh` validates Web App
trace timestamps, session/frame correlation and rejection of private trace strings.

Each `perf` window now includes `session_us` (the startup origin on the monotonic
clock), diagnostic-loss labeling, throughput and frame-threshold counters. Counter
values cover the snapshot interval; divide by its duration for events/second.
They are best-effort under contention, like the existing histograms.
`frames_over_33ms` uses 33334 microseconds; 50/100 ms counters are nested subsets.
`slow_event` includes the UI frame, screen, combined/keyboard/scrolling/fullscreen
flags, rotation (quarter turns), and Web surface dimensions. Non-UI threads use
frame 0 and unknown screen. Context is sampled at frame entry and before drawing;
scrolling denotes controller/chat drag or Mini App drag/inertia. It does not
identify DOM scrolling initiated by page scripts. No chat IDs or user text are
recorded. Detailed slow causes take priority over enclosing `frame_ui`/`draw`
records when the bounded 32-record buffer fills.

`web_runloop_us`, `web_paint_us` and `web_bridge_submit_us` isolate native work.
`web_bridge_reply_us` measures drain-result handling; `web_bridge_latency_us`
measures the full asynchronous drain round trip, including scheduling and IPC.
The number of `web_paint_us` samples is the number of paint calls, not a page FPS
measurement. `frame_tail_us` covers image loading, text polling and emoji work
after the UI/GPU submission; these metrics overlap their nested measurements.
`ui_active` also identifies WebKit and frame-tail operations during a stall.
Web App trace entries use the same `at_us`/`session_us` clock as performance logs;
frame 0 means a non-UI thread or a pre-frame startup event.

`raw_wait_us` remains receive-to-parser-start; `event_wait_us` remains
receive-to-UI-selection (it includes parsing). `parsed_wait_us` starts when the
parser is ready to enqueue the completed event, after its cache work.
`raw_oldest_age_us`/`parsed_oldest_age_us` sample the oldest pending item per pump
without traversing the backlog. `received_events` counts accepted network
updates, `parsed_events` counts enqueues, `processed_events` counts UI selections;
coalescing and shedding can make these counts differ. `pump_batch_events` counts
selections per pump; `pump_budget_us` is the assigned event-processing budget.
`pump_overrun_us` covers that event loop only, excluding subsequent cache and
cleanup work. Stop counters distinguish empty queue, foreground filtering,
exhausted time budget and event limit. They describe why selection stopped,
not a guarantee that work remains queued.

Device verification remains necessary for measurement overhead, actual Mini App
paint costs, input latency, and behavior under sustained Telegram updates.

`run-chat-scroll-tests.sh` also compiles the production modal visibility predicates
and exercises every tool flag, nested tool pairs, Web App phases, link confirmation,
Classic/Combined, with/without an open chat, fullscreen exceptions and every pixel
of the tool return button. It checks stable background geometry and suppression of
chat controls, while retaining recording/tool menus. On Vita still verify opening,
closing, sending/cancelling, loading/empty/last pages, touch/controller input,
light/dark/custom palettes and all font sizes for attachments, stickers, GIFs,
reactions, search, recording, keyboards and dialogs. These host checks do not render
the complete application or send Telegram messages.

`bash tests/run-webkit-addon-tests.sh` checks the actual packager, signature domain,
background metadata lookup, download consent, signed installation, corrupted archives,
incompatible bundles, cancellation, missing/offline releases and offline re-verification
using a temporary signing key and mocked HTTPS. Loader checks compile the production
loader with Vita stubs: an absent or unverified engine is never loaded, an ABI mismatch
is rejected, callbacks cross the C interface, and reopening retains one loaded module.
`run-update-tests.sh` verifies that application and WebKit signatures cannot substitute
for each other. No production key is used by these tests.
The download worker runs with an explicitly checked 256 KiB pthread stack in every
scenario, rather than the host's larger default. Tests also reject the old default-stack
launch and cover thread-creation failure followed by retry. This guards the 0.38.54
Vita crash: the worker's 32 KiB default stack could not hold the 64 KiB archive buffer,
even before checking whether WebKit was installed.

Physical Vita checks for 0.38.54: load the actual `.suprx` from ux0 with TDLib active;
verify newlib/pthread/TLS coexistence, memory headroom, HTTPS, JavaScript, repeated
Mini Apps, lock/unlock and proxy settings. Test first-use checking/downloading/error
screens, cancellation and retry with controller/touch, light/dark/custom palettes and
all font sizes. The release must publish the signed `webkit-abi1-r1` assets before
real network installation can succeed. Host tests do not establish module-loader or
UI behavior on the console.

Bot reply focus: `run-transfer-tests.sh` checks one-shot focus requests for commands, text replies and callbacks, URL/stale-button exclusions, incoming replies, latest-history reload from an old window and generation scoping. `run-chat-scroll-tests.sh` compiles the production main-loop focus logic and verifies immediate/latest focus, incoming messages, the real pixel viewport, empty history, manual navigation and expanded post/photo exclusions. Controller/touch behavior in Classic/Combined on a physical Vita still needs checking.

Video playback quality: `bash tests/run-transfer-tests.sh` checks per-message choices, the configured maximum, original/alternative files, missing qualities, codec/size/raster rejection, old download cancellation and new download IDs. `bash tests/run-video-tests.sh` runs production menu and resume code with mocked players, including chosen badges, empty/error states, buffering, position and pause restoration, navigation and manual overrides. The maximum persists via `bash tests/run-vault-tests.sh`. On Vita, check touch/buttons, the last menu page, all palettes/font sizes, loading and playback performance at each available quality.

Keyboard contact regressions cover consecutive IDs without an empty frame, overlapping taps, reordered reports, reused IDs, simultaneous taps and typing replay after an uncompleted split gesture. `bash tests/run-keyboard-tests.sh` exercises these in the shared keyboard used by message input and vault entry. On Vita, check two-thumb typing and split/pinch gestures, system IME responsiveness and light/dark/custom themes.

Video transfer regressions verify that the 720p maximum includes 1280×720, lower maxima exclude it, and diagnostics distinguish codec, raster and quality-limit rejection. With Video diagnostics enabled, `ux0:data/vitagram/video-quality.log` records metadata and rejection reasons (rotates at 1 MiB); it contains no captions or file paths. The affected video still needs a device log to establish why its 720p variant is absent.

`bash tests/run-font-tests.sh` also checks user-font directory discovery, UTF-8 filenames,
TTF/OTF extension filtering, empty/directory/symlink/oversized inputs, the 64-entry cap,
real FreeType loading, custom-glyph priority, built-in language fallback, background loading,
cancellation and corrupt-file failure. `bash tests/run-vault-tests.sh` checks saved font
filenames and rejects paths outside the font directory. On physical Vita still verify
font switching latency/memory, unusual ascenders/descenders, touch/controller selection,
empty/loading/error/last-page states, focus versus saved selection, all font sizes and
light/dark/custom palettes. Native tests do not establish device rendering quality.

Large-video regressions: `run-transfer-tests.sh` checks the reported 170190135-byte 720p variant, download requests, completion updates and the inclusive 1 GiB boundary. `run-download-stream-tests.sh` reads a sparse 1 GiB file across 128 MiB and at EOF, waits for a growing prefix, and decodes H.264/AAC in a sparse 170 MiB MP4 with a valid trailing free atom. Sparse padding tests file sizes and offsets, not a long video’s bitrate or packet-index memory. `run-audio-playback-tests.sh` checks production video-audio startup above 128 MiB. The vault read-ahead suite writes and seeks an encrypted 170 MiB file with bounded chunks. Real Vita download, 720p decoding, seeking, audio synchronization, full-disk handling and encrypted playback still need device validation.

Keyboard draft layout checks verify that unchanged frames reuse wrapping and cursor measurements, while text edits, cursor movement and font revisions recompute them. Font scale changes advance the shared font revision. Performance logs now distinguish `keyboard_draw`, `draw_tools`, `draw_dialogs` and system `ime_update`; device logs are needed to attribute the remaining 50–166 ms stalls and verify any latency improvement.

Text raster tests wrap FreeType rasterization calls to verify that width-only queries perform zero rasterizations and repeated cached drawing adds none. Existing reference-image checks compare actual FreeType pixels, dimensions, baseline, kerning, synthetic styles, size order and bounded atlas reset. Keyboard traces now split `keyboard_draw` into `keyboard_text`, `keyboard_keys` and `keyboard_preview`. The 0.38.64 device trace confirmed two keyboard-draw stalls of 138954 and 101640 microseconds; rasterization savings are code-tested, but their effect on these stalls requires another device measurement.

Touch history regressions replay two complete taps between UI frames and a long press across a simulated render stall. They verify timestamp ordering, deduplication, same-sample nonblocking guard, fallback after read errors, opening-input suppression and clearing retained packets. The SDK read is requested only after the latest Peek timestamp advances; actual firmware buffering and read latency still need a Vita check. Renderer stalls are not claimed fixed: performance logging names key stages as `keyboard_keys_f<faces_us>_s<theme_us>_l<labels_us>_h<hints_us>`, collecting per-stage totals across all keys only while the Performance log is enabled. No typed text is logged.

FreeType size-state regressions verify 200 alternating key-label/hint measurements cause only two FT_Set_Pixel_Sizes calls, at most 64 owned sizes, fallback on allocation failure, full release on reset and restoration of the caller-owned default size. Existing pixel/reference, kerning, synthetic-style, seven UI font sizes and custom/fallback font checks still run. This targets the 0.38.66 device log: label phases took 79495–87674 us and hints up to 34407 us, while faces and theme rendering together took about 2 ms. Device latency after the change is not yet measured.

Fullscreen custom keyboard rendering skips the covered app UI and uses the shared frame completion path; native IME keeps app rendering. Keyboard tests cover custom/native ownership, automatic native-language routing and opaque palette backing even with a translucent background color. Modal transition and video suites cover affected surrounding state. GPU scene completion/swap/IME ordering is preserved in the shared `finish_frame` lambda. On-device FPS, reopening dialogs, music during typing, Combined/Classic layouts and light/dark/custom translucent wallpapers still need device validation.

The TDLib positioned-I/O regression uses a mocked Vita file API to check download-file descriptor ownership, independent offsets, interrupted calls, short writes, errors, renames and move/close. Stream tests also check that a blocked progressive read reports its position and available prefix. Real Vita storage contention and the effect on concurrent downloads still require device logs.

Video stall tests check that audio output pauses on a missing-frame signal, resumes when cleared, and the next frame can advance while the audio clock is stopped. Decoder diagnostics report the active stage and packet/frame counts. Actual hardware decode stalls, GPU display and recovery need Vita testing.

The hardware packet throttle regression reproduces a 251-packet historical gap from the 0.38.75 device log, verifies that new frames reset the counter, and keeps the bounded retry for a decoder with no output. Hardware decoding still requires Vita testing.

Seek error regression keeps the existing start-of-file fallback for a seek that cannot produce a first picture, but does not silently restart from zero after playback has produced a picture. The Vita hardware decoder failure shown in device logs remains under investigation.

Video packet pacing checks the one-second compressed-data lead, seek/pause startup, clock recovery after a stalled picture and extreme timestamps. Real 720p hardware playback and ES-buffer recovery still require Vita verification.

A reader opened on a complete download retains that file extent despite later partial/empty progress snapshots; a genuinely truncated file still returns an I/O error. The POSIX and mocked Vita positional paths run the same check.

Video A/V sync tests cover audio lead thresholds and hysteresis, pausing and resuming the video audio ring, playback lifecycle transitions and the late-frame catch-up path. Device FPS and real hardware decoder throughput remain to be measured.

Video stream buffering tests cover the 2 MiB startup gate, existing player readiness, 1 MiB refill, bounded final segment, small completed files, cancellation and the 256 KiB video AVIO buffer. Tests run against both POSIX and mocked Vita file reads; playback and bandwidth still need a physical Vita check.

Video buffer thresholds now cover 8 MiB at startup and 4 MiB after starvation; the final short segment, small completed files, cancellation and audio behavior remain covered. Large thresholds can require short clips to finish downloading before playback.

Progressive MP4 open skips trailing optional atoms when the file starts with ftyp; audio and video restore normal index handling after opening. MP4 with metadata at the end still waits for that metadata.

`python3 tests/video_decoder_drain_test.py . /tmp` checks the receive callback
from the prepared Vita FFmpeg source: delayed batches of pictures are drained
before another packet is requested; EOF drains remaining pictures; errors
propagate and submitted packets are released. Prepare FFmpeg with
`scripts/build-ffmpeg-vita.sh` first. Hardware GetPicture timing, 60 FPS playback
and absence of ES-buffer overflow still require a physical Vita.

Storage probe: `bash tests/run-storage-probe-tests.sh` checks space reserves,
exclusive file creation, cancellation, read-back verification and cleanup on the host.
The same suite also runs the Vita API branch with native flag values and checks
that creation uses mode 0666, matching VitaSDK newlib.
On Vita, create an empty `ux0:data/vitagram/storage-test.request`, launch VitaPlane
and leave it idle without playback or uploads. The background worker consumes the
request and writes `ux0:data/vitagram/storage-test.log`. It checks `ux0:`, `ur0:`
and `uma0:` without assuming their physical device mappings. Each available mount
needs 512 MiB free plus an 8 MiB temporary file budget; a probe uses independent
read/write descriptors and reports 64 KiB operation mean, p95 and maximum latency.
`overlap_read_samples=0` means the reader did not overlap the writer's measured
interval, so that run cannot establish concurrent performance. The 30-second
per-mount deadline and playback cancellation are checked between operations;
a blocked kernel I/O cannot be interrupted by this test. Interrupted runs report
`incomplete`; missing mounts and low space are skipped. A pre-existing probe file
is never overwritten or removed: after a crash, remove only the known
`<mount>data/vitagram-storage-probe.tmp` before retrying. This is a short synthetic
test, not a sustained throughput or physical-device independence guarantee.
Real Vita measurements remain required; this feature does not move media caches.

Streaming I/O diagnostics (0.38.91): slow TDLib reads, writes and syncs include
`file_dir`, `file_fd`, `file_offset` and `file_bytes` in `performance.log`.
`file_dir` contains the mount and the existing sanitized directory label;
user filenames remain omitted. Descriptors imported without a known path report
an unknown/no-directory label. Sequential reads/writes use offset -1; positional
operations report the requested offset. The performance suite tests directory
redaction, descriptor isolation, log output, and independent UI-step timers with
collection enabled and disabled. The positioned-I/O suite checks the production
FileFd trace arguments before and after moving/renaming a file. The TDLib patch
contains the same changes as the working dependency tree.
`post_pump` now reports slow `post_state`, `post_scheduler`, `post_selection`,
`post_composition`, `post_reactions`, `post_scroll_layout`, `post_touch`,
`post_picker_gestures` and `post_textures` steps. On Vita, enable the Performance
log, reproduce interrupted video playback, and collect both `performance.log`
and `performance.previous.log`. Correlating actual card stalls and UI freezes
still requires device validation; these changes add attribution, not a storage fix.

CPU report contention (0.38.92): `bash tests/run-cpu-scheduler-tests.sh` builds the
production CPU scheduler with mocked Vita calls and blocks its report output.
UI ticks, playback transitions, parser pinning and status queries must remain
responsive while output is blocked. Samples collected meanwhile must appear in
the next report, and reporting must reset each interval only once. On Vita,
repeat streaming playback with Performance logging enabled and compare
`post_scheduler` stalls and frame delays. This fixes scheduler-lock contention
from diagnostic output; database and media writes can still stall on storage.

Streaming recovery (0.38.93): `bash tests/run-video-tests.sh` now exercises the
production packet-wait loop with pictures completing asynchronously. A ready
picture must resume stalled playback without waiting seconds for the pacer's
virtual clock. Paused seek preview, cancellation, decoder errors and the existing
compressed-input bounds remain covered. The change polls output while waiting;
it does not increase the permitted packet lead.
`bash tests/run-td-positioned-io-tests.sh` also runs the production BinlogActor
flush scheduling methods. Vita ordinary writes batch for up to 100 ms (other
platforms keep 1 ms), without extending a pending batch when new records arrive.
Forced flush remains immediate; forced sync keeps its existing no-later-than-3-ms
alarm and closing still persists pending data. The existing 16 KiB eager-flush
threshold, encryption, retry handling and persistence promises are unchanged.
Real Vita must verify the effect on database write frequency, download throughput,
startup, audio/video continuity and sleep/resume; mocks establish control flow,
not card performance or hardware-decoder timing.

Video-quality log isolation (0.38.94): `bash tests/run-transfer-tests.sh` checks
that parsing video metadata performs no stat/open on `video-quality.log`, even
when Video diagnostics are enabled. The test blocks the background log open
and verifies another message can still be parsed/enqueued; queued reports are
written as a separate batch. It also checks a 64-report queue limit, an 8 KiB
variant-report limit with an omission marker, and disabling diagnostics.
The cache worker flushes at its diagnostic interval only outside active video
and progressive reading, plus shutdown. Rotation and file writes hold no queue
mutex. On Vita, reproduce the reported 3–5-second `process type=message` stalls
with Video/Performance logs enabled; after closing playback, collect both
Performance files and `video-quality.log`. Real card timing remains unverified.

Settings navigation (0.38.95): `bash tests/run-chat-scroll-tests.sh` also runs
`python3 tests/settings_menu_test.py`. It compiles the production table and
return/open handlers, checks all 34 settings/actions occur exactly once,
parent selection/scroll restoration, confirmation cancellation, encryption
dependencies, and every pixel of row/back hit bounds including gaps, unused
space and the last partially visible row. `bash tests/run-i18n-tests.sh` checks
all 19 catalogs and packaged font character coverage. Existing card rendering
is reused without new rounded translucent fills; the mask/skin suite remains
`bash tests/run-ui-mask-tests.sh`. These checks do not render the full app.
On physical Vita verify controller/touch traversal, proxy QR cancellation,
keyboard language return, theme/font picker empty/loading/chosen/last-page
states, logout/encryption confirmations, and light/dark/custom palettes at
all font sizes. No saved-setting migration is required.

Custom language checks in `run-i18n-tests.sh` cover catalog discovery, overrides,
English fallback and rejected mismatched file codes. `run-keyboard-tests.sh` loads
a new profile from a temporary .lang, exercises touch input, registration and
native-IME fallback, rejects malformed rows and checks invalid UTF-8. Vita file
discovery, custom glyphs, touch geometry and actual native IME still need a device.

Language dropdown (0.38.97): `python3 tests/settings_menu_test.py` also compiles
the production language opener and confirmation handler: saved selection, automatic
mode, cancel without applying, unknown saved code and asynchronous apply request.
On Vita check touch/controller scrolling through the last page, language font
loading, and light/dark/custom palettes at all font sizes.

MP4 tail ranges (0.38.98): `bash tests/run-download-stream-tests.sh` checks a shared,
bounded tail window, readable head/gap/tail extents, fallback on timeout/error,
reader cancellation, invalidation, completion and real H.264/AAC startup before
full download through POSIX and mocked Vita reads with bounded read-ahead.
`bash tests/run-transfer-tests.sh` checks range request parameters, replacement of
unsent retries, preserved head progress, full-download restoration and cancellation.
Only MP4 opening can request one tail of at most 8 MiB; ordinary playback gaps
and seek limits keep the sequential path. A 30-second range deadline falls back
to the full download. Real Vita must check nonsequential file growth/free space,
card stalls, Wi-Fi loss, close/reopen, quality switching, sleep/resume, A/V sync
and cache encryption after completion; host tests do not establish card behavior.

Chat calendar checks in `run-chat-scroll-tests.sh` cover leap years, month bounds, future dates, Monday-first cell layout, gaps and unused cells, and calendar modal visibility. On Vita verify floating date overlap, touch and controller month/day navigation, empty/error/offline history, stale replies after switching chats/topics, and light/dark/custom themes at every font size.

Slow Vita tail writes (0.39.01): the stream suite advances the coordinator clock
by the observed 58.48-second file-growth stall, verifies that the tail remains
active, accepts late confirmed bytes after fallback and retries after reopening.
MP4-tail and active decoder waits now allow 120 seconds; ordinary sequential
waits retain their 60-second limit. Performance stream entries include tail
extents and active state. Real Vita still needs to verify file-growth latency,
first playback, reopening and cancellation; increasing the wait does not remove
the storage cost of extending the temporary file.

MP4 event ordering (0.39.02): the transfer suite delivers a newer live message
before an older queued file update and verifies that the message publishes the
readable tail before suppressing that update. Older messages and cached reparses
must not rewind the stream. Real Vita must still confirm playback after the
observed storage stall; this fix does not reduce file-extension latency.

VitaPlane 0.39.03 checks: keyboard regressions cover L/R without a stale letter
preview and touch cursor placement across UTF-8, explicit/soft line breaks and
scrolled drafts. Settings regressions cover optional per-download folder prompts,
confirm/cancel, unchanged default folders and stable message identity; modal checks
cover folder ownership in Classic/Combined. Updater lifecycle tests query both
repository names, missing repositories, newest-version selection and invalid
metadata. Audio/video checks cover background ordinary clips, speed-panel hit
bounds, media clocks at 0.5x/1x/2x and silent-video clock scaling. The packet-pacing
fixtures use the existing four-second paused-preview boundary. Account paths,
cryptographic labels, signature domains and title IDs remain compatible.
Run keyboard, i18n, chat-scroll, updater-lifecycle, webkit-addon, audio-playback,
video, transfer and vault suites. Real Vita still needs checking for L/R visuals,
touch cursor accuracy, destination browsing/cancellation, channel SELECT, video
speed/A-V sync, decoder capacity, sleep/resume and all palettes/font sizes.

Keyboard magnifier (0.39.04): `bash tests/run-keyboard-tests.sh` checks the 350 ms
threshold, themed opaque lens bounds, all seven font metrics, UTF-8 caret
boundaries, multiline dragging, fixed viewport ownership, bounded edge scrolling,
release/cancel/clear/reopen, second-contact isolation and password/covered-screen
exclusion. The lens reuses the keyboard's rounded fan/fringe renderer and never
copies a framebuffer or captures a password. Physical Vita still needs touch
accuracy, finger occlusion, font rendering, drag smoothness and edge-scroll pacing
checks with light/dark/custom themes, split keyboard and slow frames.

Save-as follow-up (0.39.04): chat-scroll checks include the production asynchronous
directory picker: remembered/configured/built-in fallback paths, folder-first file
lists, files never entered as folders, creation/navigation, read errors, 5000-entry
cap, blocked storage with immediate close/reopen and stale-result rejection.
Every pixel of rows/footer is checked, including gaps and unused last-page cells.
Vault checks persist the last destination independently of the default folder;
settings checks confirm cancellation does not change it. The header/path/name/list
occupy separate vertical bands; each footer label fits its own touch zone.
Physical Vita still needs mounted/unmounted drives, real-card latency, long UTF-8
paths/names, all font sizes and light/dark/custom themes.

Text editing (0.39.05): transfer checks run production permission/parser/edit
handlers for own sent text, absent/denied rights, 1023-byte capacity, automatic
versus manual entities, media exclusions, unchanged/invalid UTF-8 input, retained
preview settings, no new message/reply/topic fields, duplicate submit, server
failure with draft recovery, retry identity, external text conflicts, timeout,
cancel and stale results after navigation. Keyboard checks verify the Save label
and unchanged initial text. Run transfer, keyboard, i18n and chat-scroll suites.
Physical Vita still needs menu/controller/touch flow in Classic/Combined,
channels, topics and Saved Messages; expired edit rights, offline/reconnect,
custom/native keyboard, all fonts/palettes and live edits from another device.

VPK dialog lifecycle (0.39.06): `bash tests/run-vpk-tests.sh` also runs the production
installer and promoter module preparation with mocked Vita APIs under ASan/UBSan.
It checks worker-only preparation before native dialogs, reuse of preloaded PAF,
module load/start failures, failed worker/dialog creation, cancelled confirmation,
invalid metadata, promotion failure, ordinary success and self-update handoff.
Module loads must never occur while a native dialog is active. Physical Vita
still needs repeated installations, the final success message, helper installation,
controller input and return to chat; this addresses the suspected lifecycle conflict,
not a proven source of the corrupted PAF dimensions in the reported dump.

`bash tests/run-web-proxy-tests.sh` checks the native WEB HTTPS adapter against a
local Python TLS relay: concurrent binary streams, 503 retry, byte-identical
replay after a lost acknowledgement, hostname verification, base paths, config
replacement, cancellation, restart and session deletion. It needs host OpenSSL,
zlib and Jansson headers/libraries (Jansson headers from `build-modern`), Python 3
and the `openssl` command. DNS/service wrapping is confined to the test binary;
production uses HTTPS/443 with the packaged CA bundle. This is a protocol fixture,
not the official relay or a physical Vita test. `run-proxy-tests.sh` also covers
WEB link import, real QR decoding and published capability test vectors.

WEB proxy thread regression: the local TLS test intercepts native thread creation
and requires an explicit 256 KiB stack for both the persistent worker and each
downlink. Injected stack-attribute and thread-start failures must report a clean
error; a failed downlink start must never publish a ready loopback port. This
prevents reliance on Vita's 32 KiB default, which crashed during CA bundle loading.
These are host checks; stack headroom and the reported crash still need a Vita retest.

`bash tests/run-proxy-pool-tests.sh` checks bounded proxy-list policy, sequential
probe results, sustained-failure grace, latency ranking, switch cooldown, offline
and resume handling, stale results, empty lists and shared render/touch geometry
including gaps, footer actions and the last page. The settings test covers legacy
migration, duplicate import, active-entry deletion, atomic write failure, credential-encryption failure and
worker-only file I/O, plus vault-sealed credentials. Requires the same host
OpenSSL/Jansson headers and libraries as the existing vault tests. Transfer tests
also exercise real TelegramClient measurement generations, malformed ping values
and proxy-only failover. Real Vita UI, wireless failures and media transfers still
need device checks.

Proxy connection regressions: transfer tests verify that manual and list checks work once TDLib parameters are queued, without depending on a separate UI acknowledgement, proxy application stays FIFO with network controls and passes interaction/backlog gates, and DNS/secret failures are classified without revealing addresses or credentials. Device connectivity still requires a physical Vita retest.

The WEB relay test forces system-DNS failure and verifies authenticated HTTPS DNS fallback, TTL-cache reuse by both stream directions, invalid/no-answer DNS rejection, cancellation and original proxy hostname certificate checks.

`bash tests/run-proxy-diagnostics-tests.sh` verifies proxy logging starts automatically before menu access, can be explicitly disabled, accepts only fixed stages/numeric fields, bounds concurrent producers, rotates two bounded files, reports overflow/write loss and performs file I/O only on a background worker. It needs only a C++17 host compiler and pthreads.

WEB relay tests additionally inspect emitted DNS/TCP/TLS/session events, hostname-verification error codes and HTTP 403 session rejection, and verify that hostnames, secrets, bearer tokens and response bodies are absent from the log. Transfer tests check correlated queued/dispatched operations, TDLib errors and logout cleanup of both proxy log files. Controller/touch access to the seventh diagnostic switch, Vita storage writes/rotation and real network failures still need physical-device checks.

`bash tests/run-sqlite-vita-tests.sh` compiles the pinned SQLite/SQLCipher source with the Vita path/locking branch, no WAL or mmap, and POSIX record locks forced to fail with ENOSYS. It verifies both Vita volume path forms, bounded path output, encrypted transactions, lock contention and reopen without replacing Vita production libraries. Requires host C compiler, OpenSSL, zlib and pthreads. These are host checks; real Vita filesystem durability and crash recovery still need device verification.

The WEB relay fixture rejects Content-Type on empty downlink and DELETE requests, matching the reference server. Transfer tests verify TDLib initialization rejection cancels pending checks, blocks further probe dispatch and records only a fixed failure classification, never private error text.


Keyboard emoji/draft view (0.39.20): `bash tests/run-keyboard-tests.sh` covers
picker input ownership, grid gaps, recent choices, held variants, complete-sequence
insertion/deletion/cursor movement, byte limits and secret exclusion. Collapsed
view checks cover expanded text, swipes/controller scrolling, quick-tap restore,
long-hold magnification and release ownership. Existing rounded geometry,
light/dark/custom palette, seven font-size and touch-history checks still run.
`bash tests/run-font-tests.sh` verifies every generated sequence against the real
packaged font, full-glyph rasterization, aliases, custom-text-font independence,
shared widths and text presentation. It also extracts the actual message wrapping
and clipping functions to check complete emoji boundaries.
`bash tests/run-text-layout-tests.sh` checks cluster-preserving fitting;
`bash tests/run-i18n-tests.sh` checks all 19 catalogs. These suites need the host
HarfBuzz shared library to generate the catalog; font tests also need host FreeType.
On Vita verify emoji centering, chosen/unfocused and focused/unchosen variants,
empty recent and last pages, all palettes/font sizes, real touch scroll/hold timing,
magnifier placement, CDRAM and frame latency. Host checks do not establish device
rendering, touch feel or installation behavior.


Emoji loading fix (0.39.22): font-suite frame-tail checks compile the production
queue-drain condition and cover custom/native keyboards, input and decode cooldown.
Keyboard palette/font checks compare used and unused tile fills; used items have
no persistent highlight or check badge. Verify on Vita that initially missing
emoji progressively appear while the custom keyboard is idle, including the
variant popup and draft, without requiring the keyboard to close.


Emoji variant gestures (0.39.24): keyboard checks exercise several emoji at the
left, middle and right columns, horizontal selection while the finger remains on
the original key row, moving into the popup, preserving the last variant under
the initial contact, larger-set edge scrolling, release-only insertion and
out-of-panel cancellation. Actual finger tracking and latency need a Vita check.


Emoji controls/loading (0.39.26): keyboard tests verify variant prefetch at the
first contact, emoji Send/Save with empty-message validation, lower-left collapse,
upper-right close in expanded/collapsed views, and unchanged secret-field input.
Font tests check bounded priority-queue promotion, deduplication, cache hits and
actual rasterization. Frame-tail checks cover decoding throughout held custom
keyboard gestures while protecting native IME and idle background cooldown.
On Vita verify first-use variants appear during the initial hold, controller
Circle icon placement, Send/Save and collapse/close in all palettes/font sizes;
measure frame latency and memory on device.


Emoji category list (0.39.27): keyboard checks verify short-press cycling,
long-press opening, ignoring the opening release, vertical drag scrolling, the
last page, row gaps/outside taps, underlying Send isolation, controller selection,
Circle/touch return and all seven font sizes with light/dark/custom palettes.
Run keyboard and i18n suites. Physical Vita checks remain for scroll feel,
focus/text clipping, touch accuracy and opening/cancelling with held controls.

Proxy auto-switch delay (0.39.27): run `bash tests/run-proxy-pool-tests.sh`,
`bash tests/run-transfer-tests.sh` and `bash tests/run-i18n-tests.sh`. Policy tests
cover each 5/10/15/30/60-second boundary, post-switch delay, disabled automatic
mode, offline/resume and healthy-connection stability. Settings tests cover
worker-written persistence, missing/invalid value defaults and protected secrets.
Geometry tests cover interval cell borders and reject horizontal/vertical gaps.
Physical Vita still needs controller/touch checks for empty/full/scrolled lists,
chosen values with and without focus, light/dark/custom palettes, larger fonts,
and reconnect/failover timing on a real network.


Chat activity (0.39.28): `bash tests/run-transfer-tests.sh` covers unread-mention
counts, oldest-first search parameters, stale and duplicate results, typing sender,
cancel/expiry/topic isolation, visible-message read batching, grouped albums,
offline/loading guards and restricting comment probes to presented messages.
`python3 tests/chat_modal_test.py` compiles the shared mention-button geometry
and checks the whole circular hit area, older/partial/latest/empty history and
changed viewport height. `bash tests/run-i18n-tests.sh` checks all catalogs.
Physical Vita must verify live Telegram mention navigation/read synchronization,
touch ownership, typing latency and light/dark/custom palettes at every font size.

Proxy delay visibility (0.39.29): `bash tests/run-proxy-pool-tests.sh` checks
compact rendering/hit-test indices with the interval hidden, navigation in both
directions, empty/full/scrolled lists, gaps and unused cells. Vita still needs
checking for the Yes/No rendering and toggling the interval row on and off.


Keyboard word wrapping (0.39.31): `bash tests/run-keyboard-tests.sh` checks
whole-word wrapping for Latin/Cyrillic text and complete emoji sequences, caret
and touch positions on both sides of a wrap, reflow after insertion/deletion,
explicit newlines and existing oversized-word fallback. The suite uses
ASan/UBSan; actual glyph placement and touch feel still require a Vita check.


Typing placement (0.39.33): `python3 tests/chat_modal_test.py` compiles the
production drawing block and checks the row below the date chip in Classic
and Combined layouts, long names, no-date fallback and empty/hidden states.
The preview in `build-modern/previews/typing-under-date.png` is an illustrative
layout, not a Vita screenshot. Live Telegram typing delivery still needs a device check.

VPK app progress (0.39.34): `bash tests/run-vpk-tests.sh` runs 16 installer
scenarios and the production progress renderer under ASan/UBSan. A stalled
promoter/helper must leave the UI callback responsive with no active native
dialog; checks cover both confirmations, errors, repeated installs, extraction
percentages and self-update handoff. Renderer checks cover bar bounds, sweep
turnaround/wrap, long/Unicode labels, font scaling and palette lookups.
Physical Vita must still check installation, the final result dialog, return to
chat, a second install in the same session, helper handoff, actual font rendering
and light/dark/custom palettes. This removes the observed overlap; it does not
establish who originally corrupted PAF surface dimensions in the dump.

Chat spacing and lower-edge cue (0.39.34): `bash tests/run-chat-scroll-tests.sh`
checks the 28-pixel avatar gutter, the content inset and history touch bounds in
Classic/Combined, and the lower-message predicate for clipped cards, later loaded
rows, older history windows, empty history and the exact bottom boundary.
Check glow visibility, avatar padding and upper clipping on a physical Vita with
light/dark/custom palettes, all font sizes and music/download/bot panels.

Top outline regression (0.39.35): chat-scroll checks keep the content inset
inside the original clip and compare the actual outer bubble mesh against that
clip in Classic/Combined, including joined cards. On Vita check first-message
focus, upward scrolling and date/typing overlays.


Unread counters (0.39.36): `bash tests/run-chat-scroll-tests.sh` checks the
production counter/dot/@ renderer in Classic/Combined at all seven font sizes
with light/dark/custom notification colors, full INT_MAX counts, marked-unread
zero-count chats, bounded title space and sampled single-coverage rounded
fill/AA geometry. Settings tests cover the Chats entry, toggle and save errors;
vault tests cover persistence and old/malformed-file defaults. Run
`bash tests/run-vault-tests.sh` and `bash tests/run-i18n-tests.sh` as well.
Physical Vita still needs text/halo rendering, controller/touch navigation,
restart persistence and live read/mention updates in both views and all palettes.


Unread counter input fix (0.39.37): the settings test now runs the production
input gate and menu-ID mapping before the toggle condition. Cross, Left, Right,
repeated toggles, no input, Circle, save failure, touch-row selection and another
menu section are covered. The regression fails against the previous handler.
Run `bash tests/run-chat-scroll-tests.sh`; actual controller/touch input and
persistence after restarting on Vita still need a device check.


Muted chat badges (0.39.38): chat-scroll tests run both saved settings through
actual menu-ID/input routing and check that muted/read/unread chats replace
counts and dots with the existing speaker-with-cross control, retaining @,
across both layouts and all font sizes. Transfer tests cover explicit overrides,
private/group/channel scopes, absolute mute expiry, offline snapshots, live
updates and stale query replies. Vault tests cover persistence/defaults; i18n
tests cover all catalogs. Physical Vita must still check the icon, controller/
touch toggles, restart/offline appearance and live mute/unmute/expiry updates.

Attachment recovery/order (0.39.39): `bash tests/run-tdlib-upload-tests.sh` compiles
the production Vita recheck branch with simulated file validation/registration:
changed size/mtime, absent or forbidden files, registration failure, identity
collisions, and active upload/download/remote/generation guards. It does not run
TDLib actors or actual card I/O. `bash tests/run-transfer-tests.sh` checks mixed
selection order, equal send priorities, separate progress for repeated paths,
album response priority, and diagnostic tokens. `bash tests/run-upload-picker-tests.sh`
checks marking/removing/re-adding, review and batch order. Physical Vita checks
still need growing log uploads, mixed albums/documents, repeated sends, missing
files, and completion/status behavior after reconnects.

Upload network pressure (0.39.40): `bash tests/run-td-dns-tests.sh` checks
production SocketFd/IPAddress with wrapped system calls: 512 KiB exact-byte
transmission in at most 16 KiB writes, ENOBUFS retry without offset loss,
read backpressure, retained connection-reset errors, numeric IPv4/port checks
and no system DNS call for numeric addresses. The script also compiles the
production resolver constructor for Vita and host, checking HTTPS-only policy
for both default and fallback chains on Vita and unchanged host chains.
`bash tests/run-i18n-tests.sh`
also verifies that VitaNet provides its 4 MiB pool to sceNetInit. These are host
checks; Vita Wi-Fi state, SDK network-pool recovery and real upload stability
still require device testing. The Vita Telegram proxy resolver uses HTTPS DNS only;
when it is unavailable, resolution fails without entering blocking system DNS.

Premium emoji (0.39.41): `bash tests/run-keyboard-tests.sh` checks annotation
positions beside identical emoji, insertion/deletion, UTF-16 boundaries, static
draft rendering and byte limits. `bash tests/run-transfer-tests.sh` checks incoming
custom emoji spans, outgoing entities, preview lookup, installed custom emoji
packs and switching back to regular stickers. `bash tests/run-i18n-tests.sh` checks
the new actions in every language. On physical Vita verify inline previews in
messages/captions/drafts, spoilers, last/empty/loading pages, controller/touch,
light/dark/custom palettes, font sizes, Premium permission errors and installation.

Animated premium emoji (0.39.42): `bash tests/run-reaction-tests.sh` checks the
production four-slot animation pool, shared instances, deferred source changes
and uploads, offscreen/paused cancellation, chat/path scoping and pending downloads.
`bash tests/run-transfer-tests.sh` checks TGS/WebM metadata, lazy full-file downloads,
size limits, completion/reset updates and stale responses. TGS policy and WebM
alpha suites cover the existing decoder helpers. Actual inline TGS/WebM timing,
transparency, four simultaneous decoders, keyboard responsiveness, scrolling,
video priority, fonts/palettes and memory consumption still require physical Vita.

Inline animation continuity (0.39.43): the reaction suite runs 120 stable render
frames and a pause/resume without reopening sources or replacing the frozen frame.
The transfer suite covers thumbnail-less animation metadata beyond the retry
timeout, foreground emoji reply routing, and Storage-validated file publication
during an interactive keyboard frame. Performance logs include numeric emoji
metadata, asset, source, thread-start failure, decoded-frame and GPU-upload counters.
These host checks do not establish real Vita animation smoothness or GPU output.

Sent premium emoji (0.39.44): transfer tests exercise a pending message, its
confirmed replacement and a later content-only custom emoji upgrade without
changing the Unicode text. The visible model updates before any network reload,
retains sender/date/delivery metadata, scopes updates by chat, respects removal
of custom entities by Telegram and returns detached cache copies. Uncached content
updates still request the complete message. Actual rendering after send needs Vita.

Single premium emoji wire type (0.39.45): transfer regressions use TDLib's actual
`messageAnimatedEmoji.animated_emoji.sticker.full_type.custom_emoji_id` shape.
They cover pending formatted text becoming an animated emoji on send confirmation,
byte spans and media lookup, the compacted history wire route, later animated
content changes, unchanged sender/date and ordinary animated emoji without a custom
ID. This specifically catches the Unicode-only fallback for single premium emoji.
Physical Vita rendering after installation remains a separate check.

Dense premium emoji (0.39.46): transfer tests parse an 80-item, eight-line mosaic
with distinct IDs and UTF-16 offsets, fetch metadata in 32/32/16 batches and
complete all 80 missing-thumbnail assets with four concurrent downloads. Font
tests exercise the production TextureCache demand collection for 80 distinct IDs,
repeated IDs, existing previews and paused frames. For real TGS snapshot rendering,
run `RLOTTIE_NATIVE_BUILD=/path/to/native/rlottie-build bash tests/run-image-tests.sh`
with the pinned rlottie built natively as a static library. This checks transparent
RGBA pixels, cancellation, JSON bounds and embedded-image rejection using the
synthetic fixture. Host checks do not establish Vita rendering or memory/performance
with dense posts; scrolling and replacing all fallback symbols need device checks.
The video thumbnail regression accepts an optional WebM fixture and an `alpha`
argument: `bash tests/run-video-thumbnail-tests.sh /path/to/alpha.webm alpha`.
It exercises the actual image-worker prefix and checks transparent and opaque
pixels. This reproduced the old MP4/GIF-only whitelist failure before the fix.

VPK menus (0.39.47): the VPK suite checks the app confirmation and result
screens, including held input, cancellation priority, touch gaps, drag-out,
multiple contacts and acknowledgment. The installer test links without
native message dialog stubs and verifies that both confirmations precede
promotion. Check actual text and touch on Vita in light, dark and custom
palettes, then install twice in the same session and dismiss each result.

Premium emoji download recovery (0.39.47): the transfer regression exercises an
80-emoji batch with a stalled file, an explicit TDLib error and a file that
continues to receive bytes. It checks that fresh files take precedence over
retries, all 80 eventually get previews, and a late error from an old attempt
cannot interrupt a new attempt.

`bash tests/run-telegram-call-tests.sh` checks the TDLib call protocol declaration,
base64 key validation, 64-bit reflector ordering, separate STUN/TURN entries,
and protocol negotiation. The call window, audio and interoperability
with an official Telegram client require a call-enabled build on a physical Vita.
The call update fixture covers updates received before `callId`, stale IDs,
the transition to key exchange, numeric errors, incoming acceptance and history
results. It verifies that unsolicited ready updates can't start incoming audio.
It checks send-delay calculations with packet counter resets and missing samples.
The sound fixture checks distinct waveforms, cancellation and audio-port release
with a mocked output device.
`python3 tests/chat_modal_test.py` also checks that the call window hides chat
actions, that each call phase has the expected touch actions and that call-card
edges match their touch area. Themes and font sizes require a physical Vita check.
