from typing import Generic, TypeVar, Optional

T = TypeVar('T')


class CircularBuffer(Generic[T]):
    """C++ circular_buffer와 동일한 인터페이스의 원형 버퍼"""
    def __init__(self, capacity: int):
        self._capacity: int = max(0, capacity)
        self._buffer: list[Optional[T]] = [None] * self._capacity
        self._head: int = 0
        self._tail: int = 0
        self._size: int = 0
        self._valid: bool = (self._capacity > 0)

    def is_valid(self) -> bool:
        return self._valid

    def is_empty(self) -> bool:
        return self._size == 0

    def is_full(self) -> bool:
        return self._size == self._capacity

    def size(self) -> int:
        return self._size

    def capacity(self) -> int:
        return self._capacity

    def __len__(self) -> int:
        return self._size

    def push_back(self, item: T):
        if not self._valid:
            return

        self._buffer[self._tail] = item
        if self.is_full():
            self._head = (self._head + 1) % self._capacity
        else:
            self._size += 1
        self._tail = (self._tail + 1) % self._capacity

    def pop_front(self) -> Optional[T]:
        if not self._valid or self.is_empty():
            return None

        item = self._buffer[self._head]
        self._buffer[self._head] = None
        self._head = (self._head + 1) % self._capacity
        self._size -= 1
        return item

    def front(self) -> Optional[T]:
        if not self._valid or self.is_empty():
            return None
        return self._buffer[self._head]

    def back(self) -> Optional[T]:
        if not self._valid or self.is_empty():
            return None
        last_idx = self._capacity - 1 if self._tail == 0 else self._tail - 1
        return self._buffer[last_idx]

    def __getitem__(self, index: int) -> T:
        if index < 0 or index >= self._size:
            raise IndexError("Index out of range")
        return self._buffer[(self._head + index) % self._capacity]

    def __setitem__(self, index: int, value: T):
        if index < 0 or index >= self._size:
            raise IndexError("Index out of range")
        self._buffer[(self._head + index) % self._capacity] = value

    def clear(self):
        self._buffer = [None] * self._capacity
        self._head = 0
        self._tail = 0
        self._size = 0

    def dump(self, title: str = ""):
        if title:
            print(f"=== {title} (Size: {self._size}/{self._capacity}) ===")
        else:
            print(f"=== Circular Buffer Dump (Size: {self._size}/{self._capacity}) ===")

        if not self._valid:
            print("  \\-- <Invalid Buffer>\n")
            return

        if self.is_empty():
            print("Logical Order: <Empty>")
        else:
            elements = [f"[{self[i]}]" for i in range(self._size)]
            print(f"Logical Order: {' -> '.join(elements)}")

        print("Buffer Array:")
        for i in range(self._capacity):
            is_last = (i == self._capacity - 1)
            connector = "\\-- " if is_last else "|-- "

            occupied = False
            if not self.is_empty():
                if self._head < self._tail:
                    occupied = (self._head <= i < self._tail)
                else:
                    occupied = (i >= self._head or i < self._tail)

            val_str = str(self._buffer[i]) if occupied else "<Empty>"
            markers = []
            if i == self._head and not self.is_empty():
                markers.append("HEAD")
            if i == self._tail:
                markers.append("TAIL")

            marker_str = f" ({'/'.join(markers)})" if markers else ""
            print(f"{connector}[{i}] : {val_str}{marker_str}")
        print()


if __name__ == "__main__":
    # main.cpp test_circular_buffer_all_public 검증 시나리오
    invalid_cb = CircularBuffer[int](0)
    assert not invalid_cb.is_valid()

    cb = CircularBuffer[int](3)
    assert cb.capacity() == 3
    assert cb.size() == 0
    assert cb.is_empty()
    assert not cb.is_full()

    cb.push_back(10)
    cb.push_back(20)
    cb.push_back(30)
    assert cb.is_full()
    assert cb.size() == 3
    assert cb.front() == 10
    assert cb.back() == 30

    cb.push_back(40)  # 10 덮어씀 -> [20, 30, 40]
    assert cb.front() == 20
    assert cb.back() == 40
    assert cb[0] == 20
    assert cb[1] == 30
    assert cb[2] == 40

    cb[0] = 25  # [25, 30, 40]
    assert cb[0] == 25

    item = cb.pop_front()
    assert item == 25
    assert cb.size() == 2

    cb.clear()
    assert cb.is_empty()
    assert cb.size() == 0
    assert cb.front() is None
    assert cb.back() is None
    assert cb.pop_front() is None
    print("All CircularBuffer tests passed successfully!")
