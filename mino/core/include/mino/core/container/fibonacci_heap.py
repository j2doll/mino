import math
from typing import Any, Callable, Optional, List


class FibonacciNode:
    """C++ node와 동일한 원형 이중 연결 피보나치 트리 노드"""
    def __init__(self, value: Any):
        self.value = value
        self.degree: int = 0
        self.marked: bool = False
        self.parent: Optional['FibonacciNode'] = None
        self.child: Optional['FibonacciNode'] = None
        self.left: 'FibonacciNode' = self
        self.right: 'FibonacciNode' = self

    def __repr__(self):
        return f"Node({self.value}, deg={self.degree})"


class FibonacciHeap:
    """C++ fibonacci_heap과 동일한 인터페이스 및 동작 구현체"""
    def __init__(self, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._max_node: Optional[FibonacciNode] = None
        self._size: int = 0
        self._comp = comp

    def empty(self) -> bool:
        return self._size == 0

    def size(self) -> int:
        return self._size

    def __len__(self) -> int:
        return self._size

    def top(self) -> Optional[Any]:
        if self.empty() or not self._max_node:
            return None
        return self._max_node.value

    def emplace(self, value: Any) -> FibonacciNode:
        new_node = FibonacciNode(value)
        self._max_node = self._merge_lists(self._max_node, new_node)
        self._size += 1
        return new_node

    def push(self, value: Any) -> FibonacciNode:
        return self.emplace(value)

    def pop(self) -> bool:
        if self.empty() or not self._max_node:
            return False

        z = self._max_node
        if z.child:
            child = z.child
            while True:
                next_child = child.right
                child.parent = None
                child = next_child
                if child == z.child:
                    break
            self._max_node = self._merge_lists(self._max_node, z.child)

        if z.right == z:
            self._max_node = None
        else:
            z.left.right = z.right
            z.right.left = z.left
            self._max_node = z.right
            self._consolidate()

        self._size -= 1
        return True

    def merge(self, other: 'FibonacciHeap'):
        if self is other or other.empty():
            return
        self._max_node = self._merge_lists(self._max_node, other._max_node)
        self._size += other._size
        other._max_node = None
        other._size = 0

    def clear(self):
        self._max_node = None
        self._size = 0

    def dump(self, title: str = ""):
        """순수 ASCII 기반 피보나치 힙 트리 덤프"""
        if title:
            print(f"=== {title} (Size: {self._size}) ===")
        else:
            print(f"=== Fibonacci Heap Dump (Size: {self._size}) ===")

        if self.empty() or not self._max_node:
            print("  \\-- <Empty Heap>\n")
            return

        roots = []
        curr = self._max_node
        while True:
            roots.append(curr)
            curr = curr.right
            if curr == self._max_node:
                break

        for r in roots:
            print(f"* Root Tree (Root: [{r.value}], Degree: {r.degree})")
            print(f"  [{r.value}]")
            self._dump_children(r, prefix="  ")
        print()

    def _dump_children(self, parent: FibonacciNode, prefix: str):
        if not parent.child:
            return

        children = []
        curr = parent.child
        while True:
            children.append(curr)
            curr = curr.right
            if curr == parent.child:
                break

        for idx, child in enumerate(children):
            is_last = (idx == len(children) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}[{child.value}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(child, next_prefix)

    def _merge_lists(self, a: Optional[FibonacciNode], b: Optional[FibonacciNode]) -> Optional[FibonacciNode]:
        if not a:
            return b
        if not b:
            return a

        a_next = a.right
        b_prev = b.left

        a.right = b
        b.left = a
        a_next.left = b_prev
        b_prev.right = a_next

        return b if self._comp(a.value, b.value) else a

    def _consolidate(self):
        if not self._max_node:
            return

        max_deg = int(2.0 * math.log2(self._size + 1)) + 8
        degree_array: List[Optional[FibonacciNode]] = [None] * max_deg

        root_nodes = []
        curr = self._max_node
        while True:
            root_nodes.append(curr)
            curr = curr.right
            if curr == self._max_node:
                break

        for w in root_nodes:
            x = w
            d = x.degree
            while d < len(degree_array) and degree_array[d] is not None:
                y = degree_array[d]
                if self._comp(x.value, y.value):
                    x, y = y, x
                self._link_nodes(y, x)
                degree_array[d] = None
                d += 1

            if d >= len(degree_array):
                degree_array.extend([None] * (d - len(degree_array) + 8))
            degree_array[d] = x

        self._max_node = None
        for y in degree_array:
            if y is not None:
                y.left = y
                y.right = y
                self._max_node = self._merge_lists(self._max_node, y)

    def _link_nodes(self, y: FibonacciNode, x: FibonacciNode):
        y.left.right = y.right
        y.right.left = y.left
        y.parent = x
        y.left = y
        y.right = y
        x.child = self._merge_lists(x.child, y)
        x.degree += 1
        y.marked = False


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # main.cpp test_fibonacci_heap_all_public 시나리오 검증
    fh1 = FibonacciHeap()
    assert fh1.empty()
    assert fh1.size() == 0

    fh1.push(10)
    fh1.emplace(30)
    fh1.push(20)
    fh1.dump("1. Push (10, 30, 20)")
    assert fh1.size() == 3
    assert fh1.top() == 30

    fh2 = FibonacciHeap()
    fh2.push(50)
    fh2.push(40)
    fh2.dump("fh2 Initial (50, 40)")

    fh1.merge(fh2)
    assert fh1.size() == 5
    assert fh2.empty()
    assert fh1.top() == 50
    fh1.dump("2. After Merge (fh1 + fh2)")

    assert fh1.pop()  # 50 제거 -> Consolidate 발동
    assert fh1.top() == 40
    fh1.dump("3. After Pop (50 removed, Consolidated)")

    fh1.clear()
    assert fh1.empty()
    assert fh1.size() == 0
    assert fh1.top() is None
    assert not fh1.pop()
    print("All FibonacciHeap tests passed successfully!")

