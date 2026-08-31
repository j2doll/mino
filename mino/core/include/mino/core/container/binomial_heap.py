from typing import Any, Callable, Optional


class BinomialNode:
    """C++ struct node와 동일한 이항 트리 노드"""
    def __init__(self, value: Any):
        self.value = value
        self.degree: int = 0
        self.child: Optional['BinomialNode'] = None
        self.sibling: Optional['BinomialNode'] = None

    def __repr__(self):
        return f"Node(val={self.value}, deg={self.degree})"


class BinomialHeap:
    """
    C++ mino::core::container::binomial_heap과 동일한 구현체
    - 기본 비교자: lambda a, b: a < b (Max-Heap)
    - Min-Heap 사용 시: lambda a, b: a > b 전달
    """
    def __init__(self, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._head: Optional[BinomialNode] = None
        self._size: int = 0
        self._comp = comp

    def empty(self) -> bool:
        return self._size == 0

    def size(self) -> int:
        return self._size

    def __len__(self) -> int:
        return self._size

    def top(self) -> Optional[Any]:
        if self.empty():
            return None
        return self._find_max_node().value

    def emplace(self, value: Any) -> BinomialNode:
        new_node = BinomialNode(value)
        temp_heap = BinomialHeap(comp=self._comp)
        temp_heap._head = new_node
        temp_heap._size = 1
        self.merge(temp_heap)
        return new_node

    def push(self, value: Any) -> BinomialNode:
        return self.emplace(value)

    def pop(self) -> bool:
        if self.empty():
            return False

        # 1. 루트 리스트에서 우선순위 최상위 노드 탐색
        max_prev = None
        max_curr = self._head
        prev = None
        curr = self._head
        max_val = self._head.value

        while curr:
            if self._comp(max_val, curr.value):
                max_val = curr.value
                max_prev = prev
                max_curr = curr
            prev = curr
            curr = curr.sibling

        # 2. 루트 리스트에서 분리
        if max_prev:
            max_prev.sibling = max_curr.sibling
        else:
            self._head = max_curr.sibling

        # 3. 자식 리스트 역순 정렬
        child_curr = max_curr.child
        reversed_child_head = None
        while child_curr:
            next_node = child_curr.sibling
            child_curr.sibling = reversed_child_head
            reversed_child_head = child_curr
            child_curr = next_node

        # 4. O(1) 크기 계산 및 자식 힙 병합
        child_count = (1 << max_curr.degree) - 1
        self._size -= (1 + child_count)

        temp_heap = BinomialHeap(comp=self._comp)
        temp_heap._head = reversed_child_head
        temp_heap._size = child_count

        self.merge(temp_heap)
        return True

    def merge(self, other: 'BinomialHeap'):
        if self is other or other.empty():
            return

        self._head = self._merge_roots(self._head, other._head)
        self._size += other._size
        other._head = None
        other._size = 0

        if not self._head:
            return

        prev = None
        curr = self._head
        next_node = curr.sibling

        while next_node:
            if (curr.degree != next_node.degree) or (next_node.sibling and next_node.sibling.degree == curr.degree):
                prev = curr
                curr = next_node
            elif self._comp(next_node.value, curr.value):
                curr.sibling = next_node.sibling
                self._link_trees(next_node, curr)
            else:
                if not prev:
                    self._head = next_node
                else:
                    prev.sibling = next_node
                self._link_trees(curr, next_node)
                curr = next_node
            next_node = curr.sibling

    def clear(self):
        self._head = None
        self._size = 0

    # ==========================================
    # 순수 ASCII 기반 Tree Dump
    # ==========================================
    def dump(self, title: str = ""):
        if title:
            print(f"=== {title} (Size: {self._size}) ===")
        else:
            print(f"=== Binomial Heap Dump (Size: {self._size}) ===")

        if self.empty():
            print("  \\-- <Empty Heap>\n")
            return

        curr_tree = self._head
        while curr_tree:
            print(f"* Tree B{curr_tree.degree} (Root: [{curr_tree.value}])")
            print(f"  [{curr_tree.value}]")
            self._dump_children(curr_tree, prefix="  ")
            curr_tree = curr_tree.sibling
        print()

    def _dump_children(self, parent: BinomialNode, prefix: str):
        children = []
        curr = parent.child
        while curr:
            children.append(curr)
            curr = curr.sibling

        total = len(children)
        for idx, child in enumerate(children):
            is_last = (idx == total - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}[{child.value}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(child, next_prefix)

    def _find_max_node(self) -> BinomialNode:
        curr = self._head
        max_node = self._head
        while curr:
            if self._comp(max_node.value, curr.value):
                max_node = curr
            curr = curr.sibling
        return max_node

    def _link_trees(self, child: BinomialNode, parent: BinomialNode):
        child.sibling = parent.child
        parent.child = child
        parent.degree += 1

    def _merge_roots(self, h1: Optional[BinomialNode], h2: Optional[BinomialNode]) -> Optional[BinomialNode]:
        if not h1:
            return h2
        if not h2:
            return h1

        root_head = None
        root_tail = None

        while h1 and h2:
            if h1.degree <= h2.degree:
                chosen = h1
                h1 = h1.sibling
            else:
                chosen = h2
                h2 = h2.sibling

            if not root_head:
                root_head = root_tail = chosen
            else:
                root_tail.sibling = chosen
                root_tail = chosen

        if h1:
            root_tail.sibling = h1
        if h2:
            root_tail.sibling = h2

        return root_head


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    heap = BinomialHeap()

    # 1. 요소 삽입 (5, 10, 3, 15) -> 4개 (B2 트리 1개 형성)
    heap.push(5)
    heap.push(10)
    heap.push(3)
    heap.push(15)
    heap.dump("1. Initial Push (5, 10, 3, 15)")

    print(f"Top: {heap.top()}\n")

    # 2. pop() -> 15 제거 (B1, B0 트리로 분할)
    heap.pop()
    heap.dump("2. After Pop (15 removed)")

    # 3. heap2 생성 및 병합 (20, 8 -> B1 트리 1개)
    heap2 = BinomialHeap()
    heap2.push(20)
    heap2.push(8)
    heap2.dump("heap2 Initial Structure")

    heap.merge(heap2)
    heap.dump("3. After Merge (heap + heap2)")

    # 4. Min-Heap 예제 (std::greater 대응)
    min_heap = BinomialHeap(comp=lambda a, b: a > b)
    min_heap.push(10)
    min_heap.push(5)
    min_heap.push(20)
    min_heap.push(3)
    min_heap.dump("4. Min-Heap Structure")
