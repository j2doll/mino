# pip install paramiko
# ssh-keygen -t rsa -b 2048 -f server_key -N ""
import os
import socket
import sys
import threading
import paramiko

HOST = "0.0.0.0"
PORT = 2222
KEY_FILE = "server_key"

# 1. Automatic host key generation and loading
if not os.path.exists(KEY_FILE):
    print(f"[*] Host key '{KEY_FILE}' not found. Generating new 2048-bit RSA key...")
    key = paramiko.RSAKey.generate(2048)
    key.write_private_key_file(KEY_FILE)
    HOST_KEY = key
else:
    try:
        HOST_KEY = paramiko.RSAKey(filename=KEY_FILE)
    except Exception as e:
        print(f"[-] Failed to load host key: {e}")
        sys.exit(1)


# 2. Paramiko server interface definition
class BasicSSHServer(paramiko.ServerInterface):
    def __init__(self):
        self.event = threading.Event()

    def check_channel_request(self, kind, chanid):
        if kind == "session":
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED

    # Supports both default credentials and C++ client credentials
    def check_auth_password(self, username, password):
        if (username == "admin" and password == "secret123") or \
           (username == "test_user" and password == "test_password"):
            return paramiko.AUTH_SUCCESSFUL
        return paramiko.AUTH_FAILED

    def check_channel_pty_request(self, channel, term, width, height, pixelwidth, pixelheight, modes):
        return True

    def check_channel_shell_request(self, channel):
        self.event.set()
        return True


# 3. Individual client connection handler
def handle_client(client_socket):
    transport = None
    try:
        transport = paramiko.Transport(client_socket)
        transport.add_server_key(HOST_KEY)

        server = BasicSSHServer()
        event = threading.Event()

        # Initiate SSH negotiation and wait for handshake completion
        transport.start_server(event=event, server=server)
        event.wait(10)

        if not event.is_set():
            print("[-] SSH handshake timed out.")
            return

        # Wait for session channel request
        chan = transport.accept(20)
        if chan is None:
            print("[-] Channel request failed or timed out.")
            return

        print("[+] Client authenticated and session channel opened.")

        # Wait for shell execution request
        server.event.wait(10)

        buffer = ""
        while transport.is_active():
            data = chan.recv(1024).decode("utf-8", errors="ignore")
            if not data:
                break

            # Disconnect on control characters (Ctrl+C: \x03, Ctrl+D: \x04)
            if any(char in data for char in ["\x03", "\x04"]):
                chan.send("\r\nSession terminated by user.\r\n")
                break

            buffer += data

            # Process newline-delimited JSON (NDJSON) packets
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if line:
                    print(f"[Server Recv] {line}")
                    # Echo response back to client with newline terminator
                    ack_json = f'{{"status":"ack","received":{line}}}\n'
                    chan.send(ack_json)

    except Exception as ex:
        print(f"[-] Client session exception: {ex}")
    finally:
        if transport:
            transport.close()
        client_socket.close()
        print("[*] Client connection closed.")


# 4. Main server listener loop
def start_server(host=HOST, port=PORT):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((host, port))
    server_socket.listen(5)
    # 1.0s timeout ensures Windows properly catches KeyboardInterrupt (Ctrl+C)
    server_socket.settimeout(1.0)

    print(f"[*] SSH2 Server listening on {host}:{port}")
    print("[*] Press Ctrl+C in this terminal to stop the server.")

    try:
        while True:
            try:
                client, addr = server_socket.accept()
                print(f"[+] Connection accepted from {addr[0]}:{addr[1]}")
                client_thread = threading.Thread(target=handle_client, args=(client,))
                client_thread.daemon = True
                client_thread.start()
            except socket.timeout:
                continue
            except Exception as ex:
                print(f"[-] Accept error: {ex}")
    except KeyboardInterrupt:
        print("\n[*] Server shutdown signal received (Ctrl+C). Exiting cleanly...")
    finally:
        server_socket.close()


if __name__ == "__main__":
    start_server()
    