import enum
import threading
import time
from typing import Generic, TypeVar, Optional, Callable, Any

T = TypeVar('T')


class OverflowPolicy(enum.Enum):
    DROP_OLDEST = 1  # 오래된 항목 버림
    REJECT_NEW = 2   # 새 항목 거부


class ConcurrentQueue(Generic[T]):
    """멀티스레드 안전 동시성 큐"""
    def __init__(self, max_size: int = 0, policy: OverflowPolicy = OverflowPolicy.REJECT_NEW):
        self._max_size: int = max(0, max_size)
        self._policy: OverflowPolicy = policy
        self._queue: list[T] = []
        self._lock = threading.Lock()
        self._not_empty_cv = threading.Condition(self._lock)

    def enqueue(self, value: T) -> bool:
        """아이템 추가 (O(1))"""
        with self._not_empty_cv:
            if self._max_size != 0 and len(self._queue) >= self._max_size:
                if self._policy == OverflowPolicy.DROP_OLDEST:
                    self._queue.pop(0)
                else:
                    return False

            self._queue.append(value)
            self._not_empty_cv.notify()
            return True

    def emplace(self, value: T) -> bool:
        return self.enqueue(value)

    def try_dequeue(self) -> Optional[T]:
        """비차단 Dequeue"""
        with self._lock:
            if not self._queue:
                return None
            return self._queue.pop(0)

    def wait_dequeue(self) -> T:
        """대기형 Dequeue (데이터가 들어올 때까지 무한 대기)"""
        with self._not_empty_cv:
            while not self._queue:
                self._not_empty_cv.wait()
            return self._queue.pop(0)

    def wait_dequeue_for(self, timeout_sec: float) -> Optional[T]:
        """타임아웃 대기형 Dequeue"""
        with self._not_empty_cv:
            end_time = time.time() + timeout_sec
            while not self._queue:
                remaining = end_time - time.time()
                if remaining <= 0:
                    return None
                self._not_empty_cv.wait(timeout=remaining)
            return self._queue.pop(0)

    def dequeue_if(self, pred: Callable[[T, int], bool]) -> Optional[T]:
        """조건부 Dequeue: pred(head, size)가 참일 때만 pop"""
        with self._lock:
            if not self._queue:
                return None
            head = self._queue[0]
            current_size = len(self._queue)
            if not pred(head, current_size):
                return None
            return self._queue.pop(0)

    def size(self) -> int:
        with self._lock:
            return len(self._queue)

    def empty(self) -> bool:
        with self._lock:
            return len(self._queue) == 0

    def is_bounded(self) -> bool:
        return self._max_size != 0

    def capacity(self) -> int:
        return self._max_size

    def clear(self) -> int:
        with self._lock:
            removed = len(self._queue)
            self._queue.clear()
            return removed

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        with self._lock:
            cap_str = str(self._max_size) if self._max_size != 0 else "Inf"
            if title:
                print(f"=== {title} (Size: {len(self._queue)}/{cap_str}) ===")
            else:
                print(f"=== Concurrent Queue Dump (Size: {len(self._queue)}/{cap_str}) ===")

            if not self._queue:
                print("  \\-- <Empty Queue>\n")
                return

            elements = [f"[{x}]" for x in self._queue]
            print(f"Front -> {' -> '.join(elements)} -> Back\n")


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. 큐 생성 (용량 3, DROP_OLDEST 정책)
    queue = ConcurrentQueue[int](max_size=3, policy=OverflowPolicy.DROP_OLDEST)

    # 2. 데이터 삽입 및 덮어쓰기
    queue.enqueue(10)
    queue.enqueue(20)
    queue.enqueue(30)
    queue.dump("1. Enqueue 3 Items (Full)")

    queue.enqueue(40)  # 10 제거되고 20, 30, 40 남음
    queue.dump("2. Enqueue 40 (Drop Oldest 10)")

    # 3. 조건부 Dequeue
    val = queue.dequeue_if(lambda head, sz: head > 15)
    print(f"dequeue_if result: {val} (head: 20 > 15 -> Success)")
    queue.dump("3. After dequeue_if")

    # 4. 멀티스레드 생산자-소비자 동작 테스트
    print("=== Multi-thread Producer-Consumer Test ===")
    sync_queue = ConcurrentQueue[int](max_size=5)

    def producer():
        for i in range(1, 4):
            time.sleep(0.05)
            sync_queue.enqueue(i * 100)
            print(f"[Producer] Enqueued: {i * 100}")

    def consumer():
        for _ in range(3):
            item = sync_queue.wait_dequeue()
            print(f"[Consumer] Wait Dequeued: {item}")

    t_prod = threading.Thread(target=producer)
    t_cons = threading.Thread(target=consumer)

    t_cons.start()
    t_prod.start()

    t_prod.join()
    t_cons.join()

    sync_queue.dump("4. Final Queue State")

