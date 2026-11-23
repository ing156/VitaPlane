#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-https-resume.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I "$root/src" -I "$root/external/jansson/src" \
  -I "$root/build-modern/external/jansson/include" "$root/src/update_core.cpp" "$root/src/https_client.cpp" \
  "$root/tests/https_resume_test.cpp" -lssl -lcrypto -lz -l:libjansson.so.4 -pthread -o "$work/test"
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=localhost \
  -addext subjectAltName=DNS:localhost -keyout "$work/key.pem" -out "$work/cert.pem" >/dev/null 2>&1
python3 -I - "$work" <<'PY'
import pathlib, socketserver, ssl, subprocess, sys, threading
work = pathlib.Path(sys.argv[1])
class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        data = b''
        while b'\r\n\r\n' not in data:
            part = self.request.recv(4096)
            if not part: return
            data += part
        path = data.split(b' ')[1].decode()[1:]
        ranged = b'Range: bytes=3-\r\n' in data
        if ranged: assert b'If-Range: "old"\r\n' in data
        if not ranged or path == 'fallback':
            response = b'HTTP/1.1 200 OK\r\nContent-Length: 6\r\nETag: "new"\r\n\r\nabcdef'
        elif path == 'redirect':
            response = b'HTTP/1.1 302 Found\r\nLocation: /valid\r\nContent-Length: 0\r\n\r\n'
        elif path == 'range416': response = b'HTTP/1.1 416 Range Not Satisfiable\r\nContent-Length: 0\r\n\r\n'
        else:
            ranges = {'wrong-start':'bytes 2-5/6', 'overflow':'bytes 18446744073709551619-5/6',
                      'negative':'bytes -3-5/6', 'wrong-total':'bytes 3-5/7', 'wrong-end':'bytes 3-4/6'}
            cr = ranges.get(path, 'bytes 3-5/6')
            etag = '"new"' if path == 'changed' else '"old"'
            length = '9223372036854775808' if path == 'overflow-length' else '3'
            headers = f'Content-Range: {cr}\r\nContent-Length: {length}\r\nETag: {etag}\r\n'
            if path == 'duplicate': headers += 'Content-Range: bytes 3-5/6\r\n'
            response = ('HTTP/1.1 206 Partial Content\r\n' + headers + '\r\n').encode() + (b'd' if path == 'cut' else b'def')
        self.request.sendall(response)
class Server(socketserver.TCPServer):
    allow_reuse_address = True
    def get_request(self):
        sock, addr = super().get_request()
        return context.wrap_socket(sock, server_side=True), addr
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(work/'cert.pem', work/'key.pem')
with Server(('127.0.0.1', 0), Handler) as server:
    thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
    for mode in ['valid','fallback','redirect','changed','range416','wrong-start','wrong-end','wrong-total','overflow','negative','duplicate','overflow-length','cut']:
        subprocess.run([str(work/'test'), f'https://localhost:{server.server_address[1]}', str(work/'cert.pem'), mode], check=True)
    server.shutdown(); thread.join()
print('HTTPS resume: range/validator/redirect/restart/framing/truncation: PASS')
PY
