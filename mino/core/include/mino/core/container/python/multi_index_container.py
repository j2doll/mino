from typing import Generic, TypeVar, Optional, Iterator, Tuple, Dict, Any

Value = TypeVar('Value')
Key1 = TypeVar('Key1')
Key2 = TypeVar('Key2')


class Entry(Generic[Value, Key1, Key2]):
    def __init__(self, value: Value, k1: Key1, k2: Key2):
        self.value: Value = value
        self.k1: Key1 = k1
        self.k2: Key2 = k2

    def __repr__(self):
        return f"Entry(Key1={self.k1}, Key2={self.k2}, Value={self.value})"


class MultiIndexContainer(Generic[Value, Key1, Key2]):
    """2개의 독립된 고유 키를 기반으로 O(1) 조회를 지원하는 다중 인덱스 컨테이너"""
    def __init__(self):
        self._by_key1: Dict[Key1, Entry[Value, Key1, Key2]] = {}
        self._by_key2: Dict[Key2, Entry[Value, Key1, Key2]] = {}

    def insert(self, value: Value, k1: Key1, k2: Key2) -> bool:
        """데이터 삽입: Key1 또는 Key2 중 하나라도 이미 존재하면 False 반환"""
        if k1 in self._by_key1 or k2 in self._by_key2:
            return False

        item = Entry(value, k1, k2)
        self._by_key1[k1] = item
        self._by_key2[k2] = item
        return True

    def emplace(self, k1: Key1, k2: Key2, value: Value) -> bool:
        return self.insert(value, k1, k2)

    def find_by_key1(self, k1: Key1) -> Optional[Value]:
        entry_item = self._by_key1.get(k1)
        return entry_item.value if entry_item else None

    def find_by_key2(self, k2: Key2) -> Optional[Value]:
        entry_item = self._by_key2.get(k2)
        return entry_item.value if entry_item else None

    def contains_key1(self, k1: Key1) -> bool:
        return k1 in self._by_key1

    def contains_key2(self, k2: Key2) -> bool:
        return k2 in self._by_key2

    def erase_by_key1(self, k1: Key1) -> bool:
        if k1 not in self._by_key1:
            return False

        entry_item = self._by_key1.pop(k1)
        self._by_key2.pop(entry_item.k2, None)
        return True

    def erase_by_key2(self, k2: Key2) -> bool:
        if k2 not in self._by_key2:
            return False

        entry_item = self._by_key2.pop(k2)
        self._by_key1.pop(entry_item.k1, None)
        return True

    def size(self) -> int:
        return len(self._by_key1)

    def empty(self) -> bool:
        return len(self._by_key1) == 0

    def __len__(self) -> int:
        return len(self._by_key1)

    def clear(self):
        self._by_key1.clear()
        self._by_key2.clear()

    def __iter__(self) -> Iterator[Entry[Value, Key1, Key2]]:
        return iter(self._by_key1.values())

    def dump(self, title: str = ""):
        """순수 ASCII 기반 터미널 덤프"""
        if title:
            print(f"=== {title} (Size: {self.size()}) ===")
        else:
            print(f"=== Multi Index Container Dump (Size: {self.size()}) ===")

        if self.empty():
            print("  \\-- <Empty Container>\n")
            return

        items = list(self._by_key1.values())
        for idx, e in enumerate(items):
            is_last = (idx == len(items) - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{connector}Key1: [{e.k1}] | Key2: [{e.k2}] => Value: [{e.value}]")
        print()


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
class User:
    def __init__(self, name: str, age: int):
        self.name = name
        self.age = age

    def __repr__(self):
        return f"User(Name='{self.name}', Age={self.age})"


if __name__ == "__main__":
    users = MultiIndexContainer[User, int, str]()

    # 1. 삽입 테스트 (ID, Email)
    assert users.insert(User("Alice", 25), 1, "alice@example.com") is True
    assert users.insert(User("Bob", 30), 2, "bob@example.com") is True
    assert users.insert(User("Charlie", 28), 3, "charlie@example.com") is True
    users.dump("1. Initial Insertion (3 Users)")

    assert users.size() == 3
    assert not users.empty()

    # 2. Key1 및 Key2 검색
    u1 = users.find_by_key1(1)
    assert u1 is not None and u1.name == "Alice" and u1.age == 25

    u3 = users.find_by_key2("charlie@example.com")
    assert u3 is not None and u3.name == "Charlie" and u3.age == 28

    assert users.find_by_key2("unknown@example.com") is None

    # 3. 중복 키 삽입 거부 테스트
    assert users.insert(User("David", 35), 1, "david@example.com") is False  # ID 1 중복
    assert users.insert(User("Eve", 29), 4, "alice@example.com") is False    # Email 중복

    # 4. 삭제 테스트 (erase_by_key1 & erase_by_key2)
    assert users.erase_by_key1(2) is True  # Bob 삭제
    assert users.erase_by_key1(999) is False
    users.dump("2. After erase_by_key1(2) (Bob removed)")

    assert users.erase_by_key2("charlie@example.com") is True  # Charlie 삭제
    assert users.size() == 1
    users.dump("3. After erase_by_key2('charlie@example.com') (Charlie removed)")

    # 5. 초기화
    users.clear()
    assert users.empty()
    users.dump("4. After Clear")

    print("All MultiIndexContainer tests passed successfully!")
