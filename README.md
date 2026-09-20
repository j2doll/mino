# `mino` 

> :kr: **This project and the documentation were written in Korean.**

<p align="center"><picture><source media="(prefers-color-scheme: dark)"  srcset="docs/images/mino_dark.png"><source media="(prefers-color-scheme: light)" srcset="docs/images/mino_light.png"><img alt="mino" width="50%" src="docs/images/mino_light.png"></picture></p>

## 개요

### 프로젝트 개요
- **`mino`** 는 **`C++` 기반의 범용 라이브러리** 입니다. <sub> `C++17` 이상 </sub>
- 빠른 프로토타이핑과 재사용 가능한 컴포넌트 제공이 목표로 합니다.

<br />

### 아키텍처 요약
- 🏛️ [**core**](mino/core/include/mino/core)
   - 독립적인 코어 기능 모듈
- 📦 [**external**](mino/external/include/mino/external)
   - 외부 라이브러리를 사용하는 모듈
- 🔀 [**network**](mino/network/include/mino/network)
   - 네트워크 기능 모듈

<br />

### 예제
#### 💡 [example](example/) : 예제 루트 경로
##### 🏛️ [core](example/core) : 코어 라이브러리 예제
- 🏛️ [bit](example/core/bit/main.cpp)
   - 비트 단위 연산
- 🏛️ [broker](example/core/broker/main.cpp)
   - 인메모리+경량 메시지 브로커
- 🏛️ [cli](example/core/cli/main.cpp)
   - 명령행 인자 파서
- 🏛️ [config](example/core/config/main.cpp)
   - [설정 파일(`.config`)](example/core/config/app_config.conf) 읽기
- 🏛️ [container](example/core/container/main.cpp) : 표준 확장 컨테이너들
    - [bimap](example/core/container/test_bimap.cpp) : 양방향 맵
    - [binomial_heap](example/core/container/test_binomial_heap.cpp) : 이항 힙
    - [circular_buffer](example/core/container/test_circular_buffer.cpp) : 원형 버퍼
    - [concurrent_queue](example/core/container/test_concurrent_queue.cpp) : 동시성 큐
    - [devector](example/core/container/test_devector.cpp) : 양방향 벡터
    - [d_ary_heap](example/core/container/test_d_ary_heap.cpp) : `D`진(進) 힙
    - [fibonacci_heap](example/core/container/test_fibonacci_heap.cpp) : 피보나치 힙
    - [flat_map](example/core/container/test_flat_map.cpp) : 정렬 맵
    - [flat_multimap](example/core/container/test_flat_multimap.cpp) : 정령 다중키 맵
    - [flat_multiset](example/core/container/test_flat_multiset.cpp) : 정렬 다중키 셋
    - [flat_set](example/core/container/test_flat_set.cpp) : 정렬 셋
    - [multi_array](example/core/container/test_multi_array.cpp) : `N`차원 배열
    - [multi_index_container](example/core/container/test_multi_index_container.cpp) : 2중 인덱스 컨테이너
    - [pairing_heap](example/core/container/test_pairing_heap.cpp) : `N`진 트리 구조
    - [priority_queue](example/core/container/test_priority_queue.cpp) : 완전 이진 트리 큐
    - [skew_heap](example/core/container/test_skew_heap.cpp) : 비균형 이진 트리 힙
    - [small_vector](example/core/container/test_small_vector.cpp) : 고성능 연속 메모리 벡터
    - [stable_vector](example/core/container/test_stable_vector.cpp) : 참조 비무효 벡터
    - [static_vector](example/core/container/test_static_vector.cpp) : 스택 크기 고정 벡터
    - [topic_queue](example/core/container/test_topic_queue.cpp) : `1:N` 메시지 발행/구독 큐
    - [red_black_tree](example/core/container/test_red_black_tree.cpp) : 레드/블랙 노드 균형 트리
- 🏛️ [convert](example/core/convert/main.cpp)
    - 문자열 ↔ 숫자(정수,실수) 변환
- 🏛️ [crypt](example/core/crypt/main.cpp)
    - 암복호화 (키 사용/미사용 방식)
- 🏛️ [csv](example/core/csv/main.cpp)
    - `.csv` 파일 입출력 및 파싱. 엑셀용 `.csv` 파일 생성
- 🏛️ [daemon](example/core/daemon/main.cpp)
    - 상주형 데몬 
- 🏛️ `datetime` : 날짜·시간 처리 
    - [unit](example/core/datetime/unit/main.cpp) : 날자/시간 처리 단위 클래스
    - [util](example/core/datetime/util/main.cpp) : 포맷/파싱, `ISO` 표기, 타임존 보정 등
- 🏛️ [dispatch](example/core/dispatch/main.cpp)
    - 이벤트 디스패치
- 🏛️ [encoding](example/core/encoding/main.cpp)
    - `Base64` 인코딩/디코딩
- 🏛️ [enum](example/core/enum/main.cpp)
    - 열거(`enum`) ↔ 문자열 변환
- 🏛️ [expected](example/core/expected/main.cpp)
    - 성공값(`T`) 또는 에러값(`E`) 중 하나를 처리하는 패턴
- 🏛️ [exception](example/core/exception/main.cpp)
    - `C++17` 예외 처리 보완 기능 <sub> `std::source_location`, `std::stacktrace` 유사 기능 </sub>
- 🏛️ [file](example/core/file/main.cpp)
    - 실행 프로그램 경로/파일명 얻기. `UTF-8` 한글 경로.
    - 파일 정보. 파일 권한. 파일 크기. 파일 찾기.
- 🏛️ [findfile](example/core/findfile/main.cpp)
    - 파일에서 찾기 (`Find in files`)
- 🏛️ [hash](example/core/hash/main.cpp) 
    - `MD5`, `SHA-256`, `HMAC-SHA256`, `KDF(PBKDF2)`
- 🏛️ [ini](example/core/ini/main.cpp)
    - [`.ini`](example/core/ini/sample.ini) 파일 파서
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
    - 구조체의 직렬화/역직렬화 매크로.
- 🏛️ [resilience](example/core/resilience/main.cpp) : 복원력 패턴  
    - 재시도 전략, 지수 백오프, 서킷 브레이커.
- 🏛️ [result](example/core/result/main.cpp)
    - 타입 별 성공/실패 처리.
- 🏛️ `schedule`  
    - [task](example/core/schedule/task/main.cpp) : 단일/지연/주기 작업 등록·취소
    - [weekly](example/core/schedule/weekly/main.cpp) : 주 단위 반복 작업 스케줄
- 🏛️ [server](example/core/server/main.cpp) 
    - 서버를 위한 기본 구조
- 🏛️ [service](example/core/service/main.cpp) 
    - 서비스 등록, 시작/중지, 상태 확인
- 🏛️ [shared_memory](example/core/shared_memory/main.cpp) : 공유 메모리 `IPC` 
    - 메모리 매핑, 동기화(세마포어/뮤텍스), 데이터 일관성 관리 예.  
- 🏛️ [singleton](example/core/singleton/main.cpp) : 싱글톤 패턴  
- 🏛️ [string](example/core/string/main.cpp) : 문자열 유틸리티
    - [Trim](example/core/string/test_trim.cpp) : 문자열 정리
    - [Replace](example/core/string/test_replace.cpp) : 문자열 치환
    - [Case, Contains, Starts/Ends With](example/core/string/test_case_contains.cpp) : 대소문자 변환, 포함 여부, 접두/접미사 확인
    - [Split and Join](example/core/string/test_split_join.cpp) : 문자열 분리 및 합치기
    - [Whitespace / Newline Normalization](example/core/string/test_whitespace_normalization.cpp) : 공백 및 줄바꿈 정규화
    - [Padding / Repeat / Quotes / Indent](example/core/string/test_padding_quotes.cpp) : 채워넣기, 반복, 따옴표 처리, 들여쓰기
    - [Prefix/Suffix removal](example/core/string/test_affix_removal.cpp) : 접두어. 접미어 제거
    - [Safe Substr & Ellipsize](example/core/string/test_safe_substr_ellipsize.cpp) : 안전한 문자열 추출
    - [Parsing & Wildcard](example/core/string/test_parsing_wildcard.cpp) : 슷자 여부, 정수/실수 파싱, 와일드카드 패턴
    - [Korean numeric formatters](example/core/string/test_korean_numeric.cpp) : 한글 숫자로 변환
    - [tokenizer](example/core/string/test_tokenizer.cpp) : 다중 토큰 기반 문자열 분할
    - [to_string](example/core/string/test_to_string.cpp) : 실수 정밀도 적용 문자열 변환
    - [mutex_string](example/core/string/test_mutex_string.cpp) : 스레드 안전 문자열
    - [u8string](example/core/string/test_u8string.cpp) : `UTF-8` 문자열 처리
    - [encoding_function, to_console_encoding](example/core/string/test_encodings.cpp) : 한글 인코딩간 변환 <sub> (`UTF-8/16/32`, `wstring`, `CP949`/`EUC-KR`, `ISO-2022-KR`, `JOHAB`, `MacKorean`) </sub> , 콘솔 출력용 인코딩 변환
- 🏛️ [system](example/core/system/main.cpp) 
    - 환경변수, 경로 변환, 호스트/프로세스 정보 조회.
- 🏛️ [thread](example/core/thread/main.cpp)
    - 동적 스레드·동시성
- 🏛️ [toml](example/core/toml/main.cpp)
    - `.toml` 파싱·직렬화
- 🏛️ [tpm](example/core/tpm/main.cpp)
    - 인메모리 TP 모니터
- 🏛️ [uuid](example/core/uuid/main.cpp)
    - 고유 ID 생성·파싱  
- 🏛️ [validation](example/core/validation/main.cpp)
    - 이메일, 전화번호, `URL`, `IP`, `Base64`, `HEX` 색, `JSON`, 주민번호 검증
- 🏛️ [xml](example/core/xml/main.cpp)
    - `.xml` 파싱·직렬화
- 🏛️ [yaml](example/core/yaml/main.cpp)
    - `.yaml` 파싱·직렬화
- 🏛️ [zip](example/core/zip/main.cpp)
    - `deflate` `.zip` 압축
	
<br />

	
##### 📦 [external](example/external) : 외부 라이브러리 사용 예제
- 📦 `json`
    - [json](example/external/json/main.cpp) : `json` 확장 기능 <sub> 📄 `nlohmann::json` </sub>
    - [json2cpp.py](mino/external/include/mino/external/json/json2cpp.py) : 📄 `nlohmann::json` => `C++` 구조체 변환
- 📦 `log` : 로깅 어댑터/팩토리
    - [adapter](example/external/log/adapter/main.cpp) : 내부 로그 추상화층에 외부 로거 연결 <sub> ⚡ `spdlog` </sub>
    - [factory](example/external/log/factory/main.cpp) : 런타임 로거 구성 변경·팩토리 패턴 <sub> ⚡ `spdlog` </sub> 
    - [spd](example/external/log/spd/main.cpp) : 로깅 확장 기능 <sub> ⚡ `spdlog` </sub>
- 📦 `schedule` : `core` 스케줄러 확장
    - [weekly](example/external/schedule/weekly/main.cpp) : 주간 스케줄러 어댑터 `json` 확장 <sub> 📄 `nlohmann::json` </sub> 
- 📦 `xml` : `.xml` => `C++` 구조체 변환
    - [xml2cpp.py](mino/external/include/mino/external/xml/xml2cpp.py) : [`.xml`](example/external/xml/catalog.xml) => [`C++` 구조체](example/external/xml/catalog.hpp) 변환  
    - [example](example/external/xml/main.cpp) : 구조체 사용 예제 <sub> 🐶 `pugixml` </sub>

<br />

##### 🔀 [network](example/network) : 네트워크 관련 예제
- 🔀 멀티파트 <sub> (`multipart/mixed`,`multipart/form-data`) </sub> 다운로더
    - [httplib](example/network/multipart-downloader/httplib/main.cpp) <sub> 📡 `httplib` </sub> <sub> 🔒 `openssl` </sub> 
- 🔀 `ftp`/`sftp` 클라이언트
    - [tcp](example/network/ftp/tcp/main.cpp) <sub> `ssh` </sub>
- 🔀 네트워크 인터페이스 정보 조회
    - [interface](example/network/interface/main.cpp)
- 🔀 로깅 `hard/soft/hot reloading` 관리자 <sub> `udp` </sub>
   - [manager](example/network/log/manager/main.cpp)
   - 로깅 환경 파일 예제: [logger_manager_config.ini](example/network/log/manager/logger_manager_config.ini)
- 🔀 `memory_store` : 네트워크 기반 메모리 저장소
    - ```
           +------------+   tcp    +------------+
           |  server    |----------|   client   |
           +------------+          +------------+
                    <-- set key:value --
                    -- get key:value -->
      ```
        - [server](example/network/memory_store/server/main.cpp) <sub> `tcp` </sub>
        - [client](example/network/memory_store/client/main.cpp) <sub> `tcp` </sub>
- 🔀 `message_broker` : 분산 메시지 브로커
    - ```
                          +------------+
               +--------->|   broker   |----------+
               |          +------------+          |
            Publish                           Subscribe
             (tcp)                              (tcp)
               |                                  |
               |                                  v
       +---------------+                  +---------------+
       |      pub      |                  |      sub      |
       +---------------+                  +---------------+
      ```
        - [broker](example/network/message_broker/broker) <sub> `tcp` </sub>
        - [Publisher](example/network/message_broker/pub/main.cpp) <sub> `tcp` </sub>
        - [Subscriber](example/network/message_broker/sub/main.cpp) <sub> `tcp` </sub>
    - `python` 파이썬 `pub/sub` 
        - [pub](example/network/message_broker/python/pub/message_publisher.py)
        - [sub](example/network/message_broker/python/sub/message_subscriber.py)
    - 구조체 직렬화/역직렬화 
        - [pub-reflect](example/network/message_broker/pub-reflect/main.cpp) : [구조체](example/network/message_broker/reflect-sample.hpp) 직렬화 발행자 <sub> `core/reflect` `tcp` </sub>
        - [sub-reflect](example/network/message_broker/sub-reflect/main.cpp) :  [구조체](example/network/message_broker/reflect-sample.hpp) 역직렬화 구독자 <sub> `core/reflect` `tcp` </sub>
- 🔀 `MQTT` <sub> (`Message Queuing Telemetry Transport`) </sub>
    - ```
                            +-------------------+
                            |    MQTT Broker    |
                            +-------------------+
                              ^        ^      |
                      Publish |        |      | Publish
                              |        |      | (Matched Topic)
                              |        |      |
                              |     Subscribe |
                              |        |      |
                              |        |      v
                    +---------------+  |   +---------------+
                    |   Publisher   |  +---|  Subscriber   |
                    |     (pub)     |      |     (sub)     |
                    +---------------+      +---------------+
      			  
      ``` 
    - [`mqtt_broker.py`](example/network/mqtt/mqtt_broker.py) : `MQTT` 브로커 <sub> `python` </sub>
    - [`pub`](example/network/mqtt/pub/main.cpp) : `MQTT` 발행자 <sub> `tcp` </sub>
    - [`sub`](example/network/mqtt/sub/main.cpp) : `MQTT` 구독자 <sub> `tcp` </sub>
- 🔀 `redis` 클라이언트
    - [`redis`](example/network/redis/main.cpp) : `redis` 클라이언트 <sub> `tcp` `tls` </sub>
- 🔀 `REST API` 클라이언트
    - [httplib](example/network_curl/rest/httplib/main.cpp) : `GET`/`POST` 클라이언트 <sub> 📡 `httplib` </sub> <sub> 🔒 `openssl` </sub> <sub> 🔒 `openssl` </sub> 
- 🔀 `RPC`(`Remote Procedure Call`) 클라이언트/서버
    - ```
        +--------------+           +--------------+          +--------------+
        |    Server    |           |    broker    |          |    Client    |
        +-------+------+           +-------+------+          +-------+------+
                |                          |     Call RPC (tcp)      |
                |       Call RPC (tcp)     |<------------------------|
                |<-------------------------|                         |
              --+                          |                         |
             |  | (Self/Processing)        |                         |
             v--+     Return RPC (tcp)     |                         |
                |------------------------->|                         |
                |                          |    Return RPC (tcp)     |
                |                          |------------------------>|
      ```
        - [server](example/network/rpc/server/main.cpp) : `RPC` 서버 <sub> `tcp` </sub>
        - [client](example/network/rpc/client/main.cpp) : `RPC` 클라이언트 <sub> `tcp` </sub>
    - 공통 구조체 예제: [rpc_example_common.hpp](example/network/rpc/rpc_example_common.hpp)
- 🔀 `psftp` 연동 클라이언트
    - [sftp](example/network/sftp/putty/main.cpp)
- 🔀 `socket.io` 클라이언트 <sub> 🌐 `libcurl` </sub>
    - [socket-io](example/network/socket-io/main.cpp)
- 🔀 `ssh` 클라이언트 
    - [ssh](example/network/ssh/main.cpp)
- 🔀 `tcp` 소켓 예제
    - [server](example/network/tcp/server/main.cpp) : `tcp` 서버
    - [client](example/network/tcp/client/main.cpp) : `tcp` 클라이언트
- 🔀 `tls` 서버 및 클라이언트
    - [server](example/network/tls/server/main.cpp) : `tls` 서버
    - [client](example/network/tls/client/main.cpp) : `tls` 클라이언트
- 🔀 `udp` 소켓 예제
    - [receiver](example/network/udp/receiver/main.cpp) : `udp` 수신
    - [sender](example/network/udp/sender/main.cpp) : `udp` 송신
- 🔀 네트워크 인터페이스 목록 얻기, `IP` 주소 검증 
    - [util](example/network/util/main.cpp)

<br />

##### 🔀 [network_libssh2](example/network_libssh2) : `libssh2` 관련 예제    
- 🔀 `ssh` 클라이언트 <sub> 🔑 `libssh2` </sub> 
    - [ssh2](example/network_libssh2/ssh2/main.cpp)

<br />

##### 🔀 [network_curl](example/network_curl) : `libcurl` 관련 예제    
- 🔀 파일 다운로더 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>
    - [file-downloader](example/network_curl/file-downloader/main.cpp)
- 🔀 `ftp`/`sftp` 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>
    - [ftp](example/network_curl/ftp/main.cpp)  
- 🔀 멀티파트 <sub> (`multipart/mixed`,`multipart/form-data`) </sub> 다운로더 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub> 
    - [multipart-downloader](example/network_curl/multipart-downloader/main.cpp) 
- 🔀 `REST API` `GET`/`POST` 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub> 
    - [rest](example/network_curl/rest/main.cpp) 
- 🔀 웹소켓(`ws:`,`wss:`) 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>  
    - [ws](example/network_curl/ws/main.cpp)
        
<br />

##### 🧱 [template](example/template) : 템플릿 예제 프로젝트
- 🧱 [mino_all_example](example/template/mino_all_example/CMakeLists.txt) : 통합 예제 템플릿 <sub> `core` `network` `external` </sub>
- 🧱 [mino_core_example](example/template/mino_core_example/CMakeLists.txt) : `core` 템플릿
- 🧱 [mino_external_example](example/template/mino_external_example/CMakeLists.txt) : `external` 템플릿
- 🧱 [mino_network_example](example/template/mino_network_example/CMakeLists.txt) : `network` 템플릿

<br />

### 🏗️ 빌드 도구

#### ⊞ Windows 환경 🧩
- 🛠️ `Visual Studio` <sub> (2022 이상) </sub>
- 🔨 `cmake` <sub> (3.24 이상) </sub>
- 🥷 `ninja` <sub> (1.12.1 이상) </sub>
- 📦 `vcpkg` <sub> (2023.06 이상) </sub>
    - 사전에 환경변수 `VCPKG_ROOT`를 `vcpkg`가 설치된 경로로 설정
       - `VCPKG_ROOT`는 `PATH` 경로에 추가하여야 함 
    - :one: `Visual Studio` 인 경우
       - `vcpkg integrate install` 명령 실행
       - 또는 `Tools`/`Options`/`vcpkg Pacakage Manager`에 `VCPKG_ROOT` 경로 설정
    - :two: `VS Code`인 경우 : `.vscode`/`settings.json` 
    ```json
     {
         "cmake.configureSettings": {
           "CMAKE_TOOLCHAIN_FILE": "${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
         }
     }
    ```
    - 환경에 맞춰 `CMakeUserPresets.json`를 수정할 수 있음
    ```json
    {
      "name": "windows-vs-base",
      "hidden": true,
      "inherits": "base-common",
      "generator": "Visual Studio 17 2022",
      "architecture": {
        "value": "x64",
        "strategy": "external"
      },
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": {
          "type": "FILEPATH",
          "value": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        }
      },
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      }
    },
    {
      "name": "vs-debug",
      "displayName": "Windows VS 2022 (Debug)",
      "inherits": "windows-vs-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "vs-release",
      "displayName": "Windows VS 2022 (Release)",
      "inherits": "windows-vs-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    ``` 

<br />

#### 🐧 Linux 환경 
- 🦬 `gcc` <sub> (8.5 이상) </sub>
- 🔨 `cmake` <sub> (3.24 이상) </sub>
- 🥷 `ninja` <sub> (1.8.2 이상) </sub>
- `Linux`에서는 `vcpkg`는 사용하지 않는 것을 가정하였음
- `vscode` 사용 시 환경에 맞춰 `CMakeUserPresets.json`를 수정할 수 있음
```json
{
    "name": "linux-gcc-base",
    "hidden": true,
    "inherits": "base-common",
    "generator": "Ninja",
    "cacheVariables": {
    "CMAKE_C_COMPILER": "/opt/rh/gcc-toolset-15/root/usr/bin/gcc",
    "CMAKE_CXX_COMPILER": "/opt/rh/gcc-toolset-15/root/usr/bin/g++"
    },
    "condition": {
    "type": "equals",
    "lhs": "${hostSystemName}",
    "rhs": "Linux"
    }
},
{
    "name": "linux-gcc-debug",
    "displayName": "Linux GCC (Debug)",
    "inherits": "linux-gcc-base",
    "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug"
    }
},
{
    "name": "linux-gcc-release",
    "displayName": "Linux GCC (Release)",
    "inherits": "linux-gcc-base",
    "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Release"
    }
}
```

<br />

#### 🧩 외부 라이브러리 설치
- 🎩 `Redhat` 계열 (`Rocky`/`CentOS`/`AlmaLinux`)
```bash
# Rocky 8
sudo dnf install -y epel-release dnf-plugins-core
sudo dnf config-manager --set-enabled powertools

# Rocky 9
sudo dnf install -y epel-release
sudo dnf config-manager --set-enabled crb

# Tools & Compiler 
sudo dnf install -y gcc-c++ cmake make pkgconfig wget

# OpenSSL
sudo dnf install -y openssl-devel

# Brotli
sudo dnf install -y brotli-devel

# libssh2
sudo dnf install -y libssh2-devel

# CURL
sudo dnf swap -y libcurl-minimal libcurl
sudo dnf install -y libcurl-devel

``` 

- 🌀 `Debian` 계열 (`Ubuntu`/`Debian`)
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

# Brotli
sudo apt install -y libbrotli-dev

# CURL
sudo apt install -y libcurl4-openssl-dev

```

<br />

##### 📦 라이브러리 설치 
- 라이브러리 빌드 모드 설정 (`Debug`, `Release`)
- 라이브러리 경로 설정 (`C:\opt\mino`, `~/mino` 등)
###### :one: 🛠️ `Visual Studio` + 📦 `vcpkg` 환경
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

<br />

###### :two: 🐧 `Linux` 환경
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
- 설치 후 디렉토리 구조 확인
```
C:\opt>eza --tree mino
📁 mino/
 +- 📁 include/
 |   +- 📁 mino/
 |       +- 📁 core/
 |           +- 📁 xxx/
 |               +- 📄 *.hpp
 +- 📁 lib/
     +- 📁 cmake/
     +- 📄 mino_*.lib
 ```

```
$ eza --tree mino
📁 mino/
 +- 📁 include/
 |   +- 📁 mino/
 |       +- 📁 core/ 
 |           +- 📁 xxx/
 |               +- 📄 *.hpp
 +- 📁 lib/
     +- 📁 cmake/
     +- 📄 libmino_*.a
 ```

<br />

---

### ©️ 라이선스
- `MIT License`
   - 상세 내용 [LICENSE](LICENSE) 참고
- 📜 외부 라이브러리 
    - 📦 `external` 모듈
        - 📄 [nlohmann/json](https://github.com/nlohmann/json) : `MIT License`
        - ⚡ [spdlog](https://github.com/gabime/spdlog) : `MIT License`
        - 🐶 [pugixml](https://github.com/zeux/pugixml) : `MIT License`
        - 🗜️ [miniz-cpp](https://github.com/tfussell/miniz-cpp) : `MIT License`
    - 🔀 `network` 모듈
        - 📡 [cpp-httplib](https://github.com/yhirose/cpp-httplib) : `MIT License`
        - 🌐 [libcurl](https://curl.se/) : [`Curl License`](https://curl.se/docs/copyright.html)
        - 🔑 [libssh2](https://www.libssh2.org/) : `BSD-3 License`
        - 🔒 [openssl](https://www.openssl.org/) : `Apache License 2.0`
        - 🥖 [brotli](https://github.com/google/brotli) : `MIT License`


