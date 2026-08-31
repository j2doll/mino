from typing import Any, Callable, Optional, List, Tuple


class SkewNode:
    """C++ struct node와 동일한 이진 노드"""
    def __init__(self, value: Any):
        self.value = value
        self.left: Optional['SkewNode'] = None
        self.right: Optional['SkewNode'] = None

    def __repr__(self):
        return f"Node({self.value})"


class SkewHeap:
    """C++ skew_heap과 완전히 동일한 구현체"""
    def __init__(self, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._root: Optional[SkewNode] = None
        self._size: int = 0
        self._comp = comp

    def empty(self) -> bool:
        return self._size == 0

    def size(self) -> int:
        return self._size

    def __len__(self) -> int:
        return self._size

    def top(self) -> Optional[Any]:
        if self.empty() or not self._root:
            return None
        return self._root.value

    def emplace(self, value: Any) -> SkewNode:
        new_node = SkewNode(value)
        self._root = self._merge_nodes(self._root, new_node)
        self._size += 1
        return new_node

    def push(self, value: Any) -> SkewNode:
        return self.emplace(value)

    def pop(self) -> bool:
        if self.empty() or not self._root:
            return False
        old_root = self._root
        self._root = self._merge_nodes(old_root.left, old_root.right)
        old_root.left = old_root.right = None
        self._size -= 1
        return True

    def merge(self, other: 'SkewHeap'):
        if self is other or other.empty():
            return
        self._root = self._merge_nodes(self._root, other._root)
        self._size += other._size
        other._root = None
        other._size = 0

    def clear(self):
        self._root = None
        self._size = 0

    def dump(self, title: str = ""):
        """순수 ASCII 기반 이진 트리 덤프"""
        if title:
            print(f"=== {title} (Size: {self._size}) ===")
        else:
            print(f"=== Skew Heap Dump (Size: {self._size}) ===")

        if self.empty() or not self._root:
            print("  \\-- <Empty Heap>\n")
            return

        print(f"[{self._root.value}]")
        self._dump_children(self._root, prefix="")
        print()

    def _dump_children(self, parent: SkewNode, prefix: str):
        children: List[Tuple[SkewNode, str]] = []
        if parent.left:
            children.append((parent.left, "L"))
        if parent.right:
            children.append((parent.right, "R"))

        for idx, (child, side) in enumerate(children):
            is_last = (idx == len(children) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}({side}) [{child.value}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(child, next_prefix)

    def _merge_nodes(self, h1: Optional[SkewNode], h2: Optional[SkewNode]) -> Optional[SkewNode]:
        if not h1:
            return h2
        if not h2:
            return h1

        if self._comp(h1.value, h2.value):
            h1, h2 = h2, h1

        temp = h1.right
        h1.right = h1.left
        h1.left = self._merge_nodes(temp, h2)

        return h1


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. Max-Heap 생성 및 삽입
    heap = SkewHeap()
    heap.push(10)
    heap.push(5)
    heap.push(20)
    heap.push(15)
    heap.push(3)
    heap.push(7)
    heap.dump("1. Push (10, 5, 20, 15, 3, 7)")

    assert heap.size() == 6
    assert heap.top() == 20

    # 2. 최상위 요소 pop
    assert heap.pop()
    heap.dump("2. After Pop (20 removed)")
    assert heap.top() == 15

    # 3. 힙 병합 테스트
    h1 = SkewHeap()
    for v in [10, 5, 20]:
        h1.push(v)

    h2 = SkewHeap()
    for v in [30, 8, 12]:
        h2.push(v)

    h1.merge(h2)
    assert h1.size() == 6
    assert h1.top() == 30
    assert h2.empty()
    h1.dump("3. After Merge (h1 + h2)")

    # 4. Min-Heap 테스트 (std::greater 대응)
    min_heap = SkewHeap(comp=lambda a, b: a > b)
    for v in [10, 5, 20, 3]:
        min_heap.push(v)
    assert min_heap.top() == 3
    min_heap.dump("4. Min-Heap Structure")

    # 5. Clear
    h1.clear()
    assert h1.empty()
    h1.dump("5. After Clear")

    print("All SkewHeap tests passed successfully!")
