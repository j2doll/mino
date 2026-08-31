from typing import Generic, TypeVar, Optional, Iterator, List, Any

T = TypeVar('T')


class StableNode(Generic[T]):
    """개별 메모리 힙 노드를 시뮬레이션하는 클래스"""
    def __init__(self, value: T):
        self.value: T = value

    def __repr__(self):
        return f"Node({self.value})"


class StableVector(Generic[T]):
    """C++ stable_vector와 동일한 포인터/참조 안정성 벡터 구현체"""
    def __init__(self, init_list: Optional[List[T]] = None):
        self._nodes: List[StableNode[T]] = []
        if init_list:
            for item in init_list:
                self.push_back(item)

    def empty(self) -> bool:
        return len(self._nodes) == 0

    def size(self) -> int:
        return len(self._nodes)

    def __len__(self) -> int:
        return len(self._nodes)

    def capacity(self) -> int:
        return len(self._nodes)

    def reserve(self, new_cap: int):
        pass

    def shrink_to_fit(self):
        pass

    def push_back(self, value: T) -> StableNode[T]:
        node = StableNode(value)
        self._nodes.append(node)
        return node

    def emplace_back(self, value: T) -> StableNode[T]:
        return self.push_back(value)

    def pop_back(self):
        if self._nodes:
            self._nodes.pop()

    def front(self) -> T:
        if self.empty():
            raise IndexError("front() called on empty stable_vector")
        return self._nodes[0].value

    def back(self) -> T:
        if self.empty():
            raise IndexError("back() called on empty stable_vector")
        return self._nodes[-1].value

    def at(self, pos: int) -> Optional[StableNode[T]]:
        if pos < 0 or pos >= len(self._nodes):
            return None
        return self._nodes[pos]

    def __getitem__(self, pos: int) -> T:
        if pos < 0 or pos >= len(self._nodes):
            raise IndexError("Index out of range")
        return self._nodes[pos].value

    def __setitem__(self, pos: int, value: T):
        if pos < 0 or pos >= len(self._nodes):
            raise IndexError("Index out of range")
        self._nodes[pos].value = value

    def clear(self):
        self._nodes.clear()

    def __iter__(self) -> Iterator[T]:
        for node in self._nodes:
            yield node.value

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 상태 덤프"""
        if title:
            print(f"=== {title} (Size: {len(self._nodes)}) ===")
        else:
            print(f"=== Stable Vector Dump (Size: {len(self._nodes)}) ===")

        if self.empty():
            print("  \\-- <Empty Stable Vector>\n")
            return

        for idx, node in enumerate(self._nodes):
            is_last = (idx == len(self._nodes) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{connector}[{idx}] (NodeID: {hex(id(node))}) => {node.value}")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    sv = StableVector[int]()

    # 1. 요소 추가 및 레퍼런스(Node) 보관
    node10 = sv.push_back(10)
    node20 = sv.push_back(20)
    sv.dump("1. Initial Push (10, 20)")

    # 2. 대량 삽입으로 벡터 확장 유발
    for i in range(30, 110, 10):
        sv.push_back(i)
    sv.dump("2. After Adding Many Items (Reallocation)")

    # 3. 재할당 후에도 이전에 얻은 노드 참조가 완벽히 보존됨을 검증
    assert node10.value == 10
    assert node20.value == 20

    # 4. 값 수정
    sv[0] = 99
    assert node10.value == 99
    sv.dump("3. After Modifying Index 0 to 99")

    # 5. Clear
    sv.clear()
    assert sv.empty()
    sv.dump("4. After Clear")

    print("All StableVector tests passed successfully!")
