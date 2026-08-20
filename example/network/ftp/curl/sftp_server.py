import os
import socket
import sys
import errno
import paramiko
from paramiko import SFTPServerInterface, SFTPServer, SFTP_OK, SFTP_NO_SUCH_FILE, SFTP_PERMISSION_DENIED, SFTP_FAILURE, SFTPAttributes

ROOT_DIR = os.path.abspath("./sftp_root")
HOST_KEY_FILE = "sftp_host_key"

USERNAME = "sftp_user"
PASSWORD = "sftp_pass"
HOST = "127.0.0.1"
PORT = 50022


class CustomServer(paramiko.ServerInterface):
    def check_auth_password(self, username, password):
        if username == USERNAME and password == PASSWORD:
            return paramiko.AUTH_SUCCESSFUL
        return paramiko.AUTH_FAILED

    def check_channel_request(self, kind, chanid):
        if kind == "session":
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED


class StubSFTPHandle(paramiko.SFTPHandle):
    def __init__(self, flags=0):
        super().__init__(flags)
        self.readfile = None
        self.writefile = None

    def stat(self):
        try:
            return SFTPAttributes.from_stat(os.fstat(self.readfile.fileno() if self.readfile else self.writefile.fileno()))
        except OSError:
            return SFTP_FAILURE

    def chattr(self, attr):
        return SFTP_OK

    def read(self, offset, length):
        if not self.readfile:
            return SFTP_PERMISSION_DENIED
        self.readfile.seek(offset)
        return self.readfile.read(length)

    def write(self, offset, data):
        if not self.writefile:
            return SFTP_PERMISSION_DENIED
        self.writefile.seek(offset)
        self.writefile.write(data)
        return SFTP_OK

    def close(self):
        if self.readfile:
            self.readfile.close()
        if self.writefile:
            self.writefile.close()


class CustomSFTPInterface(SFTPServerInterface):
    def __init__(self, server, *args, **kwargs):
        super().__init__(server)

    def _realpath(self, path):
        # '~/' 또는 루트 경로 처리
        if path.startswith("/~/"):
            path = path[3:]
        elif path.startswith("~"):
            path = path[1:]
        path = path.lstrip("/")
        return os.path.abspath(os.path.join(ROOT_DIR, path))

    def list_folder(self, path):
        target = self._realpath(path)
        try:
            out = []
            for fname in os.listdir(target):
                fpath = os.path.join(target, fname)
                st = os.stat(fpath)
                attr = SFTPAttributes.from_stat(st)
                attr.filename = fname
                out.append(attr)
            return out
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_FAILURE

    def stat(self, path):
        target = self._realpath(path)
        try:
            return SFTPAttributes.from_stat(os.stat(target))
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_FAILURE

    def lstat(self, path):
        target = self._realpath(path)
        try:
            return SFTPAttributes.from_stat(os.lstat(target))
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_FAILURE

    def open(self, path, flags, attr):
        target = self._realpath(path)
        handle = StubSFTPHandle(flags)
        
        try:
            if (flags & os.O_CREAT) and (flags & (os.O_WRONLY | os.O_RDWR)):
                os.makedirs(os.path.dirname(target), exist_ok=True)
                handle.writefile = open(target, "wb+" if (flags & os.O_RDWR) else "wb")
            elif flags & (os.O_WRONLY | os.O_RDWR):
                handle.writefile = open(target, "r+b" if (flags & os.O_RDWR) else "wb")
            else:
                handle.readfile = open(target, "rb")
            return handle
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_PERMISSION_DENIED

    def remove(self, path):
        target = self._realpath(path)
        try:
            os.remove(target)
            return SFTP_OK
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_FAILURE

    def rename(self, oldpath, newpath):
        try:
            os.rename(self._realpath(oldpath), self._realpath(newpath))
            return SFTP_OK
        except OSError as e:
            return SFTP_NO_SUCH_FILE if e.errno == errno.ENOENT else SFTP_FAILURE

    def mkdir(self, path, attr):
        try:
            os.makedirs(self._realpath(path), exist_ok=True)
            return SFTP_OK
        except OSError:
            return SFTP_FAILURE

    def rmdir(self, path):
        try:
            os.rmdir(self._realpath(path))
            return SFTP_OK
        except OSError:
            return SFTP_FAILURE


def ensure_host_key():
    if not os.path.exists(HOST_KEY_FILE):
        print("[SFTP Server] RSA 호스트 키 생성 중...")
        key = paramiko.RSAKey.generate(2048)
        key.write_private_key_file(HOST_KEY_FILE)
    return paramiko.RSAKey(filename=HOST_KEY_FILE)


def run_sftp_server():
    os.makedirs(ROOT_DIR, exist_ok=True)
    os.makedirs(os.path.join(ROOT_DIR, "remote"), exist_ok=True)

    host_key = ensure_host_key()

    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((HOST, PORT))
    server_sock.listen(5)
    server_sock.settimeout(1.0)

    print(f"[SFTP Server] 시작됨: {HOST}:{PORT}")
    print(f"[SFTP Server] 사용자: {USERNAME} / {PASSWORD}")
    print(f"[SFTP Server] 루트 경로: {ROOT_DIR}")

    try:
        while True:
            try:
                client_sock, addr = server_sock.accept()
            except socket.timeout:
                continue

            print(f"[SFTP Server] 클라이언트 접속: {addr}")

            transport = paramiko.Transport(client_sock)
            transport.add_server_key(host_key)
            transport.set_subsystem_handler(
                "sftp",
                SFTPServer,
                sftp_si=CustomSFTPInterface
            )

            server = CustomServer()
            transport.start_server(server=server)

    except KeyboardInterrupt:
        print("\n[SFTP Server] Ctrl+C 감지: 서버를 종료합니다...")
    finally:
        server_sock.close()
        sys.exit(0)


if __name__ == "__main__":
    run_sftp_server()
