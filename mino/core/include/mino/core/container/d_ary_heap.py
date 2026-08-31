from typing import Any, Callable, Optional, List


class DAryHeap:
    """C++ d_ary_heap과 동일한 연속 배열 기반 D진 힙 구현체"""
    def __init__(self, d: int = 4, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        if d < 2:
            raise ValueError("d_ary_heap degree D must be at least 2")
        self._d: int = d
        self._comp: Callable[[Any, Any], bool] = comp
        self._data: List[Any] = []

    def empty(self) -> bool:
        return len(self._data) == 0

    def size(self) -> int:
        return len(self._data)

    def __len__(self) -> int:
        return len(self._data)

    def top(self) -> Optional[Any]:
        if self.empty():
            return None
        return self._data[0]

    def push(self, value: Any):
        self._data.append(value)
        self._sift_up(len(self._data) - 1)

    def emplace(self, value: Any):
        self.push(value)

    def pop(self) -> bool:
        if self.empty():
            return False
        if len(self._data) == 1:
            self._data.pop()
            return True

        self._data[0] = self._data.pop()
        self._sift_down(0)
        return True

    def clear(self):
        self._data.clear()

    def _sift_up(self, index: int):
        while index > 0:
            parent = (index - 1) // self._d
            if self._comp(self._data[parent], self._data[index]):
                self._data[parent], self._data[index] = self._data[index], self._data[parent]
                index = parent
            else:
                break

    def _sift_down(self, index: int):
        current_size = len(self._data)
        while True:
            best = index
            first_child = index * self._d + 1

            for i in range(self._d):
                child = first_child + i
                if child < current_size and self._comp(self._data[best], self._data[child]):
                    best = child

            if best != index:
                self._data[index], self._data[best] = self._data[best], self._data[index]
                index = best
            else:
                break

    def dump(self, title: str = ""):
        """순수 ASCII 기반 D진 힙 트리 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._data)}, D={self._d}) ===")
        else:
            print(f"=== D-ary Heap Dump (Size: {len(self._data)}, D={self._d}) ===")

        if self.empty():
            print("  \\-- <Empty Heap>\n")
            return

        print(f"Array: {self._data}")
        print(f"Tree:\n[{self._data[0]}]")
        self._dump_children(0, prefix="")
        print()

    def _dump_children(self, parent_idx: int, prefix: str):
        first_child = parent_idx * self._d + 1
        if first_child >= len(self._data):
            return

        last_child = min(first_child + self._d, len(self._data))
        for i in range(first_child, last_child):
            is_last = (i == last_child - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}[{self._data[i]}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(i, next_prefix)


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. 3진 Max-Heap (D=3, main.cpp test_d_ary_heap_all_public 검증 시나리오)
    heap = DAryHeap(d=3)
    heap.push(10)
    heap.push(30)
    heap.emplace(20)
    heap.push(40)
    heap.dump("1. Push (10, 30, 20, 40) into 3-ary Max-Heap")

    assert heap.top() == 40
    assert heap.pop()
    heap.dump("2. After 1st pop (40 removed)")

    assert heap.top() == 30
    assert heap.pop()
    heap.dump("3. After 2nd pop (30 removed)")

    assert heap.top() == 20
    heap.clear()
    assert heap.empty()
    assert heap.top() is None
    assert not heap.pop()

    # 2. 3진 Min-Heap 예제 (std::greater 대응)
    min_heap = DAryHeap(d=3, comp=lambda a, b: a > b)
    min_heap.push(10)
    min_heap.push(5)
    min_heap.push(20)
    min_heap.push(3)
    min_heap.dump("4. Push (10, 5, 20, 3) into 3-ary Min-Heap")

    assert min_heap.top() == 3
    min_heap.pop()
    min_heap.dump("5. After Min-Heap pop (3 removed)")
    assert min_heap.top() == 5

