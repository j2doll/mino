from typing import Generic, TypeVar, Optional, Iterator, List, Callable, Tuple, Any

T = TypeVar('T')


class FlatMultiSet(Generic[T]):
    """C++ flat_multiset과 동일한 중복 값 허용 연속 배열 정렬 셋"""
    def __init__(self, init_list: Optional[List[T]] = None, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._data: List[T] = []
        self._comp = comp

        if init_list:
            self._data = sorted(init_list, key=lambda x: x if comp(x, x) is False else x)
            # 커스텀 비교자 정렬
            for i in range(1, len(self._data)):
                for j in range(i, 0, -1):
                    if self._comp(self._data[j], self._data[j - 1]):
                        self._data[j], self._data[j - 1] = self._data[j - 1], self._data[j]
                    else:
                        break

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

    def insert(self, value: T) -> int:
        """중복 허용: upper_bound 위치에 삽입하여 FIFO 안정성 유지"""
        idx = self._upper_bound(value)
        self._data.insert(idx, value)
        return idx

    def insert_range(self, items: List[T]):
        for item in items:
            self.insert(item)

    def emplace(self, value: T) -> int:
        return self.insert(value)

    def erase(self, key: T) -> int:
        """해당 값을 가진 모든 요소를 삭제하고 삭제된 개수 반환"""
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

    def count(self, key: T) -> int:
        return self._upper_bound(key) - self._lower_bound(key)

    def find(self, key: T) -> int:
        lb = self._lower_bound(key)
        if lb < len(self._data) and not self._comp(key, self._data[lb]):
            return lb
        return len(self._data)

    def contains(self, key: T) -> bool:
        return self.find(key) != len(self._data)

    def lower_bound(self, key: T) -> int:
        return self._lower_bound(key)

    def upper_bound(self, key: T) -> int:
        return self._upper_bound(key)

    def equal_range(self, key: T) -> Tuple[int, int]:
        return self._lower_bound(key), self._upper_bound(key)

    def swap(self, other: 'FlatMultiSet[T]'):
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
            print(f"=== Flat Multiset Dump (Size: {len(self._data)}) ===")

        if self.empty():
            print("  \\-- <Empty Flat Multiset>\n")
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
    # main.cpp test_flat_multiset_all_public 시나리오 검증
    fms1 = FlatMultiSet[int]()
    init_vec = [3, 1, 2, 1]
    fms4 = FlatMultiSet[int](init_list=init_vec)
    fms5 = FlatMultiSet[int](init_list=[5, 2, 5, 1])

    assert not fms5.empty()
    assert fms5.size() == 4
    fms1.reserve(20)
    fms1.shrink_to_fit()

    # 1. 삽입 테스트 (insert, emplace)
    fms5.insert(10)
    fms5.insert(20)
    fms5.insert_range(init_vec)
    fms5.insert_range([30, 40])
    fms5.emplace(50)
    fms5.dump("1. After Multi-insert & emplace")

    # 2. 삭제 테스트 (erase)
    fms5.erase_at(0)
    fms5.erase_range(0, 2)
    removed = fms5.erase(5)  # 값 5 모두 삭제
    assert removed == 2
    fms5.dump("2. After Erasing 5 and Ranges")

    # 3. 탐색 테스트 (count, find, contains, bounds)
    fms4.insert(2)
    assert fms4.count(2) == 2
    assert fms4.find(2) != len(fms4)
    assert fms4.contains(2) is True
    assert fms4.contains(999) is False

    lb = fms4.lower_bound(2)
    ub = fms4.upper_bound(2)
    eq_s, eq_e = fms4.equal_range(2)
    assert lb == eq_s and ub == eq_e

    # 4. swap 및 clear
    fms1.swap(fms5)
    assert not fms1.empty()
    fms1.clear()
    assert fms1.empty()
    fms1.dump("3. After Swap & Clear")

    print("All FlatMultiSet tests passed successfully!")

