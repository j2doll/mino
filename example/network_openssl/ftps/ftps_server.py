import os
import sys
import time
import threading
import logging
import datetime
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import TLS_FTPHandler
from pyftpdlib.servers import FTPServer

# ==============================================================================
# 1. 자체 서명 TLS 인증서 자동 생성 (없을 경우 1회 생성)
# ==============================================================================
def ensure_ssl_certificate(cert_file="server.pem", key_file="server.key"):
    """인증서가 없으면 2048-bit RSA 자체 서명 인증서를 자동으로 생성합니다."""
    if os.path.exists(cert_file) and os.path.exists(key_file):
        return cert_file, key_file

    print("[*] Generating self-signed TLS certificate...")
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.hazmat.primitives import serialization

    key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )

    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "KR"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Mino FTPS Server"),
        x509.NameAttribute(NameOID.COMMON_NAME, "127.0.0.1"),
    ])

    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.now(datetime.timezone.utc))
        .not_valid_after(datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=365))
        .sign(key, hashes.SHA256())
    )

    with open(key_file, "wb") as f:
        f.write(key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))

    with open(cert_file, "wb") as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))

    print(f"[+] Certificate created: {cert_file}, {key_file}")
    return cert_file, key_file


# ==============================================================================
# 2. 메인 FTPS 서버 구동 함수
# ==============================================================================
def run_ftps_server(host="0.0.0.0", port=990, user="test", password="test"):
    ftp_root = os.path.abspath("./ftp_root")
    os.makedirs(ftp_root, exist_ok=True)

    # 1) 인증서 준비
    cert_file, key_file = ensure_ssl_certificate()

    # 2) 계정 및 모든 파일 작업 권한 등록
    authorizer = DummyAuthorizer()
    authorizer.add_user(
        username=user,
        password=password,
        homedir=ftp_root,
        perm="elradfmwMT"
    )

    # 3) TLS FTP 핸들러 설정
    handler = TLS_FTPHandler
    handler.authorizer = authorizer
    handler.certfile = cert_file
    handler.keyfile = key_file

    # RFC 4217 규격 적용 (제어 및 데이터 채널 TLS 보호)
    handler.tls_control_required = True
    handler.tls_data_required = True

    # 패시브 모드 설정
    handler.passive_ports = range(60000, 60050)
    handler.masquerade_address = "127.0.0.1"

    # 로깅 포맷 설정
    logging.basicConfig(
        level=logging.INFO,
        format="<%(asctime)s> [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S"
    )

    # 4) 서버 인스턴스 생성
    server = FTPServer((host, port), handler)
    server.max_cons = 256
    server.max_cons_per_ip = 10

    # 5) 백그라운드 데몬 스레드로 서버 루프 실행
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    print("=" * 60)
    print(f"[*] FTPS Server running on {host}:{port}")
    print(f"[*] Account   : {user} / {password}")
    print(f"[*] Root Dir  : {ftp_root}")
    print(f"[*] Press Ctrl + C to stop the server.")
    print("=" * 60)

    # 6) 메인 스레드는 time.sleep()으로 대기 -> Ctrl+C를 즉각 수신
    try:
        while server_thread.is_alive():
            time.sleep(0.2)
    except (KeyboardInterrupt, SystemExit):
        print("\n\n[*] Shutdown signal received (Ctrl+C). Stopping server...")
    finally:
        server.close_all()
        print("[+] FTPS Server stopped cleanly.")
        os._exit(0)  # lingering socket/thread 강제 잔여 방지 및 즉각 종료


if __name__ == "__main__":
    default_port = 990
    if len(sys.argv) >= 2:
        try:
            default_port = int(sys.argv[1])
        except ValueError:
            print(f"[-] Invalid port number: {sys.argv[1]}. Using default port 990.")

    run_ftps_server(port=default_port)

