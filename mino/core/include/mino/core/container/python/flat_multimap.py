from typing import Generic, TypeVar, Optional, Iterator, Tuple, List, Callable, Any

Key = TypeVar('Key')
T = TypeVar('T')


class FlatMultiMap(Generic[Key, T]):
    """C++ flat_multimap과 동일한 중복 키 허용 연속 배열 정렬 맵"""
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

    def _upper_bound(self, key: Key) -> int:
        low = 0
        high = len(self._data)
        while low < high:
            mid = (low + high) // 2
            if not self._comp(key, self._data[mid][0]):
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

    def insert(self, value: Tuple[Key, T]) -> int:
        """중복 키 허용: upper_bound 위치에 삽입하여 기존 동일 키 뒤에 배치 (안정성 보장)"""
        key, val = value
        idx = self._upper_bound(key)
        self._data.insert(idx, (key, val))
        return idx

    def insert_range(self, items: List[Tuple[Key, T]]):
        for item in items:
            self.insert(item)

    def emplace(self, key: Key, val: T) -> int:
        return self.insert((key, val))

    def erase(self, key: Key) -> int:
        """해당 키를 가진 모든 요소를 삭제하고 삭제된 개수 반환"""
        lb = self._lower_bound(key)
        ub = self._upper_bound(key)
        count = ub - lb
        if count > 0:
            del self._data[lb:ub]
        return count

    def erase_at(self, index: int):
        if 0 <= index < len(self._data):
            self._data.pop(index)

    def erase_range(self, first_idx: int, last_idx: int):
        del self._data[first_idx:last_idx]

    def find(self, key: Key) -> int:
        lb = self._lower_bound(key)
        if lb < len(self._data) and not self._comp(key, self._data[lb][0]):
            return lb
        return len(self._data)

    def count(self, key: Key) -> int:
        return self._upper_bound(key) - self._lower_bound(key)

    def contains(self, key: Key) -> bool:
        return self.find(key) != len(self._data)

    def lower_bound(self, key: Key) -> int:
        return self._lower_bound(key)

    def upper_bound(self, key: Key) -> int:
        return self._upper_bound(key)

    def equal_range(self, key: Key) -> Tuple[int, int]:
        return self._lower_bound(key), self._upper_bound(key)

    def clear(self):
        self._data.clear()

    def __iter__(self) -> Iterator[Tuple[Key, T]]:
        return iter(self._data)

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._data)}) ===")
        else:
            print(f"=== Flat Multimap Dump (Size: {len(self._data)}) ===")

        if self.empty():
            print("  \\-- <Empty Flat Multimap>\n")
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
    # main.cpp test_flat_multimap_all_public 시나리오 검증
    fmm1 = FlatMultiMap[int, str]()
    init_vec = [(1, "One_1"), (2, "Two")]
    fmm3 = FlatMultiMap[int, str](init_list=init_vec)
    fmm4 = FlatMultiMap[int, str](init_list=[(1, "One_1"), (1, "One_2")])

    assert not fmm4.empty()
    assert fmm4.size() == 2
    fmm1.reserve(10)

    # 1. 중복 삽입 (insert, emplace)
    fmm4.insert((1, "One_3"))
    fmm4.insert((3, "Three"))
    fmm4.insert_range(init_vec)
    fmm4.insert_range([(4, "Four"), (4, "Four_2")])
    fmm4.emplace(5, "Five")
    fmm4.dump("1. After Multi-insert & emplace")

    # 2. 탐색 (find, count, bounds, equal_range)
    assert fmm4.find(1) != len(fmm4)
    assert fmm4.count(1) >= 3

    lb = fmm4.lower_bound(1)
    ub = fmm4.upper_bound(1)
    eq_start, eq_end = fmm4.equal_range(1)
    assert lb == eq_start and ub == eq_end

    # 3. 삭제 (erase)
    fmm4.erase_at(0)
    fmm4.erase_range(0, 2)
    removed_cnt = fmm4.erase(4)  # 키 4 2개 모두 삭제
    assert removed_cnt == 2
    fmm4.dump("2. After Erasing 4 and Ranges")

    # 4. clear
    fmm4.clear()
    assert fmm4.empty()
    fmm4.dump("3. After Clear")

    print("All FlatMultiMap tests passed successfully!")

