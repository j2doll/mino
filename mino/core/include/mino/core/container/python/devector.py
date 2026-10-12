from typing import Generic, TypeVar, Optional, Iterator, List

T = TypeVar('T')


class Devector(Generic[T]):
    """C++ devector와 동일한 양방향 연속 메모리 벡터"""
    def __init__(self, count: int = 0, initial_list: Optional[List[T]] = None):
        self._buffer: List[Optional[T]] = []
        self._front_idx: int = 0
        self._back_idx: int = 0

        if initial_list:
            n = len(initial_list)
            self._reallocate_and_align(n * 2, n // 2)
            for item in initial_list:
                self._buffer[self._back_idx] = item
                self._back_idx += 1
        elif count > 0:
            self._reallocate_and_align(count * 2, count // 2)
            for _ in range(count):
                self._buffer[self._back_idx] = 0  # Default value
                self._back_idx += 1

    def _reallocate_and_align(self, new_capacity: int, front_free: int):
        new_buffer: List[Optional[T]] = [None] * new_capacity
        new_front = front_free
        new_back = new_front

        for i in range(self._front_idx, self._back_idx):
            new_buffer[new_back] = self._buffer[i]
            new_back += 1

        self._buffer = new_buffer
        self._front_idx = new_front
        self._back_idx = new_back

    def _grow_if_needed_front(self):
        if self._front_idx == 0:
            cur_cap = self.capacity()
            new_cap = 4 if cur_cap == 0 else cur_cap * 2
            front_free = new_cap - self.size() - (new_cap // 4)
            self._reallocate_and_align(new_cap, front_free)

    def _grow_if_needed_back(self):
        if self._back_idx == len(self._buffer):
            cur_cap = self.capacity()
            new_cap = 4 if cur_cap == 0 else cur_cap * 2
            front_free = new_cap // 4
            self._reallocate_and_align(new_cap, front_free)

    def empty(self) -> bool:
        return self._front_idx == self._back_idx

    def size(self) -> int:
        return self._back_idx - self._front_idx

    def __len__(self) -> int:
        return self.size()

    def capacity(self) -> int:
        return len(self._buffer)

    def free_front(self) -> int:
        return self._front_idx

    def free_back(self) -> int:
        return len(self._buffer) - self._back_idx

    def push_back(self, value: T):
        self._grow_if_needed_back()
        self._buffer[self._back_idx] = value
        self._back_idx += 1

    def push_front(self, value: T):
        self._grow_if_needed_front()
        self._front_idx -= 1
        self._buffer[self._front_idx] = value

    def pop_back(self):
        if not self.empty():
            self._back_idx -= 1
            self._buffer[self._back_idx] = None

    def pop_front(self):
        if not self.empty():
            self._buffer[self._front_idx] = None
            self._front_idx += 1

    def emplace_back(self, value: T) -> T:
        self.push_back(value)
        return value

    def emplace_front(self, value: T) -> T:
        self.push_front(value)
        return value

    def front(self) -> T:
        if self.empty():
            raise IndexError("front() called on empty devector")
        return self._buffer[self._front_idx]

    def back(self) -> T:
        if self.empty():
            raise IndexError("back() called on empty devector")
        return self._buffer[self._back_idx - 1]

    def at(self, pos: int) -> Optional[T]:
        if pos < 0 or pos >= self.size():
            return None
        return self._buffer[self._front_idx + pos]

    def __getitem__(self, pos: int) -> T:
        if pos < 0 or pos >= self.size():
            raise IndexError("Index out of range")
        return self._buffer[self._front_idx + pos]

    def __setitem__(self, pos: int, value: T):
        if pos < 0 or pos >= self.size():
            raise IndexError("Index out of range")
        self._buffer[self._front_idx + pos] = value

    def clear(self):
        self._buffer = [None] * len(self._buffer)
        mid = len(self._buffer) // 2
        self._front_idx = mid
        self._back_idx = mid

    def __iter__(self) -> Iterator[T]:
        for i in range(self._front_idx, self._back_idx):
            yield self._buffer[i]

    def dump(self, title: str = ""):
        if title:
            print(f"=== {title} (Size: {self.size()}, Cap: {self.capacity()}) ===")
        else:
            print(f"=== Devector Dump (Size: {self.size()}, Cap: {self.capacity()}) ===")

        if self.capacity() == 0:
            print("  \\-- <Unallocated Buffer>\n")
            return

        if self.empty():
            print("Logical Elements: <Empty>")
        else:
            elements = [f"[{self[i]}]" for i in range(self.size())]
            print(f"Logical Elements: {' <-> '.join(elements)}")

        print(f"Memory Layout (Front Free: {self.free_front()}, Back Free: {self.free_back()}):")
        for i in range(len(self._buffer)):
            is_last = (i == len(self._buffer) - 1)
            connector = "\\-- " if is_last else "|-- "

            if self._front_idx <= i < self._back_idx:
                val_str = str(self._buffer[i])
                markers = []
                if i == self._front_idx:
                    markers.append("FRONT")
                if i == self._back_idx - 1:
                    markers.append("BACK")
                marker_str = f" ({'/'.join(markers)})" if markers else ""
                print(f"{connector}[{i}] : {val_str}{marker_str}")
            elif i < self._front_idx:
                print(f"{connector}[{i}] : <Free Front>")
            else:
                print(f"{connector}[{i}] : <Free Back>")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    dv = Devector[int]()

    # 1. push_back (10, 20, 30)
    dv.push_back(10)
    dv.push_back(20)
    dv.push_back(30)
    dv.dump("1. Push Back (10, 20, 30)")

    # 2. push_front (5, 1) -> 양방향 확장 발생
    dv.push_front(5)
    dv.push_front(1)
    dv.dump("2. Push Front (5, 1) -> Geometric Growth to Cap 8")

    # 3. 요소 확인 및 양 끝 pop
    assert dv.front() == 1
    assert dv.back() == 30
    assert dv.at(1) == 5
    assert dv.at(99) is None

    dv.pop_back()   # 30 제거
    dv.pop_front()  # 1 제거
    dv.dump("3. After pop_back & pop_front")

    # 4. 초기화
    dv.clear()
    assert dv.empty()
    dv.dump("4. After Clear")
