from typing import Generic, TypeVar, Tuple, List, Optional, Iterator, Union

T = TypeVar('T')


class MultiArray(Generic[T]):
    """C++ multi_array와 동일한 Row-major N차원 배열 구현체"""
    def __init__(self, *extents: int, default_val: Optional[T] = None):
        if not extents or any(e <= 0 for e in extents):
            self._dims: int = len(extents)
            self._extents: Tuple[int, ...] = extents
            self._strides: Tuple[int, ...] = tuple([0] * len(extents))
            self._data: List[T] = []
            return

        self._dims: int = len(extents)
        self._extents: Tuple[int, ...] = extents
        self._strides = self._compute_strides(self._extents)

        total_size = 1
        for e in self._extents:
            total_size *= e

        self._data: List[T] = [default_val if default_val is not None else 0] * total_size

    def _compute_strides(self, extents: Tuple[int, ...]) -> Tuple[int, ...]:
        strides = [1] * len(extents)
        stride = 1
        for i in range(len(extents) - 1, -1, -1):
            strides[i] = stride
            stride *= extents[i]
        return tuple(strides)

    def _get_flattened_index(self, indices: Tuple[int, ...]) -> int:
        if len(indices) != self._dims:
            raise ValueError(f"Expected {self._dims} indices, got {len(indices)}")

        flattened = 0
        for i in range(self._dims):
            if indices[i] < 0 or indices[i] >= self._extents[i]:
                raise IndexError(f"Index {indices[i]} out of bounds for dimension {i} (extent {self._extents[i]})")
            flattened += indices[i] * self._strides[i]
        return flattened

    def __getitem__(self, indices: Union[Tuple[int, ...], int]) -> T:
        if isinstance(indices, int):
            indices = (indices,)
        return self._data[self._get_flattened_index(indices)]

    def __setitem__(self, indices: Union[Tuple[int, ...], int], value: T):
        if isinstance(indices, int):
            indices = (indices,)
        self._data[self._get_flattened_index(indices)] = value

    def at(self, *indices: int) -> T:
        return self._data[self._get_flattened_index(indices)]

    def num_dimensions(self) -> int:
        return self._dims

    def size(self) -> int:
        return len(self._data)

    def empty(self) -> bool:
        return len(self._data) == 0

    def extents(self) -> Tuple[int, ...]:
        return self._extents

    def strides(self) -> Tuple[int, ...]:
        return self._strides

    def fill(self, value: T):
        for i in range(len(self._data)):
            self._data[i] = value

    def data(self) -> List[T]:
        return self._data

    def __iter__(self) -> Iterator[T]:
        return iter(self._data)

    def dump(self, title: str = ""):
        """순수 ASCII 기반 다차원 텐서 계층 덤프"""
        ext_str = ' x '.join(map(str, self._extents))
        stride_str = ', '.join(map(str, self._strides))

        if title:
            print(f"=== {title} (Dims: {self._dims}, Size: {self.size()}) ===")
        else:
            print(f"=== Multi Array Dump (Dims: {self._dims}, Size: {self.size()}) ===")

        if self.empty():
            print("  \\-- <Empty Array>\n")
            return

        print(f"Extents: [{ext_str}], Strides: [{stride_str}]")
        self._dump_recursive(0, 0, "")
        print()

    def _dump_recursive(self, dim: int, offset: int, prefix: str):
        if dim == self._dims - 1:
            elements = [str(self._data[offset + i * self._strides[dim]]) for i in range(self._extents[dim])]
            print(f"[{', '.join(elements)}]")
            return

        for i in range(self._extents[dim]):
            is_last = (i == self._extents[dim] - 1)
            connector = "\\-- " if is_last else "|-- "
            print(f"{prefix}{connector}Dim[{dim}]={i} : ", end="")

            next_prefix = prefix + ("    " if is_last else "|   ")
            self._dump_recursive(dim + 1, offset + i * self._strides[dim], next_prefix)


# ==========================================
# 실행 및 동작 검증 예제
# ==========================================
if __name__ == "__main__":
    # 1. 2D 배열 (3x4)
    arr2d = MultiArray[int](3, 4)
    arr2d[0, 0] = 1
    arr2d[0, 1] = 2
    arr2d[1, 2] = 10
    arr2d[2, 3] = 20
    arr2d.dump("1. 2D Array (3x4)")

    assert arr2d[0, 0] == 1
    assert arr2d[1, 2] == 10
    assert arr2d[2, 3] == 20
    assert arr2d.size() == 12
    assert arr2d.extents() == (3, 4)
    assert arr2d.strides() == (4, 1)

    # 2. 3D 배열 (2x3x4)
    arr3d = MultiArray[float](2, 3, 4, default_val=0.0)
    arr3d[0, 1, 2] = 3.14
    arr3d[1, 2, 3] = 2.71
    arr3d.dump("2. 3D Tensor (2x3x4)")

    assert arr3d[0, 1, 2] == 3.14
    assert arr3d[1, 2, 3] == 2.71
    assert arr3d.size() == 24
    assert arr3d.extents() == (2, 3, 4)
    assert arr3d.strides() == (12, 4, 1)

    # 3. 일괄 초기화 테스트 (fill)
    arr2d.fill(7)
    assert all(x == 7 for x in arr2d)
    arr2d.dump("3. After fill(7)")

    print("All MultiArray tests passed successfully!")

