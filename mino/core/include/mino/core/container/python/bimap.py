from typing import Generic, TypeVar, Optional, Iterator, Tuple, Dict

TLeft = TypeVar('TLeft')
TRight = TypeVar('TRight')


class BiMap(Generic[TLeft, TRight]):
    """C++ bimap과 동일한 1:1 고유 양방향 해시 맵 구현체"""
    def __init__(self):
        self._left_map: Dict[TLeft, TRight] = {}
        self._right_map: Dict[TRight, TLeft] = {}

    def insert(self, left: TLeft, right: TRight) -> bool:
        """데이터 삽입 (어느 한쪽이라도 중복 시 False 반환)"""
        if left in self._left_map or right in self._right_map:
            return False
        self._left_map[left] = right
        self._right_map[right] = left
        return True

    def force_insert(self, left: TLeft, right: TRight):
        """기존 관계를 자동으로 끊고 무조건 1:1 매핑 삽입"""
        if left in self._left_map:
            old_right = self._left_map.pop(left)
            self._right_map.pop(old_right, None)

        if right in self._right_map:
            old_left = self._right_map.pop(right)
            self._left_map.pop(old_left, None)

        self._left_map[left] = right
        self._right_map[right] = left

    def contains_left(self, left: TLeft) -> bool:
        return left in self._left_map

    def contains_right(self, right: TRight) -> bool:
        return right in self._right_map

    def get_by_left(self, left: TLeft) -> Optional[TRight]:
        return self._left_map.get(left)

    def get_by_right(self, right: TRight) -> Optional[TLeft]:
        return self._right_map.get(right)

    def at_left(self, left: TLeft) -> TRight:
        if left not in self._left_map:
            raise KeyError(f"bimap: left key '{left}' not found")
        return self._left_map[left]

    def at_right(self, right: TRight) -> TLeft:
        if right not in self._right_map:
            raise KeyError(f"bimap: right key '{right}' not found")
        return self._right_map[right]

    def erase_by_left(self, left: TLeft) -> bool:
        if left not in self._left_map:
            return False
        right = self._left_map.pop(left)
        self._right_map.pop(right, None)
        return True

    def erase_by_right(self, right: TRight) -> bool:
        if right not in self._right_map:
            return False
        left = self._right_map.pop(right)
        self._left_map.pop(left, None)
        return True

    def size(self) -> int:
        return len(self._left_map)

    def empty(self) -> bool:
        return len(self._left_map) == 0

    def __len__(self) -> int:
        return len(self._left_map)

    def clear(self):
        self._left_map.clear()
        self._right_map.clear()

    def swap(self, other: 'BiMap[TLeft, TRight]'):
        self._left_map, other._left_map = other._left_map, self._left_map
        self._right_map, other._right_map = other._right_map, self._right_map

    def __iter__(self) -> Iterator[Tuple[TLeft, TRight]]:
        return iter(self._left_map.items())

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._left_map)}) ===")
        else:
            print(f"=== BiMap Dump (Size: {len(self._left_map)}) ===")

        if self.empty():
            print("  \\-- <Empty BiMap>\n")
            return

        items = list(self._left_map.items())
        for idx, (l, r) in enumerate(items):
            is_last = (idx == len(items) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{connector}[{l}] <---> [{r}]")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    bm = BiMap[int, str]()

    # 1. 초기 상태 확인
    assert bm.empty()
    assert bm.size() == 0

    # 2. insert 테스트
    assert bm.insert(1, "One") is True
    assert bm.insert(2, "Two") is True
    assert bm.insert(1, "DuplicateKey") is False  # Key 1 중복
    assert bm.size() == 2
    assert not bm.empty()
    bm.dump("1. Initial Insert (1 <-> One, 2 <-> Two)")

    # 3. force_insert 테스트 (기존 1<->One 갱신 및 3<->Uno 중복 처리)
    bm.force_insert(1, "Uno")
    bm.force_insert(3, "Uno")  # Uno 중복 -> 1과의 매핑 해제 후 3<->Uno 연결
    bm.dump("2. After force_insert(3, 'Uno')")

    # 4. 조회 테스트
    assert bm.get_by_left(3) == "Uno"
    assert bm.get_by_right("Two") == 2
    assert bm.get_by_left(1) is None

    # 5. 삭제 테스트 (erase_by_left & erase_by_right)
    assert bm.erase_by_left(2) is True
    assert bm.erase_by_left(999) is False
    assert bm.erase_by_right("Uno") is True
    assert bm.erase_by_right("NotExist") is False
    assert bm.empty()

    # 6. 순회 및 clear
    bm.insert(10, "Ten")
    bm.insert(20, "Twenty")
    bm.dump("3. Before Clear")

    count = 0
    for l, r in bm:
        count += 1
    assert count == 2

    bm.clear()
    assert bm.empty()
    bm.dump("4. After Clear")

    print("All BiMap tests passed successfully!")

