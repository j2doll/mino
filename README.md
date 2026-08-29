# `mino` 

> :kr: This project and the documentation were written in Korean.

## 개요

### 프로젝트 개요
- `mino`는 `C++` 기반의 범용 라이브러리입니다.
- 빠른 프로토타이핑과 재사용 가능한 컴포넌트 제공을 목표로 합니다.

### 아키텍처 요약
- [**core**](mino/core/include/mino/core)
   — 코어 기능. 외부 종속성이 없는 모듈.
- [**external**](mino/external/include/mino/external)
   - 외부 라이브러리 종속성이 있는 모듈.
- [**network**](mino/network/include/mino/network)
   - 네트워크 기능 모듈.

### 예제 
- `example` — 예제 루트 경로
  - `core` — 코어 라이브러리 예제 
    - [bit](example/core/bit/main.cpp) — 비트 연산 및 비트맵 처리
        - `mino::core::bit::bit_array` 클래스의 주요 기능과 연산자를 검증하고 사용법을 보여주는 예제입니다.
        - **초기화 및 크기 관리**: 바이트 단위 자동 환산 초기화, `set_bits`/`set_bytes`, `clear` 동작
        - **비트 조작 및 연산**:
        - **슬라이싱 & 병합**: 바이트 경계에 구애받지 않는 임의 오프셋 추출(`get`) 및 병합(`merge`)
        - **연산자 오버로딩**: 비트열 결합(`+`), Bitwise NOT(`~`), 비트 시프트(`<<`, `>>`)
        - **변환 및 반전**: `std::vector<bool>` 변환(`to_array`), 비트 순서 역전(`reverser`)
        - **디버깅 출력**: 콘솔 포맷 출력(`print`), 메모리 16진수 덤프(`dump`)     
    - [broker](example/core/broker/main.cpp) — 로컬/경량 메시지 브로커
    - [config](example/core/config/main.cpp) — 설정 파일 로드/파싱 ([app_config.conf](example/core/config/app_config.conf) 환경 정보)
    - [container](example/core/container/main.cpp) — 컨테이너(컬렉션) 유틸리티
    - [convert](example/core/convert/main.cpp) — 데이터 형 변환 및 직렬화
    - [crypt](example/core/crypt/main.cpp) — 암호화/복호화
    - [csv](example/core/csv/main.cpp) — CSV 입출력 및 파싱 
    - [daemon](example/core/daemon/main.cpp) — 데몬/서비스화 관련 실행
    - `datetime` — 날짜/시간 유틸리티 예제
      - [unit](example/core/datetime/unit/main.cpp) — 날짜/시간 단위 테스트 및 유틸
      - [util](example/core/datetime/util/main.cpp) — 날짜/시간 관련 헬퍼 함수
    - [encoding](example/core/encoding/main.cpp) — 문자 인코딩/디코딩
    - [enum](example/core/enum/main.cpp) — 열거형 사용
    - [expected](example/core/expected/main.cpp) — 예상 값/검증 관련
    - [file](example/core/file/main.cpp) — 파일 입출력 및 파일 관련 유틸
    - [hash](example/core/hash/main.cpp) — 해시 함수 및 체크섬
    - [ini](example/core/ini/main.cpp) — INI 파일 파싱 ([sample.ini](example/core/ini/sample.ini) 환경 정보)
    - [json](example/core/json/main.cpp) — JSON 직렬화/역직렬화
    - [log](example/core/log/main.cpp) — 로깅 사용
    - [macro](example/core/macro/main.cpp) — 매크로 및 템플릿 메타 프로그래밍
    - [memory](example/core/memory/main.cpp) — 메모리 관리·유틸
    - [notification](example/core/notification/main.cpp) — 단일 알림 관련
    - [notifications](example/core/notifications/main.cpp) — 복합 알림·구독
    - [overload](example/core/overload/main.cpp) — 함수/연산자 오버로드
    - [pfr](example/core/pfr/main.cpp) — PFR(플랫 리플렉션) 관련
    - [process_util](example/core/process_util/main.cpp) — 프로세스 유틸리티
    - [reflect](example/core/reflect/main.cpp) — 리플렉션/메타프로그래밍
    - [resilience](example/core/resilience/main.cpp) — 오류 복원력(예: 재시도)
    - [result](example/core/result/main.cpp) — 결과 반환/에러 핸들링 관례
    - `schedule` — 스케줄링 예제
      - [task](example/core/schedule/task/main.cpp) — 작업 스케줄
      - [weekly](example/core/schedule/weekly/main.cpp) — 주단위 스케줄
    - [server](example/core/server/main.cpp) — 간단한 서버
    - [service](example/core/service/main.cpp) — 서비스 레이어 샘플
    - [shared_memory](example/core/shared_memory/main.cpp) — 공유 메모리 기반 통신
    - [singleton](example/core/singleton/main.cpp) — 싱글톤 패턴
    - [string](example/core/string/main.cpp) — 문자열 처리 유틸
    - [system](example/core/system/main.cpp) — 시스템 유틸리티
    - [thread](example/core/thread/main.cpp) — 스레드/동시성
    - [tpm](example/core/tpm/main.cpp) — TPM(보안 모듈) 관련
    - [uuid](example/core/uuid/main.cpp) — UUID 생성/파싱
    - [validation](example/core/validation/main.cpp) — 입력/모델 검증
    - [xml](example/core/xml/main.cpp) — XML 처리
    - [yaml](example/core/yaml/main.cpp) — YAML 파싱/직렬화
  - `external` — 외부 라이브러리 사용
    - [json](example/external/json/main.cpp) — 놀먼 json 확장 연동
    - `log` — 외부 로깅 어댑터/팩토리 
      - [adapter](example/external/log/adapter/main.cpp) — 로그 어댑터 샘플
      - [factory](example/external/log/factory/main.cpp) — 동적 로깅 설정 변경 
      - [spd](example/external/log/spd/main.cpp) — spdlog 확장 연동
    - `schedule` — 외부 스케줄러 연동 예제
      - [weekly](example/external/schedule/weekly/main.cpp) — 주간 스케줄러의 놀먼 json 확장
  - `network` — 네트워크 관련
    - `download` — 다운로드 클라이언트 구현 
      - [curl](example/network/download/curl/main.cpp) — libcurl 기반 다운로드
      - [httplib](example/network/download/httplib/main.cpp) — httplib 기반 다운로드
    - `ftp` — FTP 연동 예제
      - [curl](example/network/ftp/curl/main.cpp) — curl 기반 FTP
      - [tcp](example/network/ftp/tcp/main.cpp) — TCP 기반 FTP
    - [interface](example/network/interface/main.cpp) — 네트워크 인터페이스/추상화
    - `log` — 네트워크 로깅 관련 예제
      - [manager](example/network/log/manager/main.cpp) — 로그 전송/관리자([logger_manager_config.ini](example/network/log/manager/logger_manager_config.ini) 환경 정보)
    - `memory_store` — 네트워크 기반 메모리 저장소
      - [client](example/network/memory_store/client/main.cpp) — 저장소 클라이언트
      - [server](example/network/memory_store/server/main.cpp) — 저장소 서버
    - `message_broker` — 메시지 브로커(분산 메시징)
      - [broker](example/network/message_broker/broker/main.cpp) — 브로커 구현
      - [pub](example/network/message_broker/pub/main.cpp) — 퍼블리셔
      - `python` — 파이썬 연동 예제
        - [pub](example/network/message_broker/python/pub/main.cpp) — 파이썬 퍼블리셔
        - [sub](example/network/message_broker/python/sub/main.cpp) — 파이썬 구독자
      - [sub](example/network/message_broker/sub/main.cpp) — 구독자 예제
    - `rest` — REST 클라이언트/서버 예제
      - [curl](example/network/rest/curl/main.cpp) — curl 기반 REST 클라이언트 예제
      - [httplib](example/network/rest/httplib/main.cpp) — httplib 기반 REST 예제
    - `rpc` — RPC 클라이언트/서버 예제
      - [client](example/network/rpc/client/main.cpp) — 클라이언트 예제
      - [server](example/network/rpc/server/main.cpp) — 서버 예제
      - [rpc_example_common.hpp](example/network/rpc/rpc_example_common.hpp) — RPC 공통 헤더
    - `sftp` — SFTP 연동 예제
      - [putty](example/network/sftp/putty/main.cpp) — psftp 연동 예제
    - `tcp` — TCP 소켓 예제
      - [client](example/network/tcp/client/main.cpp) — TCP 클라이언트 예제
      - [server](example/network/tcp/server/main.cpp) — TCP 서버 예제
    - `udp` — UDP 통신 예제
      - [receiver](example/network/udp/receiver/main.cpp) — UDP 수신 예제
      - [sender](example/network/udp/sender/main.cpp) — UDP 전송 예제
    - [util](example/network/util/main.cpp) — 네트워크 유틸리티/헬퍼
  - `template` — 예제 템플릿 프로젝트
    - [mino_all_example](example/template/mino_all_example/main.cpp) — 전체 기능을 조합한 통합 예제
    - [mino_core_example](example/template/mino_core_example/main.cpp) — 코어 기능 중심 예제
    - [mino_external_example](example/template/mino_external_example/main.cpp) — 외부 라이브러리 연동 예제
    - [mino_network_example](example/template/mino_network_example/main.cpp) — 네트워크 기능 중심 예제

<br />

#### 예제 사용 방법
- 각 서브폴더의 `CMakeLists.txt`와 `main.cpp`를 확인해 빌드·실행 방법을 참고하세요.
- 샘플 설정 파일은 `config`, `ini` 등의 파일를 확인하세요.
- 특정 예제를 실행하려면 해당 폴더로 이동해 CMake 빌드 후 실행하면 됩니다.

<br />

---

### 🏗️ 빌드 도구

#### ⊞ Windows 환경 🧩

- `Visual Studio` (2022 이상)
- `cmake` (3.30 이상)
- `ninja` (1.12.1 이상)
- `vcpkg` (2023.06 이상)
    - `Visual Studio` : `vcpkg integrate install` 명령 실행
    - `VS Code` : `settings.json` 
    ```json
    {
        "cmake.configureSettings": {
          "CMAKE_TOOLCHAIN_FILE": "${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        }
    }
    ```
    - `CMakeSettings.json` 설정 (MSVC 전용 설정 파일)
    ```json
     {
        // ...
        "variables": [
        {
            "name": "CMAKE_TOOLCHAIN_FILE",
            "value": "${env.VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
            "type": "FILEPATH"
        }
     }
    ```

#### 🐧 Linux 환경 🧩

- `gcc` (8.5 이상)
- `cmake` (3.26 이상) 
- `ninja` (1.8.2 이상)

#### 외부 라이브러리 설치

- `Redhat` 계열 (`Rocky`/`CentOS`/`AlmaLinux`)
```bash
# Rocky 8
sudo dnf install -y epel-release dnf-plugins-core
sudo dnf config-manager --set-enabled powertools

# Tools & Compiler 
sudo dnf install -y gcc-c++ cmake make pkgconfig

# OpenSSL
sudo dnf install -y openssl-devel

# CURL
sudo dnf swap -y libcurl-minimal libcurl
sudo dnf install -y libcurl-devel

# Brotli
sudo dnf install -y brotli-devel

# libssh2
sudo dnf install -y libssh2-devel
    
# Build library
rm -rf build

cmake -S . -B build -G "Ninja" \
 -DCMAKE_CXX_STANDARD=17 \
 -DCMAKE_BUILD_TYPE=Debug

cmake --build build -j

``` 

- `Debian` 계열 (`Ubuntu`/`Debian`)
```bash
# Ubuntu 22.04 LTS
sudo add-apt-repository universe
sudo apt update

# Tools & Compiler
sudo apt install -y build-essential cmake pkg-config ca-certificates

# OpenSSL
sudo apt install -y openssl

# libssh2
sudo apt install -y libssl-dev libssh2-1-dev

# CURL
sudo apt install -y libcurl4-openssl-dev

# Brotli
sudo apt install -y libbrotli-dev

```

##### 📦 라이브러리 설치 

- 라이브러리 빌드 모드 설정 (`Debug`, `Release`)
- 라이브러리 경로 설정 (`C:\opt\mino` 등)

###### (1) `Visual Studio` + `vcpkg` 환경

```bat
::::::::::::::::::::::::::::::::::::::::::::::::::
:: 기존 작업 경로 삭제 (Windows)
rmdir /s /q build

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (vcpkg 사용 시) (Debug)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="C:\opt\mino" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
:: -DCMAKE_CXX_STANDARD=17 를 이용하여 C++ 표준 버전 설정 가능.
:: %VCPKG_ROOT% 는 환경설정 정보로 vcpkg.exe가 있는 경로.

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (vcpkg 사용 시) (Release)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:\opt\mino" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (Windows) (vcpkg 미사용) (Release)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:\opt\mino"

::::::::::::::::::::::::::::::::::::::::::::::::::
:: 빌드
cmake --build build -j

::::::::::::::::::::::::::::::::::::::::::::::::::
:: 설치
cmake --install build
```

###### (2) `Linux` 환경
 
```bash
#############################################
# 작업 경로 삭제 (Linux)
rm -rf build

#############################################
# cmake 설정 (Linux)
cmake -B build -S . -G "Ninja" -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="~/mino"
# -DCMAKE_CXX_STANDARD=17 를 이용하여 C++ 표준 버전 설정 가능

#############################################
# 빌드
cmake --build build -j

#############################################
# 빌드 (코어 갯수 한정. $(nproc) 대신 숫자로 정의)
cmake --build build -j "$(nproc)"

#############################################
# 설치
cmake --install build
```

- 🚀 설치 후 디렉토리 구조 확인
 
```
C:\opt>eza --tree mino
mino
├── include
│   └── mino
│       └── xxx
│           └── xxx.hpp
└── lib
    ├── cmake
    │   └── mino
    │       └── *.cmake
    └── mino_core.lib
```

```
$ eza --tree mino
mino
├── include
│   └── mino
│       └── xxx
│           └── xxx.hpp
└── lib
    ├── cmake
    │   └── mino
    │       └── *.cmake
    └── libmino_core.a
```

<br />

---

### 라이선스
- MIT License
   - 상세 내용은 [LICENSE](LICENSE) 참고

#### 외부 라이브러리
##### 소스 포함 
- [nlohmann/json](mino/external/include/mino/external/third-party/nolohmann) : MIT License
- [spdlog](mino/external/include/mino/external/third-party/spdlog) : MIT License

##### 외부 사용
- [libcurl](https://curl.se/) : [Curl License](https://curl.se/docs/copyright.html)
- [libssh2](https://www.libssh2.org/) : BSD-3 License
- [openssl](https://www.openssl.org/) : Apache License 2.0
- [brotli](https://github.com/google/brotli) : MIT License
