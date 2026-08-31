# `mino` 

> :kr: **This project and the documentation were written in Korean.**

<p align="center"><picture><img src="docs/images/mino_logo.svg" width="50%" alt="" /></picture></p>

## 개요

### 프로젝트 개요
- `mino`는 `C++` 기반의 범용 라이브러리 입니다.
- 빠른 프로토타이핑과 재사용 가능한 컴포넌트 제공이 목표입니다.

### 아키텍처 요약
- 🏛️ [**core**](mino/core/include/mino/core)
   - 코어 기능. 외부 종속성이 없는 모듈.
- 📦 [**external**](mino/external/include/mino/external)
   - 외부 라이브러리 종속성이 있는 모듈.
- 🔀 [**network**](mino/network/include/mino/network)
   - 네트워크 기능 모듈.

### 예제
#### 💡 [example](example/) <sub> 예제 루트 경로 </sub>
##### 🏛️ [core](example/core) <sub> 코어 라이브러리 예제 </sub>
- 🏛️ [bit](example/core/bit/main.cpp)
   - 비트 단위 연산
- 🏛️ [broker](example/core/broker/main.cpp)
   - 인메모리+경량 메시지 브로커
- 🏛️ [config](example/core/config/main.cpp)
   - [설정 파일(`.config`)](example/core/config/app_config.conf) 읽기 기능
- 🏛️ [container](example/core/container/main.cpp) 
    - `std::`의 `container`를 확장한 컨테이너들
    - <sub> [bimap](mino/core/include/mino/core/container/bimap.hpp), binomial_heap, circular_buffer, concurrent_queue, container, devector, d_ary_heap, fibonacci_heap, flat_map, flat_multimap, flat_multiset, flat_set, multi_array, multi_index_container, pairing_heap, priority_queue, skew_heap, small_vector, stable_vector, static_vector, topic_queue </sub>
- 🏛️ [convert](example/core/convert/main.cpp)
    - 문자열 ↔ 숫자(정수,실수) 변환
- 🏛️ [crypt](example/core/crypt/main.cpp)
    - 암복호화(키 사용, 키 미사용)
- 🏛️ [csv](example/core/csv/main.cpp)
    - `.csv` 파일 입출력 및 파싱
- 🏛️ [daemon](example/core/daemon/main.cpp)
    - 상주형 데몬 예제.
- `datetime` : 날짜·시간 처리 
    - 🏛️ [unit](example/core/datetime/unit/main.cpp) : 날자/시간 처리 단위 클래스
    - 🏛️ [util](example/core/datetime/util/main.cpp) : 포맷/파싱, ISO 표기, 타임존 보정 등
- 🏛️ [encoding](example/core/encoding/main.cpp)
    - 한글 문자 인코딩·디코딩
    - UTF-8/16/32 변환, BOM 처리, 로케일별 콘솔 출력 예시로 다국어 대응.  
- 🏛️ [enum](example/core/enum/main.cpp)
    - 열거(`enum`) ↔ 문자열 변환
- 🏛️ [expected](example/core/expected/main.cpp)
    - 성공값(`T`) 또는 에러값(`E`) 중 하나를 처리하는 패턴
- 🏛️ [file](example/core/file/main.cpp)
    - 실행 프로그램 경로/파일명 얻기. UTF-8 한글 경로.
    - 파일 정보. 파일 권한. 파일 크기. 파일 찾기.
- 🏛️ [hash](example/core/hash/main.cpp) 
    - MD5/SHA/CRC 등 해시.
- 🏛️ [ini](example/core/ini/main.cpp)
    - `.ini` 파일 파서  
- 🏛️ [json](example/core/json/main.cpp)
    - `.json` 직렬화/역직렬화  
- 🏛️ [log](example/core/log/main.cpp)
    - 콘솔 로깅 싱크, 파일 로깅 싱크, 싱크 통합 로거
- 🏛️ [macro](example/core/macro/main.cpp)
    - 함수 시도(`TRY_OPT`) 계열 매크로
- 🏛️ [memory](example/core/memory/main.cpp)
    - 메모리 직렬화/역직렬화. 딥 카피.
- 🏛️ [notification](example/core/notification/main.cpp)
    - 옵저버 등록 및 경고 알림.
- 🏛️ [notifications](example/core/notifications/main.cpp)
    - 이벤트 등록 및 알림/해제. 
- 🏛️ [overload](example/core/overload/main.cpp)
    - 다중 타입 처리.
- 🏛️ [pfr](example/core/pfr/main.cpp) : 플랫 리플렉션(PFR)
    - 구조체 필드의 값 및 타입 접근. 
- 🏛️ [process_util](example/core/process_util/main.cpp)
    - 프로세스 목록 및 정보 얻기.
- 🏛️ [reflect](example/core/reflect/main.cpp)
    - 구조체 직렬화/역직렬화 매크로.
- 🏛️ [resilience](example/core/resilience/main.cpp) : 복원력 패턴  
    - 재시도 전략, 지수 백오프, 서킷 브레이커.
- 🏛️ [result](example/core/result/main.cpp)
    - 타입 별 성공/실패 처리.
- 🏛️ `schedule`  
    - [task](example/core/schedule/task/main.cpp) : 단일/지연/주기 작업 등록·취소
    - [weekly](example/core/schedule/weekly/main.cpp) : 주단위 반복 작업 스케줄
- 🏛️ [server](example/core/server/main.cpp) 
    - 서버를 위한 기본 구조
- 🏛️ [service](example/core/service/main.cpp) 
    - 서비스 등록, 시작/중지, 상태 확인
- 🏛️ [shared_memory](example/core/shared_memory/main.cpp) : 공유 메모리 IPC  
    - 메모리 매핑, 동기화(세마포어/뮤텍스), 데이터 일관성 관리 예.  
- 🏛️ [singleton](example/core/singleton/main.cpp) : 싱글톤 패턴  
- 🏛️ [string](example/core/string/main.cpp) : 문자열 유틸리티
    - <sub> Trim, Replace, Case, Contains, Starts/Ends With, Split and Join, Whitespace / Newline Normalization, Padding / Repeat / Quotes / Indent, Prefix/Suffix removal, Safe Substr & Ellipsize, Parsing & Wildcard, Korean numeric formatters, tokenizer, to_string, mutex_string, u8string, encoding_function, to_console_encoding </sub>
- 🏛️ [system](example/core/system/main.cpp)
    - 환경변수, 경로 변환, 호스트/프로세스 정보 조회.  
- 🏛️ [thread](example/core/thread/main.cpp) : 동적 스레드·동시성  
- 🏛️ [tpm](example/core/tpm/main.cpp) : 인메모리 TP 모니터
- 🏛️ [uuid](example/core/uuid/main.cpp) : 고유 ID 생성·파싱  
- 🏛️ [validation](example/core/validation/main.cpp) : 필드 검증
- 🏛️ [xml](example/core/xml/main.cpp) : `.xml` 파싱·직렬화
- 🏛️ [yaml](example/core/yaml/main.cpp) : `.yaml` 파싱·직렬화
##### 📦 [external](example/external) <sub> 외부 라이브러리 사용 예제 </sub>
- 📦 [json](example/external/json/main.cpp) : `nlohmann::json` 확장 기능.
- `log` : 외부 로깅 어댑터/팩토리
    - 📦 [adapter](example/external/log/adapter/main.cpp) : 내부 로그 추상화층에 외부 로거 연결.  
    - 📦 [factory](example/external/log/factory/main.cpp) : 런타임 로거 구성 변경·팩토리 패턴.  
    - 📦 [spd](example/external/log/spd/main.cpp) : `spdlog` 확장 로깅 기능.
- `schedule` : 외부 스케줄러 연동
    - 📦 [weekly](example/external/schedule/weekly/main.cpp) : 주간 스케줄러 어댑터 `nlohmann::json` 확장.
##### 🔀 [network](example/network) <sub> 네트워크 관련 예제 </sub>
- `download` : http 다운로드 클라이언트
    - 🔀 [curl](example/network/download/curl/main.cpp) : libcurl 기반 다운로드
    - 🔀 [httplib](example/network/download/httplib/main.cpp) : httplib 기반 다운로드
- `ftp` : FTP 클라이언트
    - 🔀 [curl](example/network/ftp/curl/main.cpp) : libcurl 기반 FTP 클라이언트.
    - 🔀 [tcp](example/network/ftp/tcp/main.cpp) : TCP 소켓 기반 FTP 클라이언트.
- 🔀 [interface](example/network/interface/main.cpp) : 네트워크 인터페이스 정보 조회.
- `log` : 핫/소프트 로깅 환경 정보 리로딩 기능.
    - 🔀 [manager](example/network/log/manager/main.cpp)
        - 환경 파일 예제: [logger_manager_config.ini](example/network/log/manager/logger_manager_config.ini)
- `memory_store` : 네트워크 기반 정보(메모리) 저장소
    - 🔀 [server](example/network/memory_store/server/main.cpp) : 서버.
    - 🔀 [client](example/network/memory_store/client/main.cpp) : 클라이언트.
- `message_broker` : 분산 메시지 브로커  
    - 🔀 [broker](example/network/message_broker/broker) : 브로커 예제.
    - 🔀 [pub](example/network/message_broker/pub/main.cpp) : 발행자(`Publisher`) 예제.
    - 🔀 [sub](example/network/message_broker/sub/main.cpp) : 구독자(`Subscriber`) 예제.
    - 🔀 [python](example/network/message_broker/python) : 파이썬 `pub/sub` 예제.
- `rest` : `REST API` 클라이언트  
    - 🔀 [curl](example/network/rest/curl/main.cpp) : libcurl 기반 REST 클라이언트.
    - 🔀 [httplib](example/network/rest/httplib/main.cpp) : httplib 기반 REST 클라이언트.
- `rpc` : `RPC` 클라이언트/서버
    - 🔀 [server](example/network/rpc/server/main.cpp) : `RPC` 서버.
    - 🔀 [client](example/network/rpc/client/main.cpp) : `RPC` 클라이언트.
    - 공통 구조체 예제: [rpc_example_common.hpp](example/network/rpc/rpc_example_common.hpp)
- `sftp` : SFTP 클라이언트
    - 🔀 [putty](example/network/sftp/putty/main.cpp) — `psftp` 연동 클라이언트.  
- `tcp` — TCP 소켓 예제
    - 🔀 [server](example/network/tcp/server/main.cpp) — TCP 서버.
    - 🔀 [client](example/network/tcp/client/main.cpp) — TCP 클라이언트.
- `udp` — UDP 통신 예제  
    - 🔀 [receiver](example/network/udp/receiver/main.cpp) — UDP 수신.
    - 🔀 [sender](example/network/udp/sender/main.cpp) — UDP 송신.
- 🔀 [util](example/network/util/main.cpp)
    - 주소 변환, 타임아웃 헬퍼, 재시도/회복 패턴 유틸.  
##### 🧱 [template](example/template) <sub> 템플릿 예제 프로젝트 </sub> 
- 🧱 [mino_all_example](example/template/mino_all_example/CMakeLists.txt) : 통합 예제 템플릿.
- 🧱 [mino_core_example](example/template/mino_core_example/CMakeLists.txt) : 코어 기능 예제 템플릿.
- 🧱 [mino_external_example](example/template/mino_external_example/CMakeLists.txt) : 외부 기능 예제 템플릿.
- 🧱 [mino_network_example](example/template/mino_network_example/CMakeLists.txt) : 네트워크 기능 예제 템플릿.

### 🏗️ 빌드 도구
#### ⊞ Windows 환경 🧩
- `Visual Studio` (2022 이상)
- `cmake` (3.24 이상)
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
    - `CMakeSettings.json` 설정 (`MSVC` 전용 설정 파일)
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
#### 🐧 Linux 환경 
- `gcc` (8.5 이상)
- `cmake` (3.24 이상)
- `ninja` (1.8.2 이상)

#### 🧩 외부 라이브러리 설치
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

- 🏛️ 설치 후 디렉토리 구조 확인
```
C:\opt>eza --tree mino
📁 mino/
 +- 📁 include/
 |   +- 📁 mino/
 |       +- 📁 xxx/
 |           +- 📄 xxx.hpp
 +- 📁 lib/
 |   +- 📁 cmake/
 |   +- 📄 libmino_*.lib
 +- 📁 use-cmake/
```

```
$ eza --tree mino
📁 mino/
 +- 📁 include/
 |   +- 📁 mino/
 |       +- 📁 xxx/
 |           +- 📄 xxx.hpp
 +- 📁 lib/
 |   +- 📁 cmake/
 |   +- 📄 libmino_*.a
 +- 📁  use-cmake/
```

<br />

---

### ©️ 라이선스
- MIT License
   - 상세 내용 [LICENSE](LICENSE) 참고
#### 📜 외부 라이브러리 라이선스
- 📦 `external` 모듈
    - [nlohmann/json](mino/external/include/mino/external/third-party/nolohmann) : MIT License
    - [spdlog](mino/external/include/mino/external/third-party/spdlog) : MIT License
- 🔀 `network` 모듈
    - [openssl](https://www.openssl.org/) : Apache License 2.0
    - [libcurl](https://curl.se/) : [Curl License](https://curl.se/docs/copyright.html)
    - [brotli](https://github.com/google/brotli) : MIT License- 
    - [libssh2](https://www.libssh2.org/) : BSD-3 License


