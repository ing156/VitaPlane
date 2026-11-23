#!/usr/bin/env python3
"""Local HTTPS protocol fixture; no account, live proxy or private media required."""
import hashlib
import hmac
import http.server
import os
import pathlib
import socket
import ssl
import struct
import subprocess
import sys
import threading
import base64
import json
import urllib.parse

TOKEN = 'A' * 43
SESSION = 'B' * 43
KEY = bytes(range(16))

def frame(kind, stream=0, payload=b''):
    return bytes([kind]) + stream.to_bytes(3, 'big') + struct.pack('>I', len(payload)) + payload

def decode(body):
    result = []
    while body:
        assert len(body) >= 8
        kind, stream, size = body[0], int.from_bytes(body[1:4], 'big'), int.from_bytes(body[4:8], 'big')
        assert len(body) >= 8 + size
        result.append((kind, stream, body[8:8+size]))
        body = body[8+size:]
    return result

class Relay(http.server.ThreadingHTTPServer):
    daemon_threads = True
    def handle_error(self, request, client_address):
        pass  # Cancellation deliberately interrupts TLS and pending long polls.

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def log_message(self, *args):
        pass  # Headers and bridge URLs contain bearer credentials.
    def response(self, code, body=b'', **headers):
        self.send_response(code)
        self.send_header('Content-Length', str(len(body)))
        for key, value in headers.items():
            self.send_header(key.replace('_', '-'), value)
        self.send_header('Connection', 'close')
        self.end_headers()
        self.wfile.write(body)
        self.close_connection = True
    def do_GET(self):
        if self.path.startswith('/resolve?'):
            query=urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
            assert query.get('type') == ['A']
            host=query['name'][0]
            if host == 'malformed.example.com':
                return self.response(200,b'{invalid')
            result={'Status':3} if host == 'nxdomain.example.com' else {
                'Status':0,'Answer':[{'type':1,'TTL':60,'data':'999.1.1.1' if host=='noanswer.example.com' else '127.0.0.1'}]}
            return self.response(200,json.dumps(result).encode())
        prefix = 'AbC_1' if self.path.startswith('/AbC_1/') else ''
        context = ('tdesktop-web-proxy-bridge-v2\nproxy.example.com\n' + prefix if prefix else
                   'tdesktop-web-proxy-bridge-v1\nproxy.example.com').encode()
        capability = base64.urlsafe_b64encode(hmac.new(KEY, context, hashlib.sha256).digest()).decode().rstrip('=')
        assert self.path == ('/' + prefix if prefix else '') + '/?bridge=' + capability
        self.response(200, ('const relayBase="/",bootstrap="' + TOKEN + '",carrierMode="https";').encode())
    def do_POST(self):
        body = self.rfile.read(int(self.headers['Content-Length']))
        assert self.headers.get('Content-Type') == ('application/octet-stream' if body else None)
        assert self.headers.get('Cookie') is None
        endpoint = self.path.split('/')[-1]
        state = self.server.state
        if endpoint == 'session':
            if self.server.mode == 'denied':
                return self.response(403,b'private-relay-debug-secret')
            assert self.headers['Authorization'] == 'Bearer ' + TOKEN
            assert body == frame(0x10, payload=b'\x01')
            with state['cv']:
                state.update(sequence=0, up=b'', cursor=0, down=b'', queue=[], streams=set(), credit={})
            return self.response(200, frame(0x11), X_Session_Token=SESSION, X_Down_Cursor='0', X_Carrier_Mode=self.server.mode)
        assert self.headers['Authorization'] == 'Bearer ' + SESSION
        if endpoint == 'up':
            sequence = int(self.headers['X-Up-Seq'])
            with state['cv']:
                if not state['busy_once']:
                    state['busy_once'] = True
                    return self.response(503, Retry_After='1')
                if sequence == state['sequence']:
                    assert body == state['up']
                    state['replays'] += 1
                else:
                    assert sequence == state['sequence'] + 1
                    for kind, stream, payload in decode(body):
                        if kind == 1:
                            assert stream not in state['streams']
                            state['streams'].add(stream)
                            state['credit'][stream] = 4 * 1024 * 1024
                        elif kind == 2:
                            assert stream in state['streams']
                            state['queue'] += [frame(2, stream, payload), frame(4, stream, struct.pack('>I', len(payload)))]
                            state['bytes'] += len(payload)
                        elif kind == 3:
                            state['streams'].discard(stream)
                        elif kind == 4:
                            state['credit'][stream] += int.from_bytes(payload, 'big')
                        else:
                            assert kind == 6
                    state['up'], state['sequence'] = body, sequence
                    state['cv'].notify_all()
                    if not state['lost_once']:
                        state['lost_once'] = True
                        self.close_connection = True
                        self.connection.shutdown(socket.SHUT_RDWR)
                        self.connection.close()
                        return
            return self.response(204, X_Up_Ack=str(sequence))
        assert endpoint == 'down' and not body
        cursor = int(self.headers['X-Down-Cursor'])
        with state['cv']:
            if cursor == state['cursor']:
                if not state['queue']:
                    state['cv'].wait(0.1)
                selected, remaining, blocked, size = [], [], set(), 0
                for item in state['queue']:
                    kind, stream, payload = decode(item)[0]
                    if size + len(item) > 2 * 1024 * 1024 or (kind == 2 and
                            (stream in blocked or len(payload) > state['credit'][stream])):
                        remaining.append(item)
                        if kind == 2:
                            blocked.add(stream)
                    else:
                        selected.append(item)
                        size += len(item)
                        if kind == 2:
                            state['credit'][stream] -= len(payload)
                if selected:
                    state['down'] = b''.join(selected)
                    state['queue'] = remaining
                    state['cursor'] += 1
                else:
                    return self.response(204, X_Down_Cursor=str(cursor))
            else:
                assert cursor + 1 == state['cursor']
            payload, next_cursor = state['down'], state['cursor']
            if not state['down_lost']:
                state['down_lost'] = True
                self.close_connection = True
                self.connection.shutdown(socket.SHUT_RDWR)
                self.connection.close()
                return
        self.response(200, payload, X_Down_Cursor=str(next_cursor))
    def do_DELETE(self):
        assert self.headers.get('Content-Type') is None
        assert self.headers['Authorization'] == 'Bearer ' + SESSION
        with self.server.state['cv']:
            self.server.state['deletes'] += 1
        self.response(204)

def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    certdir = pathlib.Path('app0:certs')
    certdir.mkdir()
    cert = certdir / 'cacert.pem'
    key = pathlib.Path('key.pem')
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1',
                    '-subj', '/CN=proxy.example.com', '-addext', 'subjectAltName=DNS:proxy.example.com,DNS:dns.google',
                    '-keyout', str(key), '-out', str(cert)], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for mode in ('https', 'websocket', 'denied'):
        server = Relay(('127.0.0.1', 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        server.mode = mode
        server.state = {'cv': threading.Condition(), 'busy_once': False, 'lost_once': False, 'down_lost': False, 'replays': 0, 'bytes': 0, 'deletes': 0}
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        env = dict(os.environ, WEB_PROXY_TEST_PORT=str(server.server_port))
        subprocess.run([binary, 'normal' if mode == 'https' else 'unsupported' if mode == 'websocket' else 'denied'], env=env, check=True, timeout=90)
        if mode == 'https':
            assert server.state['bytes'] == 4 * 256 * 1024 + 12 * 1024 * 1024
            assert server.state['replays'] >= 1
            assert server.state['deletes'] >= 3
        elif mode == 'websocket':
            assert server.state['deletes'] == 1
        else:
            assert server.state['deletes'] == 0
        server.shutdown()
        server.server_close()
    print('Local HTTPS relay: byte-identical retry, no duplicate data, session cleanup PASS')

if __name__ == '__main__':
    main()
