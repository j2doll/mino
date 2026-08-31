from typing import Generic, TypeVar, Optional, List, Callable, Any

T = TypeVar('T')


class PriorityQueue(Generic[T]):
    """C++ priority_queue와 동일한 연속 배열 기반 완전 이진 힙 구현체"""
    def __init__(self, init_list: Optional[List[T]] = None, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._comp: Callable[[Any, Any], bool] = comp
        self._c: List[T] = list(init_list) if init_list else []

        if self._c:
            self._make_heap()

    def _make_heap(self):
        # O(N) Bottom-up Heapify
        for i in range((len(self._c) // 2) - 1, -1, -1):
            self._sift_down(i)

    def _sift_up(self, index: int):
        while index > 0:
            parent = (index - 1) // 2
            if self._comp(self._c[parent], self._c[index]):
                self._c[parent], self._c[index] = self._c[index], self._c[parent]
                index = parent
            else:
                break

    def _sift_down(self, index: int):
        size = len(self._c)
        while True:
            best = index
            left = 2 * index + 1
            right = 2 * index + 2

            if left < size and self._comp(self._c[best], self._c[left]):
                best = left
            if right < size and self._comp(self._c[best], self._c[right]):
                best = right

            if best != index:
                self._c[index], self._c[best] = self._c[best], self._c[index]
                index = best
            else:
                break

    def empty(self) -> bool:
        return len(self._c) == 0

    def size(self) -> int:
        return len(self._c)

    def __len__(self) -> int:
        return len(self._c)

    def top(self) -> Optional[T]:
        if self.empty():
            return None
        return self._c[0]

    def push(self, value: T):
        self._c.append(value)
        self._sift_up(len(self._c) - 1)

    def emplace(self, value: T):
        self.push(value)

    def pop(self) -> bool:
        if self.empty():
            return False
        if len(self._c) == 1:
            self._c.pop()
            return True

        self._c[0] = self._c.pop()
        self._sift_down(0)
        return True

    def clear(self):
        self._c.clear()

    def swap(self, other: 'PriorityQueue[T]'):
        self._c, other._c = other._c, self._c
        self._comp, other._comp = other._comp, self._comp

    def dump(self, title: str = ""):
        """순수 ASCII 기반 이진 힙 트리 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._c)}) ===")
        else:
            print(f"=== Priority Queue Dump (Size: {len(self._c)}) ===")

        if self.empty():
            print("  \\-- <Empty Queue>\n")
            return

        print(f"Array: {self._c}")
        print(f"Tree:\n[{self._c[0]}]")
        self._dump_children(0, prefix="")
        print()

    def _dump_children(self, parent_idx: int, prefix: str):
        left = 2 * parent_idx + 1
        right = 2 * parent_idx + 2

        children = []
        if left < len(self._c):
            children.append(left)
        if right < len(self._c):
            children.append(right)

        for i, child_idx in enumerate(children):
            is_last = (i == len(children) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}[{self._c[child_idx]}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(child_idx, next_prefix)


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. Max-Heap (기본 std::less<int> 대응)
    pq = PriorityQueue[int]()
    pq.push(10)
    pq.push(5)
    pq.push(20)
    pq.push(15)
    pq.push(3)
    pq.dump("1. Initial Push (10, 5, 20, 15, 3)")

    assert pq.size() == 5
    assert pq.top() == 20

    # 2. Pop 연산
    assert pq.pop()
    pq.dump("2. After 1st pop (20 removed)")
    assert pq.top() == 15

    # 3. Min-Heap (std::greater<int> 대응)
    min_pq = PriorityQueue[int](comp=lambda a, b: a > b)
    min_pq.push(10)
    min_pq.push(5)
    min_pq.push(20)
    min_pq.push(15)
    min_pq.push(3)
    min_pq.dump("3. Min-Heap Push (10, 5, 20, 15, 3)")
    assert min_pq.top() == 3

    # 4. Swap & Clear
    pq3 = PriorityQueue[int](init_list=[100, 200])
    pq4 = PriorityQueue[int](init_list=[1, 2])
    pq3.swap(pq4)
    assert pq3.top() == 2
    assert pq4.top() == 200

    pq3.clear()
    assert pq3.empty()
    pq3.dump("4. After Clear")

    print("All PriorityQueue tests passed successfully!")
