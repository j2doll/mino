#include "mino/core/toml/toml.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>

int test_toml_parsing() {
    namespace toml = mino::core::toml;

    std::cout << "========================================\n";
    std::cout << "  mini_toml Full Spec Verification Test \n";
    std::cout << "========================================\n\n";

    const std::string toml_sample = R"(
# 1. Bare keys & Dotted keys
title = "Mino Engine Config"
app.window.width = 1920
app.window.height = 1080

# 2. Integers, bases, and underscores
max_connections = 1_000_000
hex_color = 0xFF00AA
oct_mask = 0o755
bin_flags = 0b1101

# 3. Floats and special values
frame_rate = 144.0
small_val = 1e-4
pos_inf = inf
neg_inf = -inf

# 4. String variations
escaped_str = "Line 1\nLine 2\t\"Quotes\" and Unicode: \u0041"
literal_path = 'C:\Users\Mino\Documents\regex.*'
multiline_basic = """
First line indentation is ignored \
and line-continuations work properly."""
multiline_literal = '''
'Single quotes' and \backslashes are preserved.
Second line.'''

# 5. Booleans
is_fullscreen = true
vsync_enabled = false

# 6. Date and Time (RFC 3339)
release_date = 2026-09-06
start_time = 14:30:00
timestamp = 2026-09-06T14:30:00Z

# 7. Arrays
resolutions = [ 720, 1080, 1440, 2160 ]
mixed_types = [ 1, "apple", true, 3.14 ]

# 8. Inline tables
database = { host = "127.0.0.1", port = 5432, timeout = 30 }

# 9. Array of Tables
[[servers]]
name = "alpha"
ip = "10.0.0.1"

[[servers]]
name = "beta"
ip = "10.0.0.2"
)";

    try {
        std::cout << "[Test 1] Parsing TOML string...\n";
        toml::toml_table root = toml::parse_toml(toml_sample);
        std::cout << " -> Parsing succeeded!\n\n";

        std::cout << "[Test 2] Verifying parsed data integrity via asserts...\n";

        assert(root["title"].as<std::string>() == "Mino Engine Config");
        auto& app = root["app"].as<toml::toml_table>();
        auto& win = app["window"].as<toml::toml_table>();
        assert(win["width"].as<int64_t>() == 1920);
        assert(win["height"].as<int64_t>() == 1080);

        assert(root["max_connections"].as<int64_t>() == 1000000);
        assert(root["hex_color"].as<int64_t>() == 0xFF00AA);
        assert(root["oct_mask"].as<int64_t>() == 0755);
        assert(root["bin_flags"].as<int64_t>() == 0b1101);

        assert(root["frame_rate"].as<double>() == 144.0);
        assert(std::isinf(root["pos_inf"].as<double>()) && root["pos_inf"].as<double>() > 0);
        assert(std::isinf(root["neg_inf"].as<double>()) && root["neg_inf"].as<double>() < 0);

        assert(root["literal_path"].as<std::string>() == R"(C:\Users\Mino\Documents\regex.*)");
        assert(root["escaped_str"].as<std::string>().find("Unicode: A") != std::string::npos);

        auto& dt = root["timestamp"].as<toml::date_time>();
        assert(dt.year == 2026 && dt.month == 9 && dt.day == 6);
        assert(dt.hour == 14 && dt.minute == 30 && dt.second == 0);
        assert(dt.has_offset && dt.offset_minutes == 0);

        auto& res_arr = root["resolutions"].as<toml::toml_array>();
        assert(res_arr.size() == 4);
        assert(res_arr[1].as<int64_t>() == 1080);

        auto& db = root["database"].as<toml::toml_table>();
        assert(db["host"].as<std::string>() == "127.0.0.1");
        assert(db["port"].as<int64_t>() == 5432);

        auto& servers = root["servers"].as<toml::toml_array>();
        assert(servers.size() == 2);
        assert(servers[0].as<toml::toml_table>()["name"].as<std::string>() == "alpha");
        assert(servers[1].as<toml::toml_table>()["name"].as<std::string>() == "beta");

        std::cout << " -> All assertions passed!\n\n";

        std::cout << "[Test 3] Serializing TOML table (dump output):\n";
        std::cout << "----------------------------------------\n";
        std::string dumped = toml::dump_toml(root);
        std::cout << dumped;
        std::cout << "----------------------------------------\n\n";

        std::cout << "[Test 4] Verifying round-trip by re-parsing serialized string...\n";
        toml::toml_table re_parsed = toml::parse_toml(dumped);
        assert(re_parsed["title"].as<std::string>() == "Mino Engine Config");
        assert(re_parsed["hex_color"].as<int64_t>() == 0xFF00AA);
        assert(re_parsed["servers"].as<toml::toml_array>().size() == 2);
        std::cout << " -> Round-trip verification successful!\n\n";

        const std::string file_path = "test_output.toml";
        std::cout << "[Test 5] Testing file write and read (" << file_path << ")...\n";
        bool save_ok = toml::dump_file(file_path, root);
        assert(save_ok && "Failed to save file");

        toml::toml_table from_file = toml::load_file(file_path);
        assert(from_file["title"].as<std::string>() == "Mino Engine Config");
        std::cout << " -> File I/O test passed!\n\n";

        std::cout << "========================================\n";
        std::cout << "      ALL TESTS PASSED (SUCCESS)        \n";
        std::cout << "========================================\n\n";
    }
    catch (const std::exception& e) {
        std::cerr << "\n[Exception Caught] " << e.what() << "\n";
        return 1;
    }

    return 0;
}

int test_parse_toml_file() {
    namespace fs = std::filesystem;
    namespace toml = mino::core::toml;

    std::cout << "========================================\n";
    std::cout << "   Testing File Parsing: settings.toml  \n";
    std::cout << "========================================\n\n";

    std::string project_dir = PROJECT_DIR;
    fs::path toml_path = fs::path(project_dir) / "settings.toml";

    std::cout << "Loading file from: " << toml_path << "\n";

    try {
        toml::toml_table cfg = toml::load_file(toml_path);

        // 1. Keys & Dotted Keys
        std::cout << "[Check 1] Keys & Dotted Keys...\n";
        assert(cfg["app_name"].as<std::string>() == "MinoEngine");
        assert(cfg["version"].as<int64_t>() == 2);
        auto& engine = cfg["engine"].as<toml::toml_table>();
        auto& render = engine["render"].as<toml::toml_table>();
        assert(render["backend"].as<std::string>() == "Vulkan");
        assert(render["vsync"].as<bool>() == true);

        // 2. Integers, Radices, Underscores
        std::cout << "[Check 2] Integers & Radices...\n";
        assert(cfg["max_entities"].as<int64_t>() == 1000000);
        assert(cfg["hex_mask"].as<int64_t>() == static_cast<int64_t>(0xDEADBEEF));
        assert(cfg["oct_perms"].as<int64_t>() == 0777);
        assert(cfg["bin_flags"].as<int64_t>() == 0b101010);

        // 3. Floats and Infinities
        std::cout << "[Check 3] Floats & Special Values...\n";
        assert(std::abs(cfg["delta_time"].as<double>() - 0.016667) < 1e-6);
        assert(std::abs(cfg["scale_factor"].as<double>() - 2.5e-3) < 1e-6);
        assert(std::isinf(cfg["pos_infinity"].as<double>()) && cfg["pos_infinity"].as<double>() > 0);
        assert(std::isinf(cfg["neg_infinity"].as<double>()) && cfg["neg_infinity"].as<double>() < 0);

        // 4. Strings (4 Variations)
        std::cout << "[Check 4] String variations...\n";
        assert(cfg["window_title"].as<std::string>().find("(Debug)") != std::string::npos);
        assert(cfg["escaped_str"].as<std::string>().find("Unicode: B") != std::string::npos);
        assert(cfg["shader_path"].as<std::string>() == R"(C:\Shaders\Standard\Fragment.glsl)");
        assert(cfg["multiline_basic"].as<std::string>().find("Line 1: No break needed Line 2:") != std::string::npos);
        assert(cfg["multiline_literal"].as<std::string>().find(R"(\n escapes.)") != std::string::npos);

        // 5. Booleans
        std::cout << "[Check 5] Booleans...\n";
        assert(cfg["is_editor"].as<bool>() == true);
        assert(cfg["is_headless"].as<bool>() == false);

        // 6. Dates and Times (RFC 3339)
        std::cout << "[Check 6] RFC 3339 Date and Time...\n";
        auto& build_date = cfg["build_date"].as<toml::date_time>();
        assert(build_date.has_date && build_date.year == 2026 && build_date.month == 9 && build_date.day == 6);

        auto& work_time = cfg["work_time"].as<toml::date_time>();
        assert(work_time.has_time && work_time.hour == 9 && work_time.minute == 30 && work_time.second == 0);

        auto& last_updated = cfg["last_updated"].as<toml::date_time>();
        assert(last_updated.has_date && last_updated.has_time && last_updated.has_offset);
        assert(last_updated.year == 2026 && last_updated.hour == 18 && last_updated.offset_minutes == 0);

        // 7. Arrays (Homogeneous & Heterogeneous)
        std::cout << "[Check 7] Arrays...\n";
        auto& refresh_rates = cfg["supported_refresh_rates"].as<toml::toml_array>();
        assert(refresh_rates.size() == 4);
        assert(refresh_rates[2].as<int64_t>() == 144);

        auto& mixed_payload = cfg["mixed_payload"].as<toml::toml_array>();
        assert(mixed_payload.size() == 4);
        assert(mixed_payload[0].as<int64_t>() == 42);
        assert(mixed_payload[1].as<std::string>() == "string_element");
        assert(mixed_payload[2].as<bool>() == true);
        assert(std::abs(mixed_payload[3].as<double>() - 3.14159) < 1e-5);

        // 8. Inline Tables
        std::cout << "[Check 8] Inline Tables...\n";
        auto& audio = cfg["audio_settings"].as<toml::toml_table>();
        assert(audio["master_volume"].as<int64_t>() == 80);
        assert(audio["mute"].as<bool>() == false);
        assert(audio["output"].as<std::string>() == "speakers");

        // 9. Standard Nested Tables
        std::cout << "[Check 9] Standard Tables ([physics], [physics.collision])...\n";
        auto& physics = cfg["physics"].as<toml::toml_table>();
        assert(std::abs(physics["gravity"].as<double>() - (-9.81)) < 1e-5);
        assert(physics["solver_iterations"].as<int64_t>() == 10);

        auto& collision = physics["collision"].as<toml::toml_table>();
        assert(collision["layer_count"].as<int64_t>() == 32);
        assert(collision["enable_ccd"].as<bool>() == true);

        // 10. Array of Tables ([[plugins]])
        std::cout << "[Check 10] Array of Tables ([[plugins]])...\n";
        auto& plugins = cfg["plugins"].as<toml::toml_array>();
        assert(plugins.size() == 2);
        assert(plugins[0].as<toml::toml_table>()["name"].as<std::string>() == "오픈에이엘");
        assert(plugins[0].as<toml::toml_table>()["priority"].as<int64_t>() == 1);
        assert(plugins[1].as<toml::toml_table>()["name"].as<std::string>() == "VulkanRenderer");
        assert(plugins[1].as<toml::toml_table>()["priority"].as<int64_t>() == 2);

        std::cout << "\n========================================\n";
        std::cout << "  settings.toml ALL CHECKS PASSED (OK)  \n";
        std::cout << "========================================\n";
    }
    catch (const std::exception& e) {
        std::cerr << "\n[Failed] File Parsing Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "=== test_toml_parsing() ===\n";
    if (test_toml_parsing() != 0)
        return 1;

    std::cout << "=== test_parse_toml_file() ===\n";
    if (test_parse_toml_file() != 0)
        return 1;

    return 0;
}
