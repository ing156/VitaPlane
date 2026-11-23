#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-call-tests.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" \
    -I "$root/build-modern/external/jansson/include" "$root/src/telegram_call_setup.cpp" \
    "$root/tests/telegram_call_setup_test.cpp" -l:libjansson.so.4 -lcrypto -o "$work/call_setup"
"$work/call_setup"
printf '%s\n' 'Telegram call setup: PASS'
python3 - "$root" "$work" <<'PY'
import pathlib
import sys

root, work = map(pathlib.Path, sys.argv[1:])
source = (root / 'src/telegram_calls.cpp').read_text()
fixture = (root / 'tests/telegram_call_progress_test.cpp').read_text()
handler = source.split('bool TelegramClient::processAudioCall', 1)[1].split('void TelegramClient::pumpAudioCall', 1)[0]
accept = source.split('bool TelegramClient::acceptAudioCall', 1)[1].split('std::string TelegramClient::audioCallDiagnostic', 1)[0]
delay = source.split('int packet_delay_ms(', 1)[1].split('std::atomic<bool> call_engine_started', 1)[0]
client = (root / 'src/telegram_client.cpp').read_text()
record = client.split('if (!std::strcmp(type, "messageCall")) ', 1)[1].split(' else if (!std::strcmp(type, "messagePoll"))', 1)[0]
(work / 'call_progress.cpp').write_text(fixture
    .replace('// CALL_UPDATE_HANDLER', 'bool TelegramClient::processAudioCall' + handler)
    .replace('// ACCEPT_CALL', 'bool TelegramClient::acceptAudioCall' + accept)
    .replace('// SEND_DELAY', 'int packet_delay_ms(' + delay)
    .replace('// PARSE_RECORD', record))
PY
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" \
    -I "$root/build-modern/external/jansson/include" "$root/src/telegram_call_setup.cpp" \
    "$work/call_progress.cpp" -pthread -l:libjansson.so.4 -lcrypto -o "$work/call_progress"
"$work/call_progress"
printf '%s\n' 'Telegram call updates, acceptance and history records: PASS'
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" -I "$root/tests/audio-playback-stubs" \
    "$root/src/call_sound.cpp" "$root/tests/call_sound_test.cpp" -pthread -o "$work/call_sound"
"$work/call_sound"
printf '%s\n' 'Call sounds, phase changes and audio-port release: PASS'
