"""
============================================================================
[mino.core.container.RedBlackTree]

Python implementation of the Red-Black Tree matching the C++17 version.

Key Features:
1. Employs a single NIL Sentinel node for clean edge cases and fixup rotations.
2. Path-based traversal (e.g., "L", "R", "LR") and color inspection.
3. Pure ASCII tree visualization dump (print_tree_structure).
4. Tree merging (merge) and full boundary test suites.
============================================================================
"""

from enum import Enum
from typing import Optional, Any
import sys


class NodeColor(Enum):
    RED = 1
    BLACK = 2


class Node:
    def __init__(self, key: Any, color: NodeColor = NodeColor.RED):
        self.key = key
        self.color = color
        self.parent: Optional['Node'] = None
        self.left: Optional['Node'] = None
        self.right: Optional['Node'] = None


class RedBlackTree:
    def __init__(self):
        # Single Sentinel node representing all NIL leaves
        self.nil_node = Node(None, color=NodeColor.BLACK)
        self.nil_node.left = self.nil_node
        self.nil_node.right = self.nil_node
        self.nil_node.parent = self.nil_node
        self.root_node = self.nil_node
        self._element_count = 0

    def _left_rotate(self, x: Node) -> None:
        y = x.right
        x.right = y.left
        if y.left != self.nil_node:
            y.left.parent = x
        y.parent = x.parent
        if x.parent == self.nil_node:
            self.root_node = y
        elif x == x.parent.left:
            x.parent.left = y
        else:
            x.parent.right = y
        y.left = x
        x.parent = y

    def _right_rotate(self, y: Node) -> None:
        x = y.left
        y.left = x.right
        if x.right != self.nil_node:
            x.right.parent = y
        x.parent = y.parent
        if y.parent == self.nil_node:
            self.root_node = x
        elif y == y.parent.right:
            y.parent.right = x
        else:
            y.parent.left = x
        x.right = y
        y.parent = x

    def _insert_fixup(self, z: Node) -> None:
        while z.parent.color == NodeColor.RED:
            if z.parent == z.parent.parent.left:
                y = z.parent.parent.right  # Uncle
                if y.color == NodeColor.RED:
                    # Case 1: Uncle is RED
                    z.parent.color = NodeColor.BLACK
                    y.color = NodeColor.BLACK
                    z.parent.parent.color = NodeColor.RED
                    z = z.parent.parent
                else:
                    if z == z.parent.right:
                        # Case 2: Uncle is BLACK & z is right child (Triangle)
                        z = z.parent
                        self._left_rotate(z)
                    # Case 3: Uncle is BLACK & z is left child (Line)
                    z.parent.color = NodeColor.BLACK
                    z.parent.parent.color = NodeColor.RED
                    self._right_rotate(z.parent.parent)
            else:
                y = z.parent.parent.left  # Uncle
                if y.color == NodeColor.RED:
                    # Case 1: Uncle is RED
                    z.parent.color = NodeColor.BLACK
                    y.color = NodeColor.BLACK
                    z.parent.parent.color = NodeColor.RED
                    z = z.parent.parent
                else:
                    if z == z.parent.left:
                        # Case 2: Uncle is BLACK & z is left child
                        z = z.parent
                        self._right_rotate(z)
                    # Case 3: Uncle is BLACK & z is right child
                    z.parent.color = NodeColor.BLACK
                    z.parent.parent.color = NodeColor.RED
                    self._left_rotate(z.parent.parent)
        self.root_node.color = NodeColor.BLACK

    def insert(self, key: Any) -> bool:
        y = self.nil_node
        x = self.root_node

        while x != self.nil_node:
            y = x
            if key < x.key:
                x = x.left
            elif key > x.key:
                x = x.right
            else:
                return False  # Duplicate keys are rejected

        z = Node(key, color=NodeColor.RED)
        z.parent = y
        z.left = self.nil_node
        z.right = self.nil_node

        if y == self.nil_node:
            self.root_node = z
        elif key < y.key:
            y.left = z
        else:
            y.right = z

        self._insert_fixup(z)
        self._element_count += 1
        return True

    def merge(self, other: 'RedBlackTree') -> None:
        if self is other or other.empty():
            return

        def traverse(cur: Node) -> None:
            if cur == other.get_nil() or cur is None:
                return
            traverse(cur.left)
            self.insert(cur.key)
            traverse(cur.right)

        traverse(other.get_root())

    # Node Access & Navigation
    def get_root(self) -> Optional[Node]:
        return None if self.root_node == self.nil_node else self.root_node

    def get_nil(self) -> Node:
        return self.nil_node

    def get_node_at_path(self, path: str) -> Optional[Node]:
        current = self.root_node
        for dir_char in path:
            if self.is_nil(current):
                return None
            if dir_char in ('L', 'l'):
                current = current.left
            elif dir_char in ('R', 'r'):
                current = current.right
            else:
                return None
        return None if self.is_nil(current) else current

    def find_node(self, key: Any) -> Optional[Node]:
        current = self.root_node
        while not self.is_nil(current):
            if key < current.key:
                current = current.left
            elif key > current.key:
                current = current.right
            else:
                return current
        return None

    def contains(self, key: Any) -> bool:
        return self.find_node(key) is not None

    # State & Color Helpers
    def is_nil(self, target: Optional[Node]) -> bool:
        return target is None or target == self.nil_node

    def is_red(self, target: Optional[Node]) -> bool:
        return not self.is_nil(target) and target.color == NodeColor.RED

    def is_black(self, target: Optional[Node]) -> bool:
        return self.is_nil(target) or target.color == NodeColor.BLACK

    # Container Metadata & Visualization
    def size(self) -> int:
        return self._element_count

    def empty(self) -> bool:
        return self._element_count == 0

    def _print_subtree_ascii(self, current: Node, prefix: str, is_left: bool) -> None:
        if current != self.nil_node:
            branch = "|-- [L] " if is_left else "\\-- [R] "
            color_str = "RED" if current.color == NodeColor.RED else "BLACK"
            print(f"{prefix}{branch}{current.key} ({color_str})")
            next_prefix = prefix + ("|   " if is_left else "    ")
            self._print_subtree_ascii(current.left, next_prefix, True)
            self._print_subtree_ascii(current.right, next_prefix, False)

    def print_tree_structure(self) -> None:
        if self.root_node == self.nil_node:
            print("(Empty Tree)")
            return
        color_str = "RED" if self.root_node.color == NodeColor.RED else "BLACK"
        print(f"[ROOT] {self.root_node.key} ({color_str})")
        self._print_subtree_ascii(self.root_node.left, " ", True)
        self._print_subtree_ascii(self.root_node.right, " ", False)


def print_separator(title: str) -> None:
    print("\n========================================")
    print(f" [TEST] {title}")
    print("========================================")


if __name__ == "__main__":
    # -------------------------------------------------------------
    # Case 1. Empty Tree & Boundary Test
    # -------------------------------------------------------------
    print_separator("1. Empty Tree & Boundary Test")
    empty_tree = RedBlackTree()
    print(f"Empty Tree size: {empty_tree.size()} (empty: {empty_tree.empty()})")
    print(f"get_root() on empty tree: {'nullptr (Passed)' if empty_tree.get_root() is None else 'Failed'}")
    print("ASCII Dump:")
    empty_tree.print_tree_structure()

    # -------------------------------------------------------------
    # Case 2. Insertions & Duplicate Handling
    # -------------------------------------------------------------
    print_separator("2. Insertions & Duplicate Handling")
    tree_a = RedBlackTree()
    dataset = [50, 30, 70, 20, 40, 60, 80, 15, 25, 35, 45]

    for v in dataset:
        inserted = tree_a.insert(v)
        print(f"Insert {v} -> {'OK' if inserted else 'DUP'}")

    re_insert = tree_a.insert(30)
    print(f"\nRe-inserting duplicate key 30: {'Success' if re_insert else 'Failed (Duplicate correctly rejected)'}")
    print(f"Current Tree A size: {tree_a.size()}")

    # -------------------------------------------------------------
    # Case 3. ASCII Tree Structure Dump
    # -------------------------------------------------------------
    print_separator("3. ASCII Tree Structure Dump")
    tree_a.print_tree_structure()

    # -------------------------------------------------------------
    # Case 4. Root Node Inspection & Black Root Invariant
    # -------------------------------------------------------------
    print_separator("4. Root Node Inspection")
    root = tree_a.get_root()
    if root is not None:
        print(f"Root Key: {root.key}")
        print(f"Root is BLACK: {'TRUE (Invariant Satisfied)' if tree_a.is_black(root) else 'FALSE'}")

    # -------------------------------------------------------------
    # Case 5. Path Navigation ("L", "R", "LR", etc.) & Color Check
    # -------------------------------------------------------------
    print_separator("5. Path Navigation & Color Inspection")
    test_paths = ["", "L", "R", "LL", "LR", "RL", "RR", "LLL", "LLR", "LLLL"]

    for path in test_paths:
        node_ptr = tree_a.get_root() if path == "" else tree_a.get_node_at_path(path)
        label = "[ROOT]" if path == "" else path

        if tree_a.is_nil(node_ptr):
            print(f'Path "{label}": -> [NIL / Leaf Node]')
        else:
            color_str = "RED" if tree_a.is_red(node_ptr) else "BLACK"
            direct_enum = "RED" if node_ptr.color == NodeColor.RED else "BLACK"
            print(f'Path "{label}": -> Key = {node_ptr.key} | Color = {color_str} | Enum Direct = {direct_enum}')

    # -------------------------------------------------------------
    # Case 6. Key Search & Pointer Comparisons
    # -------------------------------------------------------------
    print_separator("6. Key Search & Pointer Comparisons")
    for search_key in [40, 15, 999]:
        found = tree_a.find_node(search_key)
        if found is not None:
            parent_key = str(found.parent.key) if not tree_a.is_nil(found.parent) else "NONE (Root)"
            left_key = str(found.left.key) if not tree_a.is_nil(found.left) else "NIL"
            right_key = str(found.right.key) if not tree_a.is_nil(found.right) else "NIL"
            print(f"Found Key [{found.key}]:")
            print(f"  - Parent: {parent_key}")
            print(f"  - Left Child:  {left_key}")
            print(f"  - Right Child: {right_key}")
            print(f"  - Left == get_nil(): {str(found.left == tree_a.get_nil()).lower()}")
        else:
            print(f"Key [{search_key}] not found in tree.")

    # -------------------------------------------------------------
    # Case 7. Tree Merge (Tree A + Tree B)
    # -------------------------------------------------------------
    print_separator("7. Tree Merge (Tree A + Tree B)")
    tree_b = RedBlackTree()
    for v in [5, 25, 45, 90, 100]:  # 25, 45 overlap with Tree A
        tree_b.insert(v)

    print("--- Tree B Structure ---")
    tree_b.print_tree_structure()

    print(f"\nTree A size before merge: {tree_a.size()}")
    tree_a.merge(tree_b)
    print(f"Tree A size after merge: {tree_a.size()}")

    print("\n--- Tree A Structure After Merge ---")
    tree_a.print_tree_structure()

