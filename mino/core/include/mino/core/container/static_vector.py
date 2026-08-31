from typing import Generic, TypeVar, Optional, Iterator, List, Any

T = TypeVar('T')


class StaticVector(Generic[T]):
    """C++ static_vector와 동일한 고정 용량 무할당 스택 벡터 구현체"""
    def __init__(self, capacity: int, init_list: Optional[List[T]] = None):
        self._capacity: int = max(0, capacity)
        self._data: List[Optional[T]] = [None] * self._capacity
        self._size: int = 0

        if init_list:
            for item in init_list:
                if not self.push_back(item):
                    break

    def empty(self) -> bool:
        return self._size == 0

    def size(self) -> int:
        return self._size

    def __len__(self) -> int:
        return self._size

    def capacity(self) -> int:
        return self._capacity

    def max_size(self) -> int:
        return self._capacity

    def push_back(self, value: T) -> bool:
        """데이터 삽입: 용량이 가득 찼으면 False 반환"""
        if self._size >= self._capacity:
            return False
        self._data[self._size] = value
        self._size += 1
        return True

    def emplace_back(self, value: T) -> Optional[T]:
        if self._size >= self._capacity:
            return None
        self._data[self._size] = value
        self._size += 1
        return value

    def pop_back(self):
        if self._size > 0:
            self._size -= 1
            self._data[self._size] = None

    def front(self) -> T:
        if self.empty():
            raise IndexError("front() called on empty static_vector")
        return self._data[0]

    def back(self) -> T:
        if self.empty():
            raise IndexError("back() called on empty static_vector")
        return self._data[self._size - 1]

    def at(self, pos: int) -> Optional[T]:
        if pos < 0 or pos >= self._size:
            return None
        return self._data[pos]

    def __getitem__(self, pos: int) -> T:
        if pos < 0 or pos >= self._size:
            raise IndexError("Index out of range")
        return self._data[pos]

    def __setitem__(self, pos: int, value: T):
        if pos < 0 or pos >= self._size:
            raise IndexError("Index out of range")
        self._data[pos] = value

    def clear(self):
        for i in range(self._size):
            self._data[i] = None
        self._size = 0

    def resize(self, count: int, default_val: Optional[T] = None):
        target = min(count, self._capacity)
        if target < self._size:
            while self._size > target:
                self.pop_back()
        else:
            while self._size < target:
                self.push_back(default_val if default_val is not None else 0)

    def swap(self, other: 'StaticVector[T]'):
        self._data, other._data = other._data, self._data
        self._size, other._size = other._size, self._size
        self._capacity, other._capacity = other._capacity, self._capacity

    def __iter__(self) -> Iterator[T]:
        for i in range(self._size):
            yield self._data[i]

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {self._size}/{self._capacity}) ===")
        else:
            print(f"=== Static Vector Dump (Size: {self._size}/{self._capacity}) ===")

        if self._capacity == 0:
            print("  \\-- <Zero Capacity Vector>\n")
            return

        for i in range(self._capacity):
            is_last = (i == self._capacity - 1)
            connector = "\\-- " if is_last else "|-- "
            val_str = str(self._data[i]) if i < self._size else "<Uninitialized>"
            print(f"{connector}[{i}] : {val_str}")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. 생성 및 데이터 추가 (최대 용량 5)
    vec = StaticVector[int](capacity=5)
    assert vec.push_back(10) is True
    assert vec.push_back(20) is True
    assert vec.push_back(30) is True
    vec.dump("1. Push 3 Items (10, 20, 30)")

    assert vec.size() == 3
    assert vec.front() == 10
    assert vec.back() == 30
    assert vec.at(1) == 20
    assert vec.at(99) is None

    # 2. 가득 채운 후 오버플로우 방어 검증
    assert vec.push_back(40) is True
    assert vec.push_back(50) is True
    assert vec.push_back(60) is False  # 용량 초과로 실패
    vec.dump("2. Push to Full Capacity (5/5)")

    # 3. pop 및 resize
    vec.pop_back()  # 50 제거
    vec.dump("3. After pop_back (Size: 4)")

    vec.resize(2)
    vec.dump("4. After resize(2)")
    assert vec.size() == 2

    # 4. Clear
    vec.clear()
    assert vec.empty()
    vec.dump("5. After Clear")

    print("All StaticVector tests passed successfully!")
