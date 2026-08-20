import os
import signal
import sys

# pip install pyftpdlib
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer

def run_ftp_server():
    # FTP 루트 디렉터리 생성
    ftp_root = os.path.abspath("./ftp_root")
    remote_dir = os.path.join(ftp_root, "remote")
    os.makedirs(remote_dir, exist_ok=True)

    # 사용자 계정 및 권한 설정 (elradfmw: 모든 권한)
    authorizer = DummyAuthorizer()
    authorizer.add_user("test_user", "test_password", ftp_root, perm="elradfmw")

    # 핸들러 설정
    handler = FTPHandler
    handler.authorizer = authorizer
    handler.banner = "pyftpdlib based FTP server ready."

    # 50021 포트 바인딩
    server_address = ("127.0.0.1", 50021)
    server = FTPServer(server_address, handler)

    # Ctrl + C 핸들러
    def signal_handler(sig, frame):
        print("\n[FTP Server] Ctrl+C 감지: 서버를 종료합니다...")
        server.close_all()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    print(f"[FTP Server] 시작됨: {server_address[0]}:{server_address[1]}")
    print(f"[FTP Server] 사용자: test_user / test_password")
    print(f"[FTP Server] 루트 경로: {ftp_root}")

    try:
        server.serve_forever()
    except (KeyboardInterrupt, SystemExit):
        pass
    finally:
        server.close_all()

if __name__ == "__main__":
    run_ftp_server()
