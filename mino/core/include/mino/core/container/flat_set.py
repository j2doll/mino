from typing import Generic, TypeVar, Optional, Iterator, List, Callable, Tuple, Any

T = TypeVar('T')


class FlatSet(Generic[T]):
    """C++ flat_set과 동일한 고유값 보장 연속 배열 정렬 셋"""
    def __init__(self, init_list: Optional[List[T]] = None, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._data: List[T] = []
        self._comp = comp

        if init_list:
            for item in init_list:
                self.insert(item)

    def _lower_bound(self, key: T) -> int:
        low = 0
        high = len(self._data)
        while low < high:
            mid = (low + high) // 2
            if self._comp(self._data[mid], key):
                low = mid + 1
            else:
                high = mid
        return low

    def _upper_bound(self, key: T) -> int:
        low = 0
        high = len(self._data)
        while low < high:
            mid = (low + high) // 2
            if not self._comp(key, self._data[mid]):
                low = mid + 1
            else:
                high = mid
        return low

    def empty(self) -> bool:
        return len(self._data) == 0

    def size(self) -> int:
        return len(self._data)

    def __len__(self) -> int:
        return len(self._data)

    def capacity(self) -> int:
        return len(self._data)

    def reserve(self, new_cap: int):
        pass

    def shrink_to_fit(self):
        pass

    def insert(self, value: T) -> Tuple[int, bool]:
        """고유값 삽입: 이미 존재하면 (idx, False), 성공 시 (idx, True)"""
        idx = self._lower_bound(value)
        if idx < len(self._data) and not self._comp(value, self._data[idx]):
            return idx, False
        self._data.insert(idx, value)
        return idx, True

    def insert_range(self, items: List[T]):
        for item in items:
            self.insert(item)

    def emplace(self, value: T) -> Tuple[int, bool]:
        return self.insert(value)

    def erase(self, key: T) -> int:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx]):
            self._data.pop(idx)
            return 1
        return 0

    def erase_at(self, index: int):
        if 0 <= index < len(self._data):
            self._data.pop(index)

    def erase_range(self, first_idx: int, last_idx: int):
        del self._data[first_idx:last_idx]

    def count(self, key: T) -> int:
        return 1 if self.contains(key) else 0

    def find(self, key: T) -> int:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx]):
            return idx
        return len(self._data)

    def contains(self, key: T) -> bool:
        idx = self._lower_bound(key)
        return idx < len(self._data) and not self._comp(key, self._data[idx])

    def lower_bound(self, key: T) -> int:
        return self._lower_bound(key)

    def upper_bound(self, key: T) -> int:
        return self._upper_bound(key)

    def equal_range(self, key: T) -> Tuple[int, int]:
        return self._lower_bound(key), self._upper_bound(key)

    def swap(self, other: 'FlatSet[T]'):
        self._data, other._data = other._data, self._data
        self._comp, other._comp = other._comp, self._comp

    def clear(self):
        self._data.clear()

    def __iter__(self) -> Iterator[T]:
        return iter(self._data)

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._data)}) ===")
        else:
            print(f"=== Flat Set Dump (Size: {len(self._data)}) ===")

        if self.empty():
            print("  \\-- <Empty Flat Set>\n")
            return

        for idx, val in enumerate(self._data):
            is_last = (idx == len(self._data) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{connector}[{idx}] : {val}")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    fs = FlatSet[int]()

    # 1. 삽입 테스트 (고유성 및 정렬 검증)
    assert fs.insert(10)[1] is True
    assert fs.insert(5)[1] is True
    assert fs.insert(15)[1] is True
    assert fs.insert(10)[1] is False  # 중복 거부
    fs.dump("1. After Insert (10, 5, 15, duplicate 10)")

    assert fs.size() == 3
    assert fs.contains(5) is True
    assert fs.contains(20) is False
    assert fs.count(15) == 1
    assert fs.count(100) == 0

    # 2. emplace
    assert fs.emplace(20)[1] is True
    fs.dump("2. After emplace(20)")

    # 3. 삭제 테스트 (erase)
    assert fs.erase(10) == 1
    assert fs.erase(999) == 0
    fs.dump("3. After Erasing 10")

    # 4. swap & clear
    fs2 = FlatSet[int](init_list=[1, 2, 3])
    fs.swap(fs2)
    fs.dump("4. After Swap with {1, 2, 3}")

    fs.clear()
    assert fs.empty()
    fs.dump("5. After Clear")

    print("All FlatSet tests passed successfully!")
