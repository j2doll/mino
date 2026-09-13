import logging
import os
import socket
import subprocess
import threading
import tkinter as tk
from tkinter import messagebox, scrolledtext
import paramiko # pip install paramiko

# Custom logging handler to redirect Paramiko internal logs to Tkinter GUI
class TkinterLogHandler(logging.Handler):
    def __init__(self, log_fn):
        super().__init__()
        self.log_fn = log_fn

    def emit(self, record):
        try:
            msg = self.format(record)
            self.log_fn(f"[{record.levelname}] {msg}")
        except Exception:
            pass

# Auto-generate or load host key
HOST_KEY_FILE = "portable_host_key"
if not os.path.exists(HOST_KEY_FILE):
    key = paramiko.RSAKey.generate(2048)
    key.write_private_key_file(HOST_KEY_FILE)
HOST_KEY = paramiko.RSAKey(filename=HOST_KEY_FILE)

class SSHServerHandler(paramiko.ServerInterface):
    def __init__(self, valid_user, valid_pwd, log_fn):
        self.valid_user = valid_user
        self.valid_pwd = valid_pwd
        self.log = log_fn
        self.event = threading.Event()

    def check_channel_request(self, kind, chanid):
        if kind == 'session':
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED

    def check_auth_password(self, username, password):
        if username == self.valid_user and password == self.valid_pwd:
            self.log(f"[AUTH SUCCESS] User: {username}")
            return paramiko.AUTH_SUCCESSFUL
        self.log(f"[AUTH FAILED] Invalid login attempt: {username}")
        return paramiko.AUTH_FAILED

    def check_channel_shell_request(self, channel):
        self.event.set()
        return True

    def check_channel_pty_request(self, channel, term, width, height, pixelwidth, pixelheight, modes):
        return True

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Portable SSH Server (Python)")
        self.root.geometry("520x460")
        self.is_running = False

        # GUI Input Fields
        tk.Label(root, text="Port:").pack(anchor="w", padx=10, pady=(10, 0))
        self.ent_port = tk.Entry(root)
        self.ent_port.insert(0, "2222")
        self.ent_port.pack(fill="x", padx=10)

        tk.Label(root, text="Username:").pack(anchor="w", padx=10, pady=(5, 0))
        self.ent_user = tk.Entry(root)
        self.ent_user.insert(0, "admin")
        self.ent_user.pack(fill="x", padx=10)

        tk.Label(root, text="Password:").pack(anchor="w", padx=10, pady=(5, 0))
        self.ent_pwd = tk.Entry(root, show="*")
        self.ent_pwd.insert(0, "secret123")
        self.ent_pwd.pack(fill="x", padx=10)

        # Start/Stop Button
        self.btn_toggle = tk.Button(root, text="Start Server", bg="#4CAF50", fg="white", font=("bold", 10), command=self.toggle_server)
        self.btn_toggle.pack(fill="x", padx=10, pady=10)

        # Server Log Area
        tk.Label(root, text="Server Logs:").pack(anchor="w", padx=10)
        self.log_area = scrolledtext.ScrolledText(root, height=12, state="disabled")
        self.log_area.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        # Attach custom logger to capture Paramiko internal socket errors
        paramiko_logger = logging.getLogger("paramiko.transport")
        paramiko_logger.setLevel(logging.WARNING)
        paramiko_logger.addHandler(TkinterLogHandler(self.log))

    def log(self, text):
        def _update():
            self.log_area.config(state="normal")
            self.log_area.insert(tk.END, text + "\n")
            self.log_area.see(tk.END)
            self.log_area.config(state="disabled")
        # Ensure thread-safe GUI updates
        self.root.after(0, _update)

    def toggle_server(self):
        if not self.is_running:
            self.start_server()
        else:
            self.stop_server()

    def start_server(self):
        port = int(self.ent_port.get())
        user = self.ent_user.get()
        pwd = self.ent_pwd.get()

        try:
            self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_sock.bind(('0.0.0.0', port))
            self.server_sock.listen(5)
        except Exception as e:
            messagebox.showerror("Error", f"Failed to bind port: {e}")
            return

        self.is_running = True
        self.btn_toggle.config(text="Stop Server", bg="#f44336")
        self.log(f"[INFO] SSH server started on port {port}")

        threading.Thread(target=self.accept_loop, args=(user, pwd), daemon=True).start()

    def stop_server(self):
        self.is_running = False
        if hasattr(self, 'server_sock'):
            self.server_sock.close()
        self.btn_toggle.config(text="Start Server", bg="#4CAF50")
        self.log("[INFO] SSH server stopped.")

    def accept_loop(self, user, pwd):
        while self.is_running:
            try:
                client, addr = self.server_sock.accept()
            except:
                break
            self.log(f"[CONN] Client connected from {addr[0]}:{addr[1]}")
            threading.Thread(target=self.handle_client, args=(client, user, pwd, addr), daemon=True).start()

    def handle_client(self, client, user, pwd, addr):
        transport = paramiko.Transport(client)
        transport.add_server_key(HOST_KEY)
        handler = SSHServerHandler(user, pwd, self.log)
        
        try:
            transport.start_server(server=handler)
            chan = transport.accept(20)
            if chan is None:
                return
            handler.event.wait(10)

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
                    self.log(f"[SOCKET ERROR] Inbound pipe error: {e}")
                except Exception as e:
                    self.log(f"[ERROR] Inbound pipe error: {e}")
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
                    self.log(f"[SOCKET ERROR] Outbound pipe error: {e}")
                except Exception as e:
                    self.log(f"[ERROR] Outbound pipe error: {e}")

            t1 = threading.Thread(target=pipe_to_proc, daemon=True)
            t2 = threading.Thread(target=pipe_from_proc, daemon=True)
            t1.start()
            t2.start()
            t1.join()
        except socket.error as e:
            self.log(f"[SOCKET ERROR] Client {addr[0]}:{addr[1]} - {e}")
        except Exception as e:
            self.log(f"[ERROR] Client {addr[0]}:{addr[1]} - {e}")
        finally:
            transport.close()
            self.log(f"[CONN] Connection closed: {addr[0]}:{addr[1]}")

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
