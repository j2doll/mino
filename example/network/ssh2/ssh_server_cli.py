import argparse
import logging
import os
import socket
import subprocess
import sys
import threading
import paramiko # pip install paramiko

# Logging configuration (Paramiko internal logs directed to terminal)
logging.basicConfig(
    level=logging.WARNING,
    format="[%(levelname)s] %(message)s",
    stream=sys.stdout
)
paramiko_logger = logging.getLogger("paramiko.transport")
paramiko_logger.setLevel(logging.WARNING)

# Auto-generate or load host key
HOST_KEY_FILE = "portable_host_key"
if not os.path.exists(HOST_KEY_FILE):
    key = paramiko.RSAKey.generate(2048)
    key.write_private_key_file(HOST_KEY_FILE)
HOST_KEY = paramiko.RSAKey(filename=HOST_KEY_FILE)

def log(text):
    print(text, flush=True)

class SSHServerHandler(paramiko.ServerInterface):
    def __init__(self, valid_user, valid_pwd):
        self.valid_user = valid_user
        self.valid_pwd = valid_pwd
        self.event = threading.Event()

    def check_channel_request(self, kind, chanid):
        if kind == 'session':
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED

    def check_auth_password(self, username, password):
        if username == self.valid_user and password == self.valid_pwd:
            log(f"[AUTH SUCCESS] User: {username}")
            return paramiko.AUTH_SUCCESSFUL
        log(f"[AUTH FAILED] Invalid login attempt: {username}")
        return paramiko.AUTH_FAILED

    def check_channel_shell_request(self, channel):
        self.event.set()
        return True

    def check_channel_pty_request(self, channel, term, width, height, pixelwidth, pixelheight, modes):
        return True

def handle_client(client, user, pwd, addr):
    transport = paramiko.Transport(client)
    transport.add_server_key(HOST_KEY)
    handler = SSHServerHandler(user, pwd)
    
    try:
        transport.start_server(server=handler)
        chan = transport.accept(20)
        if chan is None:
            return
        handler.event.wait(10)

        # Spawn Windows cmd.exe shell
        proc = subprocess.Popen(
            ['cmd.exe'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=False
        )

        def pipe_to_proc():
            try:
                while True:
                    data = chan.recv(1024)
                    if not data:
                        break
                    proc.stdin.write(data)
                    proc.stdin.flush()
            except socket.error as e:
                log(f"[SOCKET ERROR] Inbound pipe error: {e}")
            except Exception as e:
                log(f"[ERROR] Inbound pipe error: {e}")
            finally:
                proc.terminate()

        def pipe_from_proc():
            try:
                while True:
                    data = proc.stdout.read(1)
                    if not data:
                        break
                    chan.send(data)
            except socket.error as e:
                log(f"[SOCKET ERROR] Outbound pipe error: {e}")
            except Exception as e:
                log(f"[ERROR] Outbound pipe error: {e}")

        t1 = threading.Thread(target=pipe_to_proc, daemon=True)
        t2 = threading.Thread(target=pipe_from_proc, daemon=True)
        t1.start()
        t2.start()
        t1.join()
    except socket.error as e:
        log(f"[SOCKET ERROR] Client {addr[0]}:{addr[1]} - {e}")
    except Exception as e:
        log(f"[ERROR] Client {addr[0]}:{addr[1]} - {e}")
    finally:
        transport.close()
        log(f"[CONN] Connection closed: {addr[0]}:{addr[1]}")

def start_server(port, user, pwd):
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_sock.bind(('0.0.0.0', port))
        server_sock.listen(5)
    except Exception as e:
        log(f"[ERROR] Failed to bind port {port}: {e}")
        sys.exit(1)

    log(f"[INFO] SSH server running on 0.0.0.0:{port}")
    log(f"[INFO] Configured account: {user} / {pwd}")
    log("[INFO] Press Ctrl+C to stop the server.")

    try:
        while True:
            client, addr = server_sock.accept()
            log(f"[CONN] Client connected from {addr[0]}:{addr[1]}")
            threading.Thread(target=handle_client, args=(client, user, pwd, addr), daemon=True).start()
    except KeyboardInterrupt:
        log("\n[INFO] KeyboardInterrupt received. Stopping SSH server...")
    finally:
        server_sock.close()
        log("[INFO] SSH server stopped.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Portable SSH Server (CLI Version)")
    parser.add_argument("--port", type=int, default=2222, help="Port to listen on (default: 2222)")
    parser.add_argument("--user", type=str, default="admin", help="Virtual username (default: admin)")
    parser.add_argument("--pwd", type=str, default="secret123", help="Virtual password (default: secret123)")
    
    args = parser.parse_args()
    start_server(args.port, args.user, args.pwd)
