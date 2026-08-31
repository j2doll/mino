from typing import Generic, TypeVar, Optional, Iterator, List, Any

T = TypeVar('T')


class SmallVector(Generic[T]):
    """C++ small_vector와 동일한 SBO(Small Buffer Optimization) 벡터 구현체"""
    def __init__(self, sbo_capacity: int = 4, init_list: Optional[List[T]] = None):
        self._sbo_cap: int = max(0, sbo_capacity)
        self._data: List[T] = []
        self._capacity: int = self._sbo_cap
        self._is_heap: bool = False

        if init_list:
            for item in init_list:
                self.push_back(item)

    def is_on_stack(self) -> bool:
        return not self._is_heap

    def empty(self) -> bool:
        return len(self._data) == 0

    def size(self) -> int:
        return len(self._data)

    def __len__(self) -> int:
        return len(self._data)

    def capacity(self) -> int:
        return self._capacity

    def reserve(self, new_cap: int):
        if new_cap <= self._capacity:
            return

        allocate_cap = max(new_cap, 1 if self._capacity == 0 else self._capacity * 2)
        self._capacity = allocate_cap
        if self._capacity > self._sbo_cap:
            self._is_heap = True

    def shrink_to_fit(self):
        if not self._is_heap:
            return

        if len(self._data) <= self._sbo_cap:
            self._capacity = self._sbo_cap
            self._is_heap = False
        else:
            self._capacity = len(self._data)

    def push_back(self, value: T):
        if len(self._data) == self._capacity:
            self.reserve(1 if self._capacity == 0 else self._capacity * 2)
        self._data.append(value)

    def emplace_back(self, value: T) -> T:
        self.push_back(value)
        return value

    def pop_back(self):
        if self._data:
            self._data.pop()

    def front(self) -> T:
        if self.empty():
            raise IndexError("front() called on empty small_vector")
        return self._data[0]

    def back(self) -> T:
        if self.empty():
            raise IndexError("back() called on empty small_vector")
        return self._data[-1]

    def at(self, pos: int) -> Optional[T]:
        if pos < 0 or pos >= len(self._data):
            return None
        return self._data[pos]

    def __getitem__(self, pos: int) -> T:
        if pos < 0 or pos >= len(self._data):
            raise IndexError("Index out of range")
        return self._data[pos]

    def __setitem__(self, pos: int, value: T):
        if pos < 0 or pos >= len(self._data):
            raise IndexError("Index out of range")
        self._data[pos] = value

    def clear(self):
        self._data.clear()

    def __iter__(self) -> Iterator[T]:
        return iter(self._data)

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        storage_str = "STACK (SBO Active)" if self.is_on_stack() else "HEAP (Dynamically Allocated)"
        if title:
            print(f"=== {title} (Size: {len(self._data)}/{self._capacity}, SBO Limit: {self._sbo_cap}) ===")
        else:
            print(f"=== Small Vector Dump (Size: {len(self._data)}/{self._capacity}, SBO Limit: {self._sbo_cap}) ===")

        print(f"Storage: [{storage_str}]")
        if self.empty():
            print("  \\-- <Empty Vector>\n")
            return

        elements = [f"[{x}]" for x in self._data]
        print(f"Elements: {' -> '.join(elements)}\n")


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    sv = SmallVector[int](sbo_capacity=4)

    # 1. 스택 버퍼 범위 내 삽입 (10, 20, 30)
    sv.push_back(10)
    sv.push_back(20)
    sv.push_back(30)
    sv.dump("1. Push 3 Items (Within SBO)")
    assert sv.is_on_stack() is True
    assert sv.size() == 3

    # 2. SBO 초과 삽입 (40, 50) -> 힙 동적 할당 전환
    sv.push_back(40)
    sv.push_back(50)
    sv.dump("2. Push 2 More Items (Overflow SBO -> Heap Switch)")
    assert sv.is_on_stack() is False
    assert sv.size() == 5
    assert sv.front() == 10
    assert sv.back() == 50

    # 3. 원소 제거 후 shrink_to_fit -> 스택 버퍼 복귀 검증
    sv.pop_back()  # 50 제거
    sv.pop_back()  # 40 제거
    sv.shrink_to_fit()
    sv.dump("3. Pop 2 Items & shrink_to_fit (Back to Stack)")
    assert sv.is_on_stack() is True
    assert sv.size() == 3

    # 4. Clear
    sv.clear()
    assert sv.empty()
    sv.dump("4. After Clear")

    print("All SmallVector tests passed successfully!")

