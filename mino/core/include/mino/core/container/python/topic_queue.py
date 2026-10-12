import enum
import threading
import time
from datetime import datetime
from typing import Any, Callable, Dict, List, Optional


class MessageContext:
    def __init__(self, data: Any):
        self.data: Any = data
        self.published_at: datetime = datetime.now()


class QueueState(enum.Enum):
    STOPPED = 1
    STOPPING = 2
    RUNNING = 3


class SubscriberChannel:
    def __init__(self, callback: Callable[[MessageContext], None]):
        self.callback: Callable[[MessageContext], None] = callback
        self.item_queue: List[MessageContext] = []
        self.lock = threading.Lock()
        self.cv = threading.Condition(self.lock)
        self.worker_thread: Optional[threading.Thread] = None


class TopicQueue:
    """C++ topic_queue와 완전히 동일한 싱글톤 기반 Pub-Sub 메시지 큐"""
    _instance = None
    _singleton_lock = threading.Lock()

    @classmethod
    def get_instance(cls) -> 'TopicQueue':
        with cls._singleton_lock:
            if cls._instance is None:
                cls._instance = cls()
            return cls._instance

    def __init__(self):
        self._subscribers: Dict[str, List[SubscriberChannel]] = {}
        self._state_lock = threading.RLock()
        self._is_running: bool = False
        self._is_cleanup_finished: bool = True

    def get_state(self) -> QueueState:
        with self._state_lock:
            if self._is_running:
                return QueueState.RUNNING
            if not self._is_cleanup_finished:
                return QueueState.STOPPING
            return QueueState.STOPPED

    def subscribe(self, topic: str, callback: Callable[[MessageContext], None]) -> bool:
        with self._state_lock:
            if self._is_running or not self._is_cleanup_finished:
                print("[WARN] Subscription failed: Queue must be in STOPPED state to add a subscriber.")
                return False

            if not callback:
                return False

            if topic not in self._subscribers:
                self._subscribers[topic] = []

            self._subscribers[topic].append(SubscriberChannel(callback))
            return True

    def start(self):
        with self._state_lock:
            if self._is_running:
                return

            self._is_running = True
            self._is_cleanup_finished = False

            thread_count = 0
            for topic, channel_list in self._subscribers.items():
                for channel in channel_list:
                    channel.worker_thread = threading.Thread(
                        target=self._worker_loop, args=(channel,), daemon=True
                    )
                    channel.worker_thread.start()
                    thread_count += 1

            print(f">>> Topic Queue System Started (Total worker threads: {thread_count}) <<<")

    def stop(self):
        with self._state_lock:
            if not self._is_running:
                return
            self._is_running = False

        # 모든 채널에 알림 전송
        for topic, channel_list in self._subscribers.items():
            for channel in channel_list:
                with channel.cv:
                    channel.cv.notify_all()

        # 모든 워커 스레드 종료 대기
        for topic, channel_list in self._subscribers.items():
            for channel in channel_list:
                if channel.worker_thread and channel.worker_thread.is_alive():
                    channel.worker_thread.join()
                    channel.worker_thread = None

        with self._state_lock:
            self._is_cleanup_finished = True

        print(">>> Topic Queue System Stopped Successfully <<<")

    def publish(self, topic: str, data: Any) -> bool:
        with self._state_lock:
            if not self._is_running:
                return False

            if topic not in self._subscribers or not self._subscribers[topic]:
                return True

            channel_list = self._subscribers[topic]

        ctx = MessageContext(data)
        for channel in channel_list:
            with channel.cv:
                channel.item_queue.append(ctx)
                channel.cv.notify()

        return True

    def _worker_loop(self, channel: SubscriberChannel):
        while True:
            ctx = None
            has_data = False

            with channel.cv:
                while self._is_running and not channel.item_queue:
                    channel.cv.wait()

                if not self._is_running and not channel.item_queue:
                    break

                if channel.item_queue:
                    ctx = channel.item_queue.pop(0)
                    has_data = True

            if has_data and ctx:
                try:
                    channel.callback(ctx)
                except Exception as e:
                    print(f"[ERROR] Exception in subscriber callback: {e}")

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        with self._state_lock:
            state_str = self.get_state().name
            if title:
                print(f"=== {title} (State: {state_str}, Topics: {len(self._subscribers)}) ===")
            else:
                print(f"=== Topic Queue Dump (State: {state_str}, Topics: {len(self._subscribers)}) ===")

            if not self._subscribers:
                print("  \\-- <No Subscribed Topics>\n")
                return

            topic_items = list(self._subscribers.items())
            for idx, (topic, channel_list) in enumerate(topic_items):
                is_last_topic = (idx == len(topic_items) - 1)
                t_connector = "\\-- " if is_last_topic else "|-- "
                print(f"{t_connector}Topic: [{topic}] (Subscribers: {len(channel_list)})")

                sub_prefix = "    " if is_last_topic else "|   "
                for s_idx, channel in enumerate(channel_list):
                    is_last_sub = (s_idx == len(channel_list) - 1)
                    s_connector = "\\-- " if is_last_sub else "|-- "

                    with channel.lock:
                        pending = len(channel.item_queue)
                    active = channel.worker_thread is not None and channel.worker_thread.is_alive()

                    print(f"{sub_prefix}{s_connector}Subscriber #{s_idx + 1} | "
                          f"Pending Items: {pending} | Worker: {'ACTIVE' if active else 'IDLE'}")
            print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    tq = TopicQueue.get_instance()

    # 1. 구독자 등록 (Stopped 상태에서)
    received_temp = []
    received_logs = []

    tq.subscribe("sensor/temperature", lambda ctx: received_temp.append(ctx.data))
    tq.subscribe("sensor/temperature", lambda ctx: print(f"  -> [Temp Monitor 2] Read: {ctx.data} C"))
    tq.subscribe("system/log", lambda ctx: received_logs.append(ctx.data))

    tq.dump("1. Initial Subscription (Stopped State)")

    # 2. 큐 시작
    tq.start()
    assert tq.get_state() == QueueState.RUNNING

    # 3. 메시지 발행 (Pub-Sub)
    tq.publish("sensor/temperature", 24)
    tq.publish("sensor/temperature", 27)
    tq.publish("system/log", "System boot completed.")

    time.sleep(0.1)  # 비동기 워커 스레드 처리 대기
    tq.dump("2. After Publishing Messages")

    # 4. 큐 종료
    tq.stop()
    assert tq.get_state() == QueueState.STOPPED
    tq.dump("3. After System Stopped")

    assert received_temp == [24, 27]
    assert received_logs == ["System boot completed."]
    print("All TopicQueue tests passed successfully!")

