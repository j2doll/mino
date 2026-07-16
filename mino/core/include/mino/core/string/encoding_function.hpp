#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <locale>

#include "mino/core/string/u8.hpp"

namespace mino::core::string
{
    // Unicode 문자 집합을 표현하는 인코딩 : UTF-8, UTF-16, UTF-32.
    // KS X 1001 문자 집합을 표현하는 인코딩 : EUC-KR, ISO-2022-KR.
    // 
    // UTF-8 <-> UTF-16
     bool utf8_to_utf16(const std::string& utf8, std::u16string& out);
     bool utf16_to_utf8(const std::u16string& u16, std::string& out);

     std::wstring utf8_to_utf16(const std::string& utf8);
     std::string utf16_to_utf8(const std::wstring& utf16);

    // UTF-8 <-> UTF-32
     bool utf8_to_utf32(const std::string& utf8, std::u32string& out);
     bool utf32_to_utf8(const std::u32string& u32, std::string& out);

    // UTF-16 <-> UTF-32
     bool utf16_to_utf32(const std::u16string& u16, std::u32string& out);
     bool utf32_to_utf16(const std::u32string& u32, std::u16string& out);

    // ---------- CP949 ----------
    // UTF-8 <-> CP949 (EUC-KR superset; Windows: CP 949)
     bool utf8_to_cp949(const std::string& utf8, std::string& out_cp949);
     bool cp949_to_utf8(const std::string& cp949, std::string& out_utf8);

    // UTF-16 <-> CP949
     bool utf16_to_cp949(const std::u16string& u16, std::string& out_cp949);
     bool cp949_to_utf16(const std::string& cp949, std::u16string& out_u16);

    // ---------- ISO-2022-KR ----------
    // UTF-8 <-> ISO-2022-KR
     bool utf8_to_iso2022kr(const std::string& utf8, std::string& out_iso2022kr);
     bool iso2022kr_to_utf8(const std::string& iso2022kr, std::string& out_utf8);
    // UTF-16 <-> ISO-2022-KR
     bool utf16_to_iso2022kr(const std::u16string& u16, std::string& out_iso2022kr);
     bool iso2022kr_to_utf16(const std::string& iso2022kr, std::u16string& out_u16);

    // ISO-2022-KR vs CP949 차이 요약
    //
    // 1) 표준/성격
    // - ISO-2022-KR: 인터넷 표준(MIME) 기반의 상태 전환(stateful) 인코딩입니다.
    // - CP949: 마이크로소프트 확장 EUC-KR(코드페이지 949), 상태 비의존적입니다.
    //
    // 2) 상태 전환 유무
    // - ISO-2022-KR: ESC 시퀀스(예: ESC $ ) C)로 문자셋을 전환합니다.
    // - CP949: 고정 단일 바이트/멀티바이트 조합으로 상태 전환이 없습니다.
    //
    // 3) 바이트 구성
    // - ISO-2022-KR: ASCII(1바이트)와 KS X 1001(2바이트)을 ESC로 전환해 사용합니다.
    // - CP949: ASCII(1바이트)+확장 한글/한자(주로 2바이트)로 표현합니다.
    //
    // 4) 문자 범위/포함
    // - ISO-2022-KR: KS X 1001(2350자 한글·기본 기호) 범위 중심입니다.
    // - CP949: KS X 1001+확장 완성형 한글(1만+), 추가 한자와 기호를 포함합니다.
    //
    // 5) 상호 호환
    // - ISO-2022-KR 문서는 CP949로 대체 해석 시 ESC 시퀀스가 깨질 수 있습니다.
    // - CP949 문서는 ISO-2022-KR로 변환 시 확장 문자 손실 위험이 큽니다.
    //
    // 6) 사용 맥락
    // - ISO-2022-KR: 구형 메일/MIME 호환 전송에 강점(7비트 경로 통과 목적).
    // - CP949: Windows/국내 레거시 SW/문서에서 널리 쓰였습니다.
    //
    // 7) 네트워크/전송 특성
    // - ISO-2022-KR: 7비트 안전 전송(게이트웨이에서 8비트 손실 방지)이 장점입니다.
    // - CP949: 8비트 경로 필요, 메일 전송 시 Base64/Quoted-Printable이 흔합니다.
    //
    // 8) 자동 판별 난이도
    // - ISO-2022-KR: ESC 바이트(0x1B) 존재로 판별이 비교적 쉽습니다.
    // - CP949: EUC-KR과 유사하여 휴리스틱·BOM 부재로 판별이 어렵습니다.
    //
    // 9) 유니코드 매핑
    // - ISO-2022-KR: KS X 1001 한정으로 매핑 단순(없는 문자는 손실·대체).
    // - CP949: 확장 문자 매핑 다양, 역매핑 시 충돌/비표준 코드 주의 필요.
    //
    // 10) 파일 간 교환
    // - 장기 보존/교환은 UTF-8 권장입니다. 두 레거시 간 변환은 손실이 생깁니다.
    // - 특히 CP949→ISO-2022-KR 변환은 확장 문자 손실 가능성이 높습니다.
    //
    // 11) Windows/리눅스 API 관점
    // - CP949: Windows 코드페이지 949로 직접 지정 가능합니다.
    // - ISO-2022-KR: 일반 코드페이지가 아닌 MIME/콘텐츠 전송 인코딩에 가깝습니다.
    //
    // 12) C++ 처리 시 권장
    // - 내부 문자열은 UTF-8로 유지하고, 입출력 경계에서만 변환을 권장합니다.
    // - ISO-2022-KR 파싱은 상태 기계(ESC, SO/SI 등) 구현이 필요합니다.
    // - CP949 디코딩은 멀티바이트 테이블 기반으로 비교적 단순합니다.
    //
    // 13) 흔한 문제
    // - ISO-2022-KR 텍스트를 CP949로 열면 ESC가 그대로 노출되어 깨집니다.
    // - CP949 확장 한글은 ISO-2022-KR/KS X 1001에 없어 물음표로 대체됩니다.
    //
    // 14) 요약 결론
    // - ISO-2022-KR: 전송 안정성(7비트) 목적의 상태 전환 인코딩입니다.
    // - CP949: Windows 중심 확장 완성형(8비트), 문자 범위가 더 넓습니다.
    // - 현대 개발에서는 UTF-8을 기본으로, 레거시 호환은 변환기로 처리하십시오.

    // ---------- JOHAB (CP 1361) ----------
    // UTF-8 <-> JOHAB
     bool utf8_to_johab(const std::string& utf8, std::string& out_johab);
     bool johab_to_utf8(const std::string& johab, std::string& out_utf8);
    // UTF-16 <-> JOHAB
     bool utf16_to_johab(const std::u16string& u16, std::string& out_johab);
     bool johab_to_utf16(const std::string& johab, std::u16string& out_u16);

    // JOHAB (CP1361) 요약  
    //
    // 1) 개요/성격
    // - JOHAB: 조합형(algorithmic) 한글 인코딩, MS 코드페이지 1361입니다.
    // - 상태 비의존적 멀티바이트 인코딩으로, 전환 시퀀스가 없습니다.
    //
    // 2) 조합형 vs 완성형
    // - JOHAB: 초성·중성·종성 조합 규칙으로 코드값을 산출합니다.
    // - CP949: 미리 정한 완성형 테이블(매핑표)로 코드값을 정합니다.
    //
    // 3) 문자 범위/포함
    // - JOHAB: 현대 11172 음절 대부분과 자모를 표현(조합 기반)합니다.
    // - 드물게 고어 자모 일부 표현이 가능하나 구현별 차이가 있습니다.
    //
    // 4) 바이트 구성
    // - ASCII(1바이트)는 그대로, 한글/한자는 2바이트로 인코딩합니다.
    // - 코드 영역 배치는 규칙적이어서 범위 검사/유효성 확인이 쉽습니다.
    //
    // 5) 정렬/순서
    // - 조합 규칙이 반영되어, 음절의 정렬 순서가 규칙적으로 나옵니다.
    // - CP949의 테이블 순서와 달라 사전식 비교 결과가 달라질 수 있습니다.
    //
    // 6) 호환/사용처
    // - Windows 레거시 환경 일부에서만 제한적으로 사용되었습니다.
    // - 웹/이메일 표준에서는 비권장이며 호환성 이슈가 잦습니다.
    //
    // 7) 자동 판별 난이도
    // - CP949/EUC-KR과 바이트 패턴이 겹치므로 탐지가 매우 어렵습니다.
    // - 메타데이터(코드페이지 지정) 없으면 오검출 가능성이 큽니다.
    //
    // 8) 유니코드 매핑
    // - 알고리즘적 매핑으로 대부분 무손실 변환이 가능합니다.
    // - 단, 구현 차이/미정의 영역은 대체문자(�)가 발생할 수 있습니다.
    //
    // 9) 파일 교환/보존
    // - 장기 보존과 교환에는 UTF-8을 권장합니다.
    // - JOHAB↔CP949 변환 시 자모/확장 문자 손실 가능성을 점검하세요.
    //
    // 10) C/C++ 처리 관점
    // - 내부는 UTF-8 유지, 입출력 경계에서 iconv/win32 API로 변환하세요.
    // - 유효성 검사 시 조합 규칙과 바이트 범위를 함께 확인하십시오.
    //
    // 11) 흔한 문제
    // - CP949로 열면 글자가 뒤섞이거나 물음표로 바뀌는 현상이 납니다.
    // - 폰트/렌더러가 자모 조합을 지원하지 않으면 깨져 보일 수 있습니다.
    //
    // 12) 요약 결론
    // - JOHAB(CP1361)은 조합 규칙 기반의 2바이트 한글 인코딩입니다.
    // - 범위/정렬은 규칙적이나, 생태계 지원이 약해 실무 채택은 드뭅니다.
    // - 현대 개발에서는 UTF-8 기본, 레거시는 명시적 변환을 권장합니다.

    // ---------- MacKorean ----------
    // UTF-8 <-> MacKorean
     bool utf8_to_mackorean(const std::string& utf8, std::string& out_mackor);
     bool mackorean_to_utf8(const std::string& mackor, std::string& out_utf8);
    // UTF-16 <-> MacKorean
     bool utf16_to_mackorean(const std::u16string& u16, std::string& out_mackor);
     bool mackorean_to_utf16(const std::string& mackor, std::u16string& out_u16);

    // MacKorean (Mac OS Korean) 요약 
    //
    // 1) 개요/성격
    // - MacKorean: 구형 Mac OS에서 사용된 한국어 문자 인코딩입니다.
    // - ISO/IEC 표준이 아닌 Apple 전용 코드페이지로 정의되었습니다.
    //
    // 2) 완성형 기반
    // - KS X 1001(완성형) 기반에 Mac 전용 추가 기호/문자를 포함합니다.
    // - CP949처럼 확장 범위는 넓지 않으며 Apple 환경에 한정되었습니다.
    //
    // 3) 바이트 구성
    // - 기본은 8비트 단일 바이트 코드페이지입니다.
    // - ASCII(0x00~0x7F) + 한글/한자/기호(0x80~0xFF) 배치 구조입니다.
    //
    // 4) 호환성
    // - Windows CP949/EUC-KR과 호환되지 않아 교환 시 깨짐이 발생합니다.
    // - Mac 환경 내부 호환은 가능했으나, 타 플랫폼과는 불안정했습니다.
    //
    // 5) 문자 범위
    // - 현대 한글 음절 일부 + 기본 한자/기호를 지원합니다.
    // - CP949의 확장 한글(1만+) 수준은 포함되지 않았습니다.
    //
    // 6) 사용처/역사
    // - Classic Mac OS(한글 시스템 6~9) 시절에 주로 사용되었습니다.
    // - Mac OS X 이후는 UTF-8/UTF-16으로 전환되어 사실상 사용 중단입니다.
    //
    // 7) 자동 판별 난이도
    // - 특정 코드 범위가 CP949/EUC-KR과 겹쳐 탐지가 매우 어렵습니다.
    // - "MacRoman" 계열과 혼동되기도 하여, 변환기 지정이 필요합니다.
    //
    // 8) 유니코드 매핑
    // - Apple에서 제공한 변환표를 사용해 Unicode 매핑이 가능합니다.
    // - 일부 기호는 표준 Unicode 매핑이 불명확할 수 있습니다.
    //
    // 9) 파일 교환/보존
    // - 장기 보존 시 UTF-8 변환을 강력히 권장합니다.
    // - MacKorean↔다른 코드페이지 변환 시 깨짐/손실을 반드시 점검하세요.
    //
    // 10) 요약 결론
    // - MacKorean은 구형 Mac OS 전용 한국어 인코딩입니다.
    // - 범위 제한, 호환성 문제로 현재는 거의 사용되지 않습니다.
    // - 현대 개발/보존 목적에서는 UTF-8이 필수 선택입니다.

    //---------------------------------------
    // 실패 시 예외(exception)를 던지는 타입.
    // NOTICE: 빈드시 try catch 구문으로 감싸서 사용하여야 함. 실패 시 std::runtime_error 예외 발생.
     std::u16string utf8_to_utf16_or_throw(const std::string& utf8);
     std::string    utf16_to_utf8_or_throw(const std::u16string& u16);

     std::u32string utf8_to_utf32_or_throw(const std::string& utf8);
     std::string    utf32_to_utf8_or_throw(const std::u32string& u32);

     std::u32string utf16_to_utf32_or_throw(const std::u16string& u16);
     std::u16string utf32_to_utf16_or_throw(const std::u32string& u32);

     std::string    utf8_to_cp949_or_throw(const std::string& utf8);
     std::string    cp949_to_utf8_or_throw(const std::string& cp949);

     std::string    utf16_to_cp949_or_throw(const std::u16string& u16);
     std::u16string cp949_to_utf16_or_throw(const std::string& cp949);

     std::string    utf8_to_iso2022kr_or_throw(const std::string& utf8);
     std::string    iso2022kr_to_utf8_or_throw(const std::string& iso2022kr);

     std::u16string iso2022kr_to_utf16_or_throw(const std::string& iso2022kr);
     std::string    utf16_to_iso2022kr_or_throw(const std::u16string& u16);

     std::string    utf8_to_johab_or_throw(const std::string& utf8);
     std::string    johab_to_utf8_or_throw(const std::string& johab);

     std::u16string johab_to_utf16_or_throw(const std::string& johab);
     std::string    utf16_to_johab_or_throw(const std::u16string& u16);

     std::string    utf8_to_mackorean_or_throw(const std::string& utf8);
     std::string    mackorean_to_utf8_or_throw(const std::string& mackor);

     std::u16string mackorean_to_utf16_or_throw(const std::string& mackor);
     std::string    utf16_to_mackorean_or_throw(const std::u16string& u16);

} //  
