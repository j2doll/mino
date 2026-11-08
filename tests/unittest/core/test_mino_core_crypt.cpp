#include "catch_amalgamated.hpp"

#include "mino/core/crypt/cipher.hpp"
#include "mino/core/crypt/cipher_utils.hpp"
#include "mino/core/crypt/keyless_cipher.hpp"
#include "mino/core/crypt/ssh_crypto.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("cipher_utils - Hex 변환 및 PKCS#7 패딩 테스트", "[crypt][cipher_utils]") {
    using namespace mino::core::crypt;

    SECTION("to_hex 및 from_hex 상호 변환") {
        const std::vector<uint8_t> origin = { 0x00, 0x1A, 0x2F, 0x9B, 0xFF };
        const std::string hex_str = cipher_utils::to_hex(origin);

        REQUIRE(hex_str == "001a2f9bff");
        REQUIRE(cipher_utils::from_hex(hex_str) == origin);
    }

    SECTION("from_hex 홀수 길이 입력 검증") {
        REQUIRE(cipher_utils::from_hex("abc").empty());
    }

    SECTION("add_padding 및 remove_padding 동작") {
        const std::string text = "HelloWorld"; // 10 bytes -> 6 bytes of 0x06 padding
        auto padded = cipher_utils::add_padding(text, 16);

        REQUIRE(padded.size() == 16);
        REQUIRE(padded.back() == 6);
        REQUIRE(cipher_utils::remove_padding(padded) == text);

        // 정확히 16바이트 배수인 경우 16바이트 패딩 블록 추가 검증
        const std::string exact16 = "1234567890123456";
        auto padded16 = cipher_utils::add_padding(exact16, 16);
        REQUIRE(padded16.size() == 32);
        REQUIRE(padded16.back() == 16);
        REQUIRE(cipher_utils::remove_padding(padded16) == exact16);
    }

    SECTION("remove_padding 비정상 패딩 검증") {
        // 비어있는 입력
        REQUIRE(cipher_utils::remove_padding({}).empty());

        // 패딩 바이트가 블록 크기(16)를 초과하는 경우
        std::vector<uint8_t> invalid_len = { 0x01, 0x02, 0x11 };
        REQUIRE(cipher_utils::remove_padding(invalid_len).empty());

        // 패딩 바이트가 일치하지 않는 손상된 패딩
        std::vector<uint8_t> corrupted = { 't', 'e', 's', 't', 0x02, 0x03 };
        REQUIRE(cipher_utils::remove_padding(corrupted).empty());
    }
}

TEST_CASE("cipher - AES-128 ECB 암복호화 테스트", "[crypt][cipher]") {
    using namespace mino::core::crypt;

    cipher c;

    SECTION("키 미설정 시 빈 문자열 반환") {
        REQUIRE(c.encrypt("hello").empty());
        REQUIRE(c.decrypt("aabbcc").empty());
    }

    SECTION("암호화 및 복호화 무결성") {
        c.set_key("0123456789abcdef");

        const std::string plain = "SecurePayloadMessage!";
        const std::string cipher_hex = c.encrypt(plain);

        REQUIRE_FALSE(cipher_hex.empty());
        REQUIRE(cipher_hex.size() % 32 == 0); // 16바이트의 16진수 표현 단위
        REQUIRE(c.decrypt(cipher_hex) == plain);
    }

    SECTION("비정상 암호문 복호화 실패 처리") {
        c.set_key("0123456789abcdef");

        // 블록 단위(16바이트 = 32 hex 문자)가 아닌 암호문
        REQUIRE(c.decrypt("1234").empty());
        REQUIRE(c.decrypt("").empty());
    }
}

TEST_CASE("keyless_cipher - 인코딩 및 역연산 테스트", "[crypt][keyless_cipher]") {
    using namespace mino::core::crypt;

    SECTION("빈 문자열 처리") {
        REQUIRE(keyless_cipher::encrypt("").empty());
        REQUIRE(keyless_cipher::decrypt("").empty());
    }

    SECTION("문자열 암복호화 일치 여부") {
        const std::string original = "StandardKeylessValue_1234!@#$";
        const std::string encrypted = keyless_cipher::encrypt(original);

        REQUIRE_FALSE(encrypted.empty());
        REQUIRE(encrypted != original);
        REQUIRE(keyless_cipher::decrypt(encrypted) == original);
    }
}

TEST_CASE("sha256 - 표준 테스트 벡터 검증", "[crypt][sha256]") {
    using namespace mino::core::crypt;

    SECTION("빈 입력 해시 (RFC 6234 e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)") {
        auto digest = sha256::hash(nullptr, 0);
        std::vector<uint8_t> digest_vec(digest.begin(), digest.end());
        REQUIRE(cipher_utils::to_hex(digest_vec) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }

    SECTION("'abc' 해시 (ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad)") {
        const std::string input = "abc";
        auto digest = sha256::hash(reinterpret_cast<const uint8_t*>(input.data()), input.size());
        std::vector<uint8_t> digest_vec(digest.begin(), digest.end());
        REQUIRE(cipher_utils::to_hex(digest_vec) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }

    SECTION("인스턴스 분할 업데이트(update) 및 reset 검증") {
        sha256 ctx;
        ctx.update(reinterpret_cast<const uint8_t*>("a"), 1);
        ctx.update(reinterpret_cast<const uint8_t*>("bc"), 2);
        auto digest = ctx.finalize();
        std::vector<uint8_t> digest_vec(digest.begin(), digest.end());
        REQUIRE(cipher_utils::to_hex(digest_vec) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

        ctx.reset();
        auto empty_digest = ctx.finalize();
        std::vector<uint8_t> empty_vec(empty_digest.begin(), empty_digest.end());
        REQUIRE(cipher_utils::to_hex(empty_vec) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
}

TEST_CASE("hmac_sha256 - RFC 4231 테스트 벡터 검증", "[crypt][hmac]") {
    using namespace mino::core::crypt;

    SECTION("RFC 4231 Test Case 2 (Key='Jefe', Data='what do ya want for nothing?')") {
        const std::string key = "Jefe";
        const std::string data = "what do ya want for nothing?";
        auto mac = hmac_sha256(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
            reinterpret_cast<const uint8_t*>(data.data()), data.size());

        std::vector<uint8_t> mac_vec(mac.begin(), mac.end());
        REQUIRE(cipher_utils::to_hex(mac_vec) == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    }
}

TEST_CASE("x25519 - RFC 7748 키 교환 테스트", "[crypt][x25519]") {
    using namespace mino::core::crypt;

    SECTION("Alice와 Bob의 공유 비밀키 일치 여부") {
        // Alice 키쌍 생성용 임의 시크릿
        std::array<uint8_t, 32> alice_sec{};
        for (size_t i = 0; i < 32; ++i) alice_sec[i] = static_cast<uint8_t>(i + 1);

        // Bob 키쌍 생성용 임의 시크릿
        std::array<uint8_t, 32> bob_sec{};
        for (size_t i = 0; i < 32; ++i) bob_sec[i] = static_cast<uint8_t>(i + 33);

        // Curve25519 베이스 포인트 (9)
        std::array<uint8_t, 32> basepoint{};
        basepoint[0] = 9;

        std::array<uint8_t, 32> alice_pub{};
        std::array<uint8_t, 32> bob_pub{};
        x25519::curve25519(alice_pub.data(), alice_sec.data(), basepoint.data());
        x25519::curve25519(bob_pub.data(), bob_sec.data(), basepoint.data());

        // 공유 비밀키 계산
        std::array<uint8_t, 32> shared_alice{};
        std::array<uint8_t, 32> shared_bob{};
        x25519::curve25519(shared_alice.data(), alice_sec.data(), bob_pub.data());
        x25519::curve25519(shared_bob.data(), bob_sec.data(), alice_pub.data());

        REQUIRE(shared_alice == shared_bob);
    }
}

TEST_CASE("aes128_ctr - 스트림 암복호화 및 RFC 카운터 처리", "[crypt][aes128_ctr]") {
    using namespace mino::core::crypt;

    const uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    const uint8_t iv[16] = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
                             0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

    SECTION("암호화 후 복호화 대칭성 검증 (임의 길이 스트림)") {
        const std::string text = "Stream cipher payload spanning across multiple blocks for verification testing.";
        std::vector<uint8_t> plain(text.begin(), text.end());
        std::vector<uint8_t> cipher(plain.size());
        std::vector<uint8_t> decrypted(plain.size());

        aes128_ctr enc;
        enc.init(key, iv);
        enc.process(plain.data(), cipher.data(), plain.size());

        REQUIRE(cipher != plain);

        aes128_ctr dec;
        dec.init(key, iv);
        dec.process(cipher.data(), decrypted.data(), cipher.size());

        REQUIRE(decrypted == plain);
    }

    SECTION("분할 처리(Chunked Stream) 시 스트림 연속성 검증") {
        const std::vector<uint8_t> data(37, 0x42); // 16바이트 배수가 아닌 37바이트
        std::vector<uint8_t> out_all(37);
        std::vector<uint8_t> out_chunked(37);

        // 한 번에 처리
        aes128_ctr engine1;
        engine1.init(key, iv);
        engine1.process(data.data(), out_all.data(), 37);

        // 조각(Chunk) 내어 처리
        aes128_ctr engine2;
        engine2.init(key, iv);
        engine2.process(data.data(), out_chunked.data(), 10);
        engine2.process(data.data() + 10, out_chunked.data() + 10, 15);
        engine2.process(data.data() + 25, out_chunked.data() + 25, 12);

        REQUIRE(out_all == out_chunked);
    }
}
