import asyncio
import logging
import os
import asyncssh

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)

# Enable detailed SFTP subsystem logging
logging.getLogger("asyncssh.sftp").setLevel(logging.DEBUG)

SFTP_USER = "testuser"
SFTP_PASS = "testpass"
ROOT_DIR = os.path.abspath("./sftp_storage")
HOST_KEY_FILE = "sftp_host_key"
PORT = 8022

class SimpleSSHServer(asyncssh.SSHServer):
    def connection_made(self, conn):
        peer = conn.get_extra_info("peername")
        logging.info(f"[SESSION] Connected from: {peer}")

    def connection_lost(self, exc):
        if exc:
            logging.error(f"[SESSION] Connection lost: {exc}")
        else:
            logging.info("[SESSION] Disconnected normally.")

    def password_auth_supported(self):
        return True

    def validate_password(self, username, password):
        logging.info(f"[AUTH] Attempting login for user: '{username}'")
        if username == SFTP_USER and password == SFTP_PASS:
            logging.info(f"[AUTH SUCCESS] User '{username}' authenticated.")
            return True
        logging.warning(f"[AUTH FAIL] Login rejected for user: '{username}'")
        return False

    def kbdint_auth_supported(self):
        return False

async def start_server():
    os.makedirs(ROOT_DIR, exist_ok=True)

    if not os.path.exists(HOST_KEY_FILE):
        key = asyncssh.generate_private_key("ssh-rsa")
        key.write_private_key(HOST_KEY_FILE)
    else:
        key = asyncssh.read_private_key(HOST_KEY_FILE)

    # Use default SFTPServer with chroot isolation
    server = await asyncssh.create_server(
        SimpleSSHServer,
        host="",
        port=PORT,
        server_host_keys=[key],
        sftp_factory=lambda ch: asyncssh.SFTPServer(ch, chroot=ROOT_DIR)
    )

    logging.info(f"SFTP server running on port {PORT}")
    logging.info(f"Root storage path: {ROOT_DIR}")
    logging.info(f"Host Key SHA256 Fingerprint: {key.get_fingerprint('sha256')}")

    await server.wait_closed()

if __name__ == "__main__":
    try:
        asyncio.run(start_server())
    except (OSError, asyncssh.Error) as exc:
        logging.critical(f"Server execution failed: {exc}")
    except KeyboardInterrupt:
        logging.info("Server stopped.")

