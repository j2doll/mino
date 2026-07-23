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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace mino::core::resilience {

    class  retry_helper {
    public:
        /**
         * @brief error-code or exception based retry helper
         *
         * If the callable returns a non-void value, a return value equal to
         * the default-constructed return type (`return_t{}`) is considered success.
         * Otherwise the helper will retry until `max_attempts` is reached.
         *
         * If the callable throws, exceptions are logged and treated as failures;
         * the last exception is rethrown on the final attempt.
         */
        template <typename func_type, typename... args_type>
        static auto retry(
            int max_attempts,
            std::chrono::milliseconds base_delay,
            bool use_backoff,
            const std::shared_ptr<spdlog::logger>& logger,
            func_type&& func,
            args_type&&... args
        ) -> decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...))
        {
            using return_t = decltype(std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...));
            using duration_type = std::decay_t<decltype(base_delay)>;
            std::exception_ptr last_exception;

            for (int attempt = 1; attempt <= max_attempts; ++attempt)
            {
                try
                {
                    if constexpr (std::is_void<return_t>::value)
                    {
                        // void-returning callable: success if no exception thrown
                        std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...);
                        return;
                    }
                    else
                    {
                        // non-void: treat default-constructed return_t as success (e.g., 0 for int)
                        auto result = std::invoke(std::forward<func_type>(func), std::forward<args_type>(args)...);
                        if (result == return_t{})
                        {
                            return result;
                        }

                        // log failure return value if logger provided
                        if (logger)
                        {
                            logger->warn("Attempt {} returned failure value", attempt);
                        }

                        if (attempt == max_attempts)
                        {
                            // 마지막 시도이면 마지막 반환값을 그대로 반환
                            return result;
                        }
                    }
                }
                catch (...)
                {
                    last_exception = std::current_exception();
                    if (logger)
                    {
                        try
                        {
                            std::rethrow_exception(last_exception);
                        }
                        catch (const std::exception& ex)
                        {
                            logger->warn("Attempt {} failed: {}", attempt, ex.what());
                        }
                        catch (...)
                        {
                            logger->warn("Attempt {} failed: unknown exception", attempt);
                        }
                    }

                    if (attempt == max_attempts)
                    {
                        // 마지막 시도에서 예외가 발생하면 재던짐
                        std::rethrow_exception(last_exception);
                    }
                }

                // 재시도 전 대기
                const long long multiplier = static_cast<long long>(std::llround(std::pow(2.0L, static_cast<long double>(attempt - 1))));
                const auto scaled = std::chrono::duration_cast<duration_type>(base_delay * multiplier);
                const duration_type delay = use_backoff ? scaled : base_delay;

                std::this_thread::sleep_for(delay);
            }

            // 이 지점에는 도달하지 않아야 하나, 안전하게 기본값 반환
            if constexpr (std::is_void<return_t>::value)
            {
                return;
            }
            else
            {
                return return_t{};
            }
        }
    };

} 