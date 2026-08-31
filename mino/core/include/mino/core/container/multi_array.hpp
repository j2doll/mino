#pragma once

#include <array>
#include <vector>
#include <numeric>
#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::array        |  multi_array
// ------------+--------------------+--------------------+------------------
// 차원 지원   | 1차원              | 1차원              | N차원 (컴파일 타임 차수)
// 메모리 구조 | 단일 연속 동적블록 | 단일 연속 스택블록 | 단일 연속 동적블록
// 인덱스 변환 | 단순 인덱스 접근   | 단순 인덱스 접근   | Row-major 스트라이드 계산
// ------------+--------------------+--------------------+------------------
// 임의 접근   | O(1)               | O(1)               | O(1) (Dims회 곱셈/덧셈)
// 연속성 보장 | 완전 보장          | 완전 보장          | 완전 보장 (C-Style 포인터 호환)
// 크기 변경   | 동적 크기 조절     | 고정 크기          | 고정 차원 동적 확장
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 1D 선형 데이터     | 작은 크기 1D 배열  | 행렬 연산, 2D/3D 이미지 텐서,
//             |                    |                    | 공간 시뮬레이션 그리드
// ============+====================+====================+==================
//
// multi_array는 다차원 텐서/배열 데이터를 연속된 단일 1차원 메모리 블록에 저장하고,
// Row-major (C-style) 스트라이드를 기반으로 다차원 인덱싱을 지원하는 컨테이너입니다.
//
// // [1] 2D 배열 생성 (3x4 행렬)
// multi_array<int, 2> arr2d(3, 4);
// 
// // 가변 인자를 이용한 데이터 쓰기 (O(1))
// arr2d(0, 0) = 1;
// arr2d(0, 1) = 2;
// arr2d(1, 2) = 10;
// arr2d(2, 3) = 20;
// 
// // 가변 인자를 이용한 데이터 읽기
// std::cout << "arr2d(0, 0) = " << arr2d(0, 0) << std::endl; // 1
// std::cout << "arr2d(1, 2) = " << arr2d(1, 2) << std::endl; // 10
// 
// // [2] std::array 인덱싱을 이용한 접근
// std::array<std::size_t, 2> idx = { 2, 3 };
// std::cout << "arr2d[{2, 3}] = " << arr2d[idx] << std::endl; // 20
// 
// // [3] 3D 배열 생성 (2x3x4 텐서)
// multi_array<double, 3> arr3d(2, 3, 4);
// arr3d(0, 1, 2) = 3.14;
// arr3d(1, 2, 3) = 2.71;
// 
// // 차원 및 크기 메타데이터 조회
// std::cout << "Dims: " << arr3d.num_dimensions() << std::endl; // 3
// std::cout << "Total size: " << arr3d.size() << std::endl;      // 24 (2 * 3 * 4)
// 
// auto extents = arr3d.extents(); // {2, 3, 4}
// auto strides = arr3d.strides(); // {12, 4, 1}
// 
// // [4] 1차원 선형 포인터 직접 접근
// int* raw_ptr = arr2d.data();
// std::cout << "raw_ptr[6]: " << raw_ptr[6] << std::endl; // 10 (1*4 + 2)
// 

namespace mino::core::container {

    template <typename T, std::size_t Dims>
    class multi_array {
        static_assert(Dims > 0, "multi_array: Dimension Dims must be at least 1");

    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using extents_type = std::array<size_type, Dims>;

        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        multi_array() {
            extents_.fill(0);
            strides_.fill(0);
        }

        explicit multi_array(const extents_type& extents) : extents_(extents) {
            init_storage();
        }

        explicit multi_array(std::initializer_list<size_type> init) {
            extents_.fill(0);
            size_type count = std::min(init.size(), Dims);
            std::copy_n(init.begin(), count, extents_.begin());
            init_storage();
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        explicit multi_array(Args... args) : extents_{ static_cast<size_type>(args)... } {
            init_storage();
        }

        // --------------------------------------------------------------------
        // 1. 인덱스 연산자 (Unchecked / Debug Assert)
        // --------------------------------------------------------------------
        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        reference operator()(Args... indices) noexcept {
            return data_[get_flattened_index({ static_cast<size_type>(indices)... })];
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        const_reference operator()(Args... indices) const noexcept {
            return data_[get_flattened_index({ static_cast<size_type>(indices)... })];
        }

        reference operator[](const extents_type& indices) noexcept {
            return data_[get_flattened_index(indices)];
        }

        const_reference operator[](const extents_type& indices) const noexcept {
            return data_[get_flattened_index(indices)];
        }

        // --------------------------------------------------------------------
        // 2. 런타임 범위 검사 접근 (Checked at)
        // --------------------------------------------------------------------
        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        reference at(Args... indices) {
            return at(extents_type{ static_cast<size_type>(indices)... });
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        const_reference at(Args... indices) const {
            return at(extents_type{ static_cast<size_type>(indices)... });
        }

        reference at(const extents_type& indices) {
            for (size_type i = 0; i < Dims; ++i) {
                if (indices[i] >= extents_[i]) {
                    throw std::out_of_range("multi_array: index out of bounds at dimension " + std::to_string(i));
                }
            }
            return data_[get_flattened_index(indices)];
        }

        const_reference at(const extents_type& indices) const {
            for (size_type i = 0; i < Dims; ++i) {
                if (indices[i] >= extents_[i]) {
                    throw std::out_of_range("multi_array: index out of bounds at dimension " + std::to_string(i));
                }
            }
            return data_[get_flattened_index(indices)];
        }

        // --------------------------------------------------------------------
        // 3. 메타데이터 및 유틸리티
        // --------------------------------------------------------------------
        [[nodiscard]] size_type num_dimensions() const noexcept { return Dims; }
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] const extents_type& extents() const noexcept { return extents_; }
        [[nodiscard]] const extents_type& strides() const noexcept { return strides_; }

        void fill(const T& value) {
            std::fill(data_.begin(), data_.end(), value);
        }

        T* data() noexcept { return data_.data(); }
        const T* data() const noexcept { return data_.data(); }

        iterator begin() noexcept { return data_.begin(); }
        iterator end() noexcept { return data_.end(); }
        const_iterator begin() const noexcept { return data_.cbegin(); }
        const_iterator end() const noexcept { return data_.cend(); }
        const_iterator cbegin() const noexcept { return data_.cbegin(); }
        const_iterator cend() const noexcept { return data_.cend(); }

        // --------------------------------------------------------------------
        // 4. 순수 ASCII 기반 다차원 배열 Dump
        // --------------------------------------------------------------------
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Dims: " << Dims << ", Size: " << size() << ") ===\n";
            }
            else {
                std::cout << "=== Multi Array Dump (Dims: " << Dims << ", Size: " << size() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Array>\n\n";
                return;
            }

            std::cout << "Extents: [";
            for (size_type i = 0; i < Dims; ++i) {
                std::cout << extents_[i] << (i + 1 == Dims ? "" : " x ");
            }
            std::cout << "], Strides: [";
            for (size_type i = 0; i < Dims; ++i) {
                std::cout << strides_[i] << (i + 1 == Dims ? "" : ", ");
            }
            std::cout << "]\n";

            dump_recursive(0, 0, "");
            std::cout << "\n";
        }

    private:
        void init_storage() {
            size_type total_size = 1;
            for (size_type i = 0; i < Dims; ++i) {
                total_size *= extents_[i];
            }
            data_.assign(total_size, T{});
            compute_strides();
        }

        size_type get_flattened_index(const extents_type& indices) const noexcept {
            size_type flattened = 0;
            for (size_type i = 0; i < Dims; ++i) {
                assert(indices[i] < extents_[i] && "Index out of bounds");
                flattened += indices[i] * strides_[i];
            }
            return flattened;
        }

        void compute_strides() noexcept {
            size_type stride = 1;
            for (size_type i = Dims; i > 0; --i) {
                strides_[i - 1] = stride;
                stride *= extents_[i - 1];
            }
        }

        void dump_recursive(size_type dim, size_type offset, const std::string& prefix) const {
            if (dim == Dims - 1) {
                std::cout << "[";
                for (size_type i = 0; i < extents_[dim]; ++i) {
                    std::cout << data_[offset + i * strides_[dim]] << (i + 1 == extents_[dim] ? "" : ", ");
                }
                std::cout << "]\n";
                return;
            }

            for (size_type i = 0; i < extents_[dim]; ++i) {
                bool is_last = (i == extents_[dim] - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << prefix << connector << "Dim[" << dim << "]=" << i << " : ";

                std::string next_prefix = prefix + (is_last ? "    " : "|   ");
                dump_recursive(dim + 1, offset + i * strides_[dim], next_prefix);
            }
        }

        extents_type extents_{};
        extents_type strides_{};
        std::vector<T> data_;
    };

} // namespace mino::core::container
