#pragma once

#include <array>
#include <vector>
#include <numeric>
#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <initializer_list>
#include <cassert>

namespace mino::core::container {

    template <typename T, std::size_t Dims>
    class multi_array {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using extents_type = std::array<size_type, Dims>;

        // std::array 기반 생성자
        explicit multi_array(const extents_type& extents) : extents_(extents) {
            size_type total_size = 1;
            for (size_type i = 0; i < Dims; ++i) {
                total_size *= extents_[i];
            }
            data_.resize(total_size);
            compute_strides();
        }

        // std::initializer_list 기반 생성자: 개수 미달 시 0 채움, 초과 시 자름
        explicit multi_array(std::initializer_list<size_type> init) {
            size_type count = std::min(init.size(), Dims);
            std::copy_n(init.begin(), count, extents_.begin());
            for (size_type i = count; i < Dims; ++i) {
                extents_[i] = 0;
            }

            size_type total_size = 1;
            for (size_type i = 0; i < Dims; ++i) {
                total_size *= extents_[i];
            }
            data_.resize(total_size);
            compute_strides();
        }

        // 가변 인자 생성자 편의 기능 (예: multi_array(3, 4, 5))
        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        explicit multi_array(Args... args) : extents_{ static_cast<size_type>(args)... } {
            size_type total_size = 1;
            for (size_type i = 0; i < Dims; ++i) {
                total_size *= extents_[i];
            }
            data_.resize(total_size);
            compute_strides();
        }

        // 다차원 인덱스 접근 연산자 (가변 인자 템플릿 사용)
        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        reference operator()(Args... indices) noexcept {
            return data_[get_flattened_index(std::array<size_type, Dims>{static_cast<size_type>(indices)...})];
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dims>>
        const_reference operator()(Args... indices) const noexcept {
            return data_[get_flattened_index(std::array<size_type, Dims>{static_cast<size_type>(indices)...})];
        }

        // 복합 인덱스 배열을 통한 접근 (std::array 버전)
        reference operator[](const extents_type& indices) noexcept {
            return data_[get_flattened_index(indices)];
        }

        const_reference operator[](const extents_type& indices) const noexcept {
            return data_[get_flattened_index(indices)];
        }

        // 기본 정보 반환 함수들
        size_type num_dimensions() const noexcept { return Dims; }
        size_type size() const noexcept { return data_.size(); }
        const extents_type& extents() const noexcept { return extents_; }
        const extents_type& strides() const noexcept { return strides_; }

        T* data() noexcept { return data_.data(); }
        const T* data() const noexcept { return data_.data(); }

    private:
        // 다차원 인덱스를 1차원 선형 메모리 주소로 변환
        size_type get_flattened_index(const extents_type& indices) const noexcept {
            size_type flattened = 0;
            for (size_type i = 0; i < Dims; ++i) {
                assert(indices[i] < extents_[i] && "Index out of bounds");
                flattened += indices[i] * strides_[i];
            }
            return flattened;
        }

        // Row-major (C-style) 기준으로 스트라이드 계산
        void compute_strides() noexcept {
            size_type stride = 1;
            for (size_type i = Dims; i > 0; --i) {
                strides_[i - 1] = stride;
                stride *= extents_[i - 1];
            }
        }

        extents_type extents_{};
        extents_type strides_{};
        std::vector<T> data_;
    };

} // namespace mino::core::container
