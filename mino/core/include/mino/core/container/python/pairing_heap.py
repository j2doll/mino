from typing import Any, Callable, Optional


class PairingNode:
    """C++ struct node와 동일한 Left-Child / Next-Sibling 노드 구조"""
    def __init__(self, value: Any):
        self.value = value
        self.child: Optional['PairingNode'] = None
        self.next: Optional['PairingNode'] = None
        self.prev: Optional['PairingNode'] = None

    def __repr__(self):
        return f"Node({self.value})"


class PairingHeap:
    """C++ pairing_heap과 완전히 동일한 구현체"""
    def __init__(self, comp: Callable[[Any, Any], bool] = lambda a, b: a < b):
        self._root: Optional[PairingNode] = None
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

    def emplace(self, value: Any) -> PairingNode:
        new_node = PairingNode(value)
        self._root = self._merge_nodes(self._root, new_node)
        self._size += 1
        return new_node

    def push(self, value: Any) -> PairingNode:
        return self.emplace(value)

    def pop(self) -> bool:
        if self.empty() or not self._root:
            return False
        old_root = self._root
        self._root = self._merge_children(old_root.child)
        old_root.child = old_root.next = old_root.prev = None
        self._size -= 1
        return True

    def merge(self, other: 'PairingHeap'):
        if self is other or other.empty():
            return
        self._root = self._merge_nodes(self._root, other._root)
        self._size += other._size
        other._root = None
        other._size = 0

    def update(self, handle: PairingNode, new_value: Optional[Any] = None):
        if not handle:
            return
        if new_value is not None:
            handle.value = new_value

        if handle is self._root:
            ch = handle.child
            handle.child = None
            self._root = self._merge_nodes(handle, self._merge_children(ch))
            return

        self._detach_node(handle)
        ch = handle.child
        handle.child = None
        self._root = self._merge_nodes(self._root, self._merge_nodes(handle, self._merge_children(ch)))

    def erase(self, handle: PairingNode):
        if not handle:
            return
        if handle is self._root:
            self.pop()
            return
        self._detach_node(handle)
        children = handle.child
        handle.child = handle.next = handle.prev = None
        self._size -= 1
        self._root = self._merge_nodes(self._root, self._merge_children(children))

    def clear(self):
        self._root = None
        self._size = 0

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 트리 덤프"""
        if title:
            print(f"=== {title} (Size: {self._size}) ===")
        else:
            print(f"=== Pairing Heap Dump (Size: {self._size}) ===")

        if self.empty() or not self._root:
            print("  \\-- <Empty Heap>\n")
            return

        print(f"[{self._root.value}]")
        self._dump_children(self._root, prefix="")
        print()

    def _dump_children(self, parent: PairingNode, prefix: str):
        children = []
        curr = parent.child
        while curr:
            children.append(curr)
            curr = curr.next

        total = len(children)
        for idx, child in enumerate(children):
            is_last = (idx == total - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}[{child.value}]")
            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_children(child, next_prefix)

    def _merge_nodes(self, n1: Optional[PairingNode], n2: Optional[PairingNode]) -> Optional[PairingNode]:
        if not n1:
            return n2
        if not n2:
            return n1
        if n1 is n2:
            return n1
        if self._comp(n1.value, n2.value):
            n1, n2 = n2, n1

        n2.next = n1.child
        if n1.child:
            n1.child.prev = n2
        n2.prev = n1
        n1.child = n2
        return n1

    def _merge_children(self, first_child: Optional[PairingNode]) -> Optional[PairingNode]:
        if not first_child:
            return None

        current = first_child
        pairs_head = None
        pairs_tail = None

        while current:
            n1 = current
            n2 = current.next
            if n2:
                current = n2.next
                n1.next = n1.prev = n2.next = n2.prev = None
                merged = self._merge_nodes(n1, n2)
                if not pairs_head:
                    pairs_head = pairs_tail = merged
                else:
                    pairs_tail.next = merged
                    merged.prev = pairs_tail
                    pairs_tail = merged
            else:
                n1.next = n1.prev = None
                if not pairs_head:
                    pairs_head = n1
                else:
                    pairs_tail.next = n1
                    n1.prev = pairs_tail
                break

        last = pairs_head
        while last and last.next:
            last = last.next

        result = last
        if result:
            current = result.prev
            while current:
                prev_node = current.prev
                current.next = current.prev = result.next = result.prev = None
                result = self._merge_nodes(current, result)
                current = prev_node

        return result

    def _detach_node(self, n: Optional[PairingNode]):
        if not n or n is self._root:
            return
        if n.prev:
            if n.prev.child is n:
                n.prev.child = n.next
            else:
                n.prev.next = n.next
        if n.next:
            n.next.prev = n.prev
        n.next = n.prev = None


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # Max-Heap 검증 시나리오
    heap = PairingHeap()
    h1 = heap.push(10)
    h2 = heap.push(5)
    h3 = heap.push(20)
    h4 = heap.push(15)
    h5 = heap.push(3)
    heap.dump("1. Initial Push (10, 5, 20, 15, 3)")

    assert heap.size() == 5
    assert heap.top() == 20

    # Pop
    assert heap.pop()
    heap.dump("2. After Pop (20 removed)")
    assert heap.top() == 15

    # Emplace & Merge
    h6 = heap.emplace(25)
    heap2 = PairingHeap()
    heap2.push(30)
    heap2.push(8)
    heap2.push(12)
    heap.merge(heap2)
    heap.dump("3. After Merge with heap2")
    assert heap.size() == 8
    assert heap.top() == 30

    # Update & Erase
    heap.update(h1, 50)
    heap.dump("4. After update(h1, 50)")
    assert heap.top() == 50

    heap.erase(h4)  # 15 삭제
    heap.dump("5. After erase(h4(15))")
    assert heap.size() == 7

    # Clear
    heap.clear()
    assert heap.empty()
    heap.dump("6. After Clear")

    print("All PairingHeap tests passed successfully!")
