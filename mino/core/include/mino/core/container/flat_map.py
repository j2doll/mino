import bisect
from typing import Generic, TypeVar, Optional, Iterator, Tuple, List, Callable, Any

Key = TypeVar('Key')
T = TypeVar('T')


class FlatMap(Generic[Key, T]):
    """C++ flat_map과 동일한 연속 배열 기반 정렬 맵"""
    def __init__(self, init_list: Optional[List[Tuple[Key, T]]] = None, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._data: List[Tuple[Key, T]] = []
        self._comp = comp

        if init_list:
            for k, v in init_list:
                self.insert((k, v))

    def _lower_bound(self, key: Key) -> int:
        low = 0
        high = len(self._data)
        while low < high:
            mid = (low + high) // 2
            if self._comp(self._data[mid][0], key):
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

    def reserve(self, new_cap: int):
        pass  # Python list는 자동으로 메모리를 관리함

    def at(self, key: Key) -> Optional[T]:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            return self._data[idx][1]
        return None

    def __getitem__(self, key: Key) -> T:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            return self._data[idx][1]
        raise KeyError(f"Key '{key}' not found")

    def __setitem__(self, key: Key, value: T):
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            self._data[idx] = (key, value)
        else:
            self._data.insert(idx, (key, value))

    def insert(self, value: Tuple[Key, T]) -> Tuple[int, bool]:
        key, val = value
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            return idx, False
        self._data.insert(idx, (key, val))
        return idx, True

    def emplace(self, key: Key, val: T) -> Tuple[int, bool]:
        return self.insert((key, val))

    def erase(self, key: Key) -> int:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            self._data.pop(idx)
            return 1
        return 0

    def erase_at(self, index: int):
        if 0 <= index < len(self._data):
            self._data.pop(index)

    def find(self, key: Key) -> int:
        idx = self._lower_bound(key)
        if idx < len(self._data) and not self._comp(key, self._data[idx][0]):
            return idx
        return len(self._data)

    def contains(self, key: Key) -> bool:
        return self.find(key) != len(self._data)

    def clear(self):
        self._data.clear()

    def __iter__(self) -> Iterator[Tuple[Key, T]]:
        return iter(self._data)

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._data)}) ===")
        else:
            print(f"=== Flat Map Dump (Size: {len(self._data)}) ===")

        if self.empty():
            print("  \\-- <Empty Flat Map>\n")
            return

        for idx, (k, v) in enumerate(self._data):
            is_last = (idx == len(self._data) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{connector}[{k}] => {v}")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # main.cpp test_flat_map_all_public 시나리오 검증
    fm1 = FlatMap[int, str]()
    fm3 = FlatMap[int, str](init_list=[(3, "Three"), (1, "One")])

    assert not fm3.empty()
    assert fm3.size() == 2
    fm1.reserve(10)

    # 1. 정렬 확인 (1이 3보다 앞에 위치해야 함)
    assert fm3._data[0][0] == 1
    fm3.dump("1. Initial (Sorted: 1, 3)")

    # 2. at 및 operator[]
    assert fm3.at(1) == "One"
    assert fm3.at(99) is None

    fm3[2] = "Two"
    fm3[4] = "Four"
    assert fm3.size() == 4
    fm3.dump("2. After Adding 2 and 4")

    # 3. insert & emplace
    p1 = (5, "Five")
    assert fm3.insert(p1)[1] is True
    assert fm3.insert((6, "Six"))[1] is True
    assert fm3.emplace(7, "Seven")[1] is True
    assert fm3.insert((5, "Duplicate"))[1] is False  # 중복 키 거부
    fm3.dump("3. After insert & emplace (5, 6, 7)")

    # 4. erase
    idx_7 = fm3.find(7)
    fm3.erase_at(idx_7)
    assert fm3.erase(6) == 1
    assert fm3.erase(999) == 0
    fm3.dump("4. After Erasing 7 and 6")

    # 5. contains & find
    assert fm3.contains(5) is True
    assert fm3.contains(999) is False

    # 6. clear
    fm3.clear()
    assert fm3.empty()
    fm3.dump("5. After Clear")

    print("All FlatMap tests passed successfully!")

