#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>
#include <system_error>
#include <memory>
#include <string>
#include <algorithm>

#include "mino/core/log/log.hpp"

namespace mino::core::resilience {

    class retry_helper {
    public:
        template <typename func_type, typename... args_type>
        static auto retry(
            int max_attempts,
            std::chrono::milliseconds base_delay,
            bool use_backoff,
            const std::shared_ptr<mino::core::log::tinylog::logger>& logger,
            double backoff_factor,
            std::chrono::milliseconds max_delay,
            func_type&& func,
            args_type&&... args
        ) -> decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...))
        {
            using return_t = decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...));
            using duration_type = std::decay_t<decltype(base_delay)>;

            // max_attempts가 0 이하이면 무한 재시도 모드
            const bool is_infinite = (max_attempts <= 0);

            for (int attempt = 1; is_infinite || attempt <= max_attempts; ++attempt)
            {
                try
                {
                    if constexpr (std::is_void<return_t>::value)
                    {
                        std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...);
                        return;
                    }
                    else
                    {
                        auto result = std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...);

                        // 성공일 때 즉시 반환
                        if (!(result == return_t{}))
                        {
                            return result;
                        }

                        if (logger)
                        {
                            logger->warn("Attempt {} returned failure value", attempt);
                        }

                        // 유한 시도 모드에서 마지막 시도에 도달한 경우 결과 반환
                        if (!is_infinite && attempt == max_attempts)
                        {
                            return result;
                        }
                    }
                }
                catch (const std::exception& ex)
                {
                    if (logger)
                    {
                        logger->warn("Attempt {} failed: {}", attempt, ex.what());
                    }
                }
                catch (...)
                {
                    if (logger)
                    {
                        logger->warn("Attempt {} failed: unknown exception", attempt);
                    }
                }

                // 유한 시도 모드에서 마지막 시도인 경우 더 이상 대기하지 않음
                if (!is_infinite && attempt == max_attempts)
                {
                    break;
                }

                // 대기 시간 계산
                duration_type delay = base_delay;
                if (use_backoff)
                {
                    const double exp = static_cast<double>(attempt - 1);

                    // 1. max_delay 상한선이 설정된 경우 (양수값)
                    if (max_delay > std::chrono::milliseconds::zero())
                    {
                        if (base_delay >= max_delay)
                        {
                            delay = max_delay;
                        }
                        else if (backoff_factor > 1.0)
                        {
                            // max_multiplier = max_delay / base_delay
                            const double max_multiplier = static_cast<double>(max_delay.count()) / static_cast<double>(base_delay.count());
                            // backoff_factor^max_exp = max_multiplier => max_exp = log(max_multiplier) / log(backoff_factor)
                            const double max_exp = std::log(max_multiplier) / std::log(backoff_factor);

                            // 이미 상한선에 도달한 지수라면 거듭제곱 계산 없이 max_delay로 고정
                            if (exp >= max_exp)
                            {
                                delay = max_delay;
                            }
                            else
                            {
                                const double mult = std::pow(backoff_factor, exp);
                                const auto scaled = std::chrono::duration_cast<duration_type>(base_delay * mult);
                                delay = (std::min)(scaled, max_delay);
                            }
                        }
                        else
                        {
                            // backoff_factor <= 1.0 (값이 증가하지 않거나 감소)
                            const double mult = std::pow(backoff_factor, exp);
                            delay = std::chrono::duration_cast<duration_type>(base_delay * mult);
                        }
                    }
                    // 2. max_delay 상한선이 없는 경우 (0 이하)
                    else
                    {
                        // int64_t(약 9e18) 오버플로를 방지하기 위해 최대 배수를 1e15로 안전하게 제한
                        const double max_safe_mult = 1e15;
                        double mult = 1.0;

                        if (backoff_factor > 1.0)
                        {
                            const double max_safe_exp = std::log(max_safe_mult) / std::log(backoff_factor);
                            if (exp >= max_safe_exp)
                            {
                                mult = max_safe_mult;
                            }
                            else
                            {
                                mult = std::pow(backoff_factor, exp);
                            }
                        }
                        else
                        {
                            mult = std::pow(backoff_factor, exp);
                        }

                        delay = std::chrono::duration_cast<duration_type>(base_delay * mult);
                    }
                }

                std::this_thread::sleep_for(delay);
            }

            if constexpr (std::is_void<return_t>::value)
            {
                return;
            }
            else
            {
                return return_t{};
            }
        }

        // 편의 오버로딩 1: max_delay 미지정 시 기본 60초 상한선 적용
        template <typename func_type, typename... args_type>
        static auto retry(
            int max_attempts,
            std::chrono::milliseconds base_delay,
            bool use_backoff,
            const std::shared_ptr<mino::core::log::tinylog::logger>& logger,
            double backoff_factor,
            func_type&& func,
            args_type&&... args
        ) -> decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...))
        {
            return retry(
                max_attempts,
                base_delay,
                use_backoff,
                logger,
                backoff_factor,
                std::chrono::milliseconds{ 60000 }, // 기본 상한선 60초
                std::forward<func_type>(func),
                std::forward<args_type>(args)...
            );
        }

        // 편의 오버로딩 2: 기존 기본 호출 지원 (backoff_factor = 2.0, max_delay = 60초)
        template <typename func_type, typename... args_type>
        static auto retry(
            int max_attempts,
            std::chrono::milliseconds base_delay,
            bool use_backoff,
            const std::shared_ptr<mino::core::log::tinylog::logger>& logger,
            func_type&& func,
            args_type&&... args
        ) -> decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...))
        {
            return retry(
                max_attempts,
                base_delay,
                use_backoff,
                logger,
                2.0,                                 // 기본 배수 2.0
                std::chrono::milliseconds{ 60000 }, // 기본 상한선 60초
                std::forward<func_type>(func),
                std::forward<args_type>(args)...
            );
        }
    };

}
