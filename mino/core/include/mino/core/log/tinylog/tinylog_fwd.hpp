#pragma once

// tinylog 전방 선언
namespace mino::core::log::tinylog {

    // 1. 열거형 전방 선언 (실제 구현부에서 특정 타입을 지정했다면 : int 등을 맞춰주세요)
    enum class log_level;
    enum class encoding_type;
    enum class eol_type;

    // 2. 구조체 전방 선언
    struct base_sink_config;
    struct console_sink_config;
    struct rolling_file_sink_config;

    // 3. 클래스 전방 선언
    class sink;
    class console_sink;
    class rolling_file_sink;

    class logger;

} // namespace mino::core::log::tinylog
