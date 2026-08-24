import socket
import time
import threading
import sys
from typing import Callable, List, Optional

# ============================================================================
# 1. Protocol & Subscriber Class Definition Layer
# ============================================================================

class MsgType:
    SUBSCRIBE = 1
    PUBLISH = 2

class AsciiHeader:
    def __init__(self):
        self.msg_type: int = 0
        self.timestamp: int = 0
        self.topic: str = ""
        self.msg_kind: str = ""
        self.body_len: int = 0
        self.header_full_size: int = 0

class MessageBrokerSubscriber:
    def __init__(self, broker_ip: str, broker_port: int):
        self.broker_ip: str = broker_ip
        self.broker_port: int = broker_port
        self.sub_topics: List[str] = []
        self.app_callback: Optional[Callable[[str, str, str, int], None]] = None
        
        self._socket: Optional[socket.socket] = None
        self._stream_buffer: bytes = b""
        self._is_running: bool = False
        self._receive_thread: Optional[threading.Thread] = None
        self._reconnect_thread: Optional[threading.Thread] = None

    def set_topics(self, topics: List[str]) -> None:
        self.sub_topics = topics
        print(f"[DEBUG] Target topics configured: {self.sub_topics}")

    def set_on_message_handler(self, callback: Callable[[str, str, str, int], None]) -> None:
        self.app_callback = callback

    def _make_packet(self, msg_type: int, topic: str, msg_kind: str, body: str = "") -> bytes:
        timestamp_ms = int(time.time() * 1000)
        
        packet = f"TYPE:{msg_type}\n"
        packet += f"TIMESTAMP:{timestamp_ms}\n"
        packet += f"TOPIC:{topic}\n"
        packet += f"KIND:{msg_kind}\n"
        packet += f"BODY_LEN:{len(body.encode('utf-8'))}\n"
        packet += "\n"  
        
        if body:
            packet += body
            
        return packet.encode('utf-8')

    def _parse_ascii_header(self) -> Optional[AsciiHeader]:
        header_end = self._stream_buffer.find(b"\n\n")
        if header_end == -1:
            return None

        header_header_full_size = header_end + 2
        header_bytes = self._stream_buffer[:header_end]
        header_text = header_bytes.decode('utf-8', errors='ignore')

        header = AsciiHeader()
        header.header_full_size = header_header_full_size

        lines = header_text.split('\n')
        for line in lines:
            if ':' not in line:
                continue
            key, val = line.split(':', 1)
            
            if key == "TYPE":
                header.msg_type = int(val)
            elif key == "TIMESTAMP":
                header.timestamp = int(val)
            elif key == "TOPIC":
                header.topic = val
            elif key == "KIND":
                header.msg_kind = val
            elif key == "BODY_LEN":
                header.body_len = int(val)

        return header

    def _get_platform_reconnect_delay(self) -> int:
        """운영체제별 TIME_WAIT 자원 회수 시간을 고려한 대기 시간을 초 단위로 반환합니다."""
        if sys.platform.startswith('win'):
            print("[DEBUG] Platform detected: Windows. Setting TIME_WAIT recovery delay to 120 seconds (2 minutes).")
            return 120
        else:
            print("[DEBUG] Platform detected: Linux/Unix. Setting TIME_WAIT recovery delay to 60 seconds (1 minute).")
            return 60

    def connect(self) -> bool:
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            print(f"[DEBUG] Socket created. Attempting to connect to {self.broker_ip}:{self.broker_port}...")
            self._socket.connect((self.broker_ip, self.broker_port))
            print(f"[Python Subscriber] Connected to broker {self.broker_ip}:{self.broker_port}")

            for topic in self.sub_topics:
                sub_packet = self._make_packet(MsgType.SUBSCRIBE, topic, "")
                self._socket.sendall(sub_packet)
                print(f"[Python Subscriber] Sent subscription packet for topic: {topic}")

            self._is_running = True
            self._receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self._receive_thread.start()
            return True

        except Exception as e:
            print(f"[Python Subscriber] Connection failed: {e}")
            if self._socket:
                self._socket.close()
            return False

    def _receive_loop(self) -> None:
        while self._is_running:
            try:
                data = self._socket.recv(4096)
                if not data:
                    print("[Python Subscriber] Connection closed by broker cleanly (EOF received).")
                    break

                self._stream_buffer += data

                while True:
                    header = self._parse_ascii_header()
                    if not header:
                        break

                    total_packet_size = header.header_full_size + header.body_len
                    if len(self._stream_buffer) < total_packet_size:
                        break  

                    body_bytes = self._stream_buffer[header.header_full_size : total_packet_size]
                    body = body_bytes.decode('utf-8', errors='ignore')

                    if self.app_callback:
                        self.app_callback(header.topic, header.msg_kind, body, header.timestamp)

                    self._stream_buffer = self._stream_buffer[total_packet_size:]

            except Exception as e:
                if self._is_running:
                    print(f"[Python Subscriber] Connection error occurred: {e}")
                break

        # 루프를 빠져나왔다는 것은 연결이 끊어졌음을 의미합니다.
        # 사용자에 의해 명시적으로 중단된 게 아니라면 재연결을 가동합니다.
        if self._is_running:
            self._handle_unexpected_disconnection()

    def _handle_unexpected_disconnection(self) -> None:
        """예기치 못한 단절 발생 시 백그라운드 스레드를 통해 재연결을 시도합니다."""
        if self._reconnect_thread and self._reconnect_thread.is_alive():
            return
            
        self._is_running = False
        if self._socket:
            try:
                self._socket.close()
            except:
                pass
                
        self._reconnect_thread = threading.Thread(target=self._reconnect_loop, daemon=True)
        self._reconnect_thread.start()

    def _reconnect_loop(self) -> None:
        """TIME_WAIT 시간을 대기한 후 연결이 성공할 때까지 무한 재시도하는 루프입니다."""
        delay = self._get_platform_reconnect_delay()
        
        print(f"[Python Subscriber] Entering platform safety sleep for {delay}s before reconnecting...")
        time.sleep(delay)
        
        attempt = 0
        while not self._is_running:
            attempt += 1
            print(f"[Python Subscriber] Reconnection attempt #{attempt}...")
            
            if self.connect():
                print("[Python Subscriber] Reconnection achieved successfully. Resuming operations.")
                break
            else:
                # 자주 재시도할 필요가 없으므로 첫 실패 이후에는 30초씩 여유를 두고 재시도합니다.
                print(f"[Python Subscriber] Reconnection attempt #{attempt} failed. Retrying in 30 seconds...")
                time.sleep(30)

    def disconnect(self) -> None:
        print("[Python Subscriber] Disconnecting from broker...")
        self._is_running = False
        if self._socket:
            try:
                self._socket.close()
                print("[DEBUG] Socket closed cleanly.")
            except Exception as e:
                print(f"[DEBUG] Exception during socket close: {e}")
        if self._receive_thread:
            self._receive_thread.join(timeout=2.0)
        self._stream_buffer = b""

    def is_connected(self) -> bool:
        return self._is_running or (self._reconnect_thread is not None and self._reconnect_thread.is_alive())


# ============================================================================
# 2. Main Execution Script Layer (User Application Space)
# ============================================================================

def on_message_received(topic: str, msg_kind: str, body: str, timestamp: int):
    """Callback function triggers when a new message is successfully parsed."""
    time_struct = time.localtime(timestamp / 1000.0)
    time_string = time.strftime("%Y-%m-%d %H:%M:%S", time_struct)
    
    print("\n" + "★"*20)
    print(f"[KST] {time_string} | Topic: <{topic}> | Kind: [{msg_kind}] -> Body: {body}")
    print("★"*20 + "\n")


if __name__ == "__main__":
    # 연결할 broker의 IP와 포트 설정
    BROKER_IP = "127.0.0.1"
    BROKER_PORT = 54321
    # 구독 대상인 토픽. 복수 개 설정 가능.
    TARGET_TOPICS = ["sports"] 

    subscriber = MessageBrokerSubscriber(BROKER_IP, BROKER_PORT)
    subscriber.set_topics(TARGET_TOPICS)
    subscriber.set_on_message_handler(on_message_received)

    if subscriber.connect():
        print("[Main] Python Subscriber started successfully. Listening for messages...")
        
        try:
            # 메인 스레드는 계속 살아있으면서 수신 상태 및 재연결 상태를 유지합니다.
            while subscriber.is_connected():
                time.sleep(1)
        except KeyboardInterrupt:
            print("\n[Main] Interrupted by user switch signal.")
            
        subscriber.disconnect()
        print("[Main] Program exited cleanly.")
    else:
        print("[Main] Failed to start subscriber.")
        
