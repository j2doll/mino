
import socket
import time
import threading
import sys
from typing import List, Optional, Tuple

# ============================================================================
# 1. Protocol & Publisher Class Definition Layer
# ============================================================================

class MsgType:
    SUBSCRIBE = 1
    PUBLISH = 2

class MessageBrokerPublisher:
    def __init__(self, broker_ip: str, broker_port: int):
        self.broker_ip: str = broker_ip
        self.broker_port: int = broker_port
        
        self._socket: Optional[socket.socket] = None
        self._is_connected: bool = False
        self._reconnect_lock = threading.Lock()
        self._is_reconnecting: bool = False

    def _make_packet(self, msg_type: int, topic: str, msg_kind: str, body: str = "") -> bytes:
        """C++ pubsub_protocol.hpp의 make_packet 구조와 100% 일치하는 ASCII 패킷을 생성합니다."""
        timestamp_ms = int(time.time() * 1000)
        
        packet = f"TYPE:{msg_type}\n"
        packet += f"TIMESTAMP:{timestamp_ms}\n"
        packet += f"TOPIC:{topic}\n"
        packet += f"KIND:{msg_kind}\n"
        packet += f"BODY_LEN:{len(body.encode('utf-8'))}\n"
        packet += "\n"  # 헤더의 끝을 알리는 빈 줄(Delimiter)
        
        if body:
            packet += body
            
        return packet.encode('utf-8')

    def _get_platform_reconnect_delay(self) -> int:
        """운영체제별 TIME_WAIT 자원 회수 시간을 고려한 대기 시간을 초 단위로 반환합니다."""
        if sys.platform.startswith('win'):
            print("[DEBUG] Platform detected: Windows. Setting TIME_WAIT recovery delay to 120 seconds (2 minutes).")
            return 120
        else:
            print("[DEBUG] Platform detected: Linux/Unix. Setting TIME_WAIT recovery delay to 60 seconds (1 minute).")
            return 60

    def connect(self) -> bool:
        """브로커에 소켓 연결을 수립합니다 (C++ publisher::connect 이식)."""
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            print(f"[DEBUG] Socket created. Attempting to connect to broker {self.broker_ip}:{self.broker_port}...")
            self._socket.connect((self.broker_ip, self.broker_port))
            
            self._is_connected = True
            print(f"[Python Publisher] Connected to broker successfully.")
            return True
        except Exception as e:
            print(f"[Python Publisher] Connection failed: {e}")
            self._is_connected = False
            if self._socket:
                self._socket.close()
            return False

    def publish(self, topic: str, msg_kind: str, message: str) -> Tuple[bool, str]:
        """C++ publisher::publish 구조와 일치하는 데이터 발행 함수입니다."""
        if not self._is_connected:
            # 연결이 비정상적으로 끊긴 상태라면 백그라운드 재연결 스레드 가동
            self._handle_disconnection()
            return False, "disconnection_error"

        if not topic:
            return False, "invalid_topic"

        try:
            # ASCII 패킷 조립 및 바이트 스트림 전송
            packet = self._make_packet(MsgType.PUBLISH, topic, msg_kind, message)
            self._socket.sendall(packet)
            
            print(f"[Python Publisher] Packet sent for topic: {topic}, kind: {msg_kind}, length: {len(message)}")
            return True, "success"
            
        except Exception as e:
            print(f"[Python Publisher] Failed to send packet for topic {topic}: {e}")
            self._handle_disconnection()
            return False, "socket_error"

    def _handle_disconnection(self) -> None:
        """송신 실패 또는 단절 감지 시 백그라운드에서 재연결 루프를 실행합니다."""
        with self._reconnect_lock:
            if self._is_reconnecting:
                return
            self._is_reconnecting = True
            self._is_connected = False

        if self._socket:
            try:
                self._socket.close()
            except:
                pass

        # 메인 로직 스레드가 1~2분 동안 멈추는 것을 방지하기 위해 별도 스레드로 분리
        reconnect_thread = threading.Thread(target=self._reconnect_loop, daemon=True)
        reconnect_thread.start()

    def _reconnect_loop(self) -> None:
        """TIME_WAIT 소켓 자원이 해제될 때까지 대기한 후 재연결을 수행합니다."""
        delay = self._get_platform_reconnect_delay()
        print(f"[Python Publisher] Broker link dead. Entering safety sleep for {delay}s to clear TIME_WAIT...")
        time.sleep(delay)

        attempt = 0
        while not self._is_connected:
            attempt += 1
            print(f"[Python Publisher] Reconnection attempt #{attempt}...")
            
            if self.connect():
                print("[Python Publisher] Reconnection achieved successfully. Resuming operations.")
                with self._reconnect_lock:
                    self._is_reconnecting = False
                break
            else:
                print(f"[Python Publisher] Reconnection attempt #{attempt} failed. Next retry in 30 seconds...")
                time.sleep(30)

    def disconnect(self) -> None:
        """명시적으로 브로커와의 연결을 닫습니다."""
        print("[Python Publisher] Disconnecting from broker...")
        self._is_connected = False
        with self._reconnect_lock:
            self._is_reconnecting = False
            
        if self._socket:
            try:
                self._socket.close()
                print("[DEBUG] Publisher socket closed cleanly.")
            except Exception as e:
                print(f"[DEBUG] Exception during socket close: {e}")

    def is_connected(self) -> bool:
        return self._is_connected


# ============================================================================
# 2. Main Execution Script Layer (User Application Space)
# ============================================================================

if __name__ == "__main__":
    # 브로커 IP 및 포트 지정 (C++ 환경 및 파이썬 서브스크라이버 환경과 일치)
    BROKER_IP = "127.0.0.1"
    BROKER_PORT = 54321
    
    # C++ main.cpp 예제에서 서브스크라이버가 대기 중인 토픽 명칭 지정
    TARGET_TOPIC = "sports"

    # Publisher 생성 및 최초 연결
    publisher = MessageBrokerPublisher(BROKER_IP, BROKER_PORT)
    
    if publisher.connect():
        print("[Main] Publisher started successfully. Sending sample data streams every 3 seconds...")
        
        message_counter = 0
        try:
            while True:
                message_counter += 1
                payload_body = f"Hello from Python Publisher! Sequence ID: #{message_counter}"
                
                # 'sports' 토픽에 'text' 종류의 메시지 발행 시도
                success, reason = publisher.publish(
                    topic=TARGET_TOPIC, 
                    msg_kind="text", 
                    message=payload_body
                )
                
                if not success:
                    print(f"[Main] Publish failed. Reason: {reason}. (Will check reconnection state)")
                
                # 3초마다 반복 송신
                time.sleep(3)
                
        except KeyboardInterrupt:
            print("\n[Main] Interrupted by user switch signal.")
            
        publisher.disconnect()
        print("[Main] Program exited cleanly.")
    else:
        print("[Main] Failed to start publisher initial setup.")

