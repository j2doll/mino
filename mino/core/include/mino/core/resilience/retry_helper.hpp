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

    class retry_helper {
    public:
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

            for (int attempt = 1; attempt <= max_attempts; ++attempt)
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
                        if (result == return_t{})
                        {
                            return result;
                        }

                        if (logger)
                        {
                            logger->warn("Attempt {} returned failure value", attempt);
                        }

                        if (attempt == max_attempts)
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

                // 재시도 전 대기
                const long long multiplier = static_cast<long long>(std::llround(std::pow(2.0L, static_cast<long double>(attempt - 1))));
                const auto scaled = std::chrono::duration_cast<duration_type>(base_delay * multiplier);
                const duration_type delay = use_backoff ? scaled : base_delay;

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
    };

}
