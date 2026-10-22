# `mino` 

> :kr: **This project and the documentation were written in Korean.**

<p align="center"><picture><source media="(prefers-color-scheme: dark)"  srcset="docs/images/mino_dark.png"><source media="(prefers-color-scheme: light)" srcset="docs/images/mino_light.png"><img alt="mino" width="50%" src="docs/images/mino_light.png"></picture></p>

## 개요

### 프로젝트 개요
- **`mino`** 는 **`C++` 기반의 범용 라이브러리** 입니다. <sub> `C++17` 이상 </sub>
- 빠른 프로토타이핑과 재사용 가능한 컴포넌트 제공이 목표로 합니다.

<br />

### 아키텍처 요약

```
               +----------+
               |   core   |
               +----------+
                     |
          +----------+----------+
          |                     |
    +----------+          +----------+
    | external |          | network  |
    +----------+          +----------+

```

- 🏛️ [**core**](mino/core/include/mino/core)
   - 핵심 기능 모듈 <sub> 독립형 </sub>
- 📦 [**external**](mino/external/include/mino/external)
   - 외부 라이브러리 사용 모듈 <sub> 외부 종속, `core` 사용 </sub> 
- 🔀 [**network**](mino/network)
   - 네트워크 모듈 <sub> 하위 모듈로 분류, `core` 사용 </sub>

<br />

### 예제
#### 💡 [example](example/) <sub> 예제 루트 경로 </sub> 
##### 🏛️ [core](example/core) <sub> 독립형 핵심 기능 </sub>
- 🏛️ [bit](example/core/bit/main.cpp)
   - 비트 단위 연산
- 🏛️ [broker](example/core/broker/main.cpp)
   - 싱글톤 기반 인-메모리 서비스 중개자(`Service Locator/Registry`)
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
    - [multi_index_container](example/core/container/test_multi_index_container.cpp) : `N`중 인덱스 컨테이너
    - [pairing_heap](example/core/container/test_pairing_heap.cpp) : `N`진 트리 구조
    - [priority_queue](example/core/container/test_priority_queue.cpp) : 완전 이진 트리 큐
    - [skew_heap](example/core/container/test_skew_heap.cpp) : 비균형 이진 트리 힙
    - [small_vector](example/core/container/test_small_vector.cpp) : 고성능 연속 메모리 벡터
    - [stable_vector](example/core/container/test_stable_vector.cpp) : 참조 비무효 벡터
    - [static_vector](example/core/container/test_static_vector.cpp) : 스택 크기 고정 벡터
    - [topic_queue](example/core/container/test_topic_queue.cpp) : `1:N` 메시지 발행/구독 큐
    - [red_black_tree](example/core/container/test_red_black_tree.cpp) : `red`/`black` 노드 균형 트리
    - [span](example/core/container/test_span.cpp) : 소유권이 없는(`non-owning`) 뷰(`view`) 
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
    - `util`
       - [conversion_functions](example/core/datetime/util/test_conversion_functions.cpp) : 시간 단위 변환 <sub> `std::tm` <-> `time_t` <-> `time_point` </sub>
       - [formatting_functions](example/core/datetime/util/test_formatting_functions.cpp) : 문자열 포맷팅
       - [parsing_functions](example/core/datetime/util/test_parsing_functions.cpp) : 시간 문자열 파싱 <sub> `Strict`, `ISO-8601`, `RFC-3339` </sub>
       - [current_time_functions](example/core/datetime/util/test_current_time_functions.cpp) : 현재 시간 문자열 생성
- 🏛️ `dispatch`
    - [dynamic_event_dispatcher](example/core/dispatch/test_dynamic_event_dispatcher.cpp) : <sub> 동적 힙 메모리(`string`,`vector`)와 참조 횟수 기반 자원 관리(`shared_ptr`)를 포함하는 비-`POD` 구조체 </sub>
    - [any_event_dispatcher](example/core/dispatch/test_any_event_dispatcher.cpp) : <sub> 동적 바이너리 페이로드(`vector<uint8_t>`) 포함 </sub>
    - [static_dispatcher](example/core/dispatch/test_static_dispatcher.cpp) : <sub> 힙 할당이 전혀 발생하지 않는 스택 기반 데이터 구조 </sub>
    - [collecting_event_dispatcher](example/core/dispatch/test_collecting_event_dispatcher.cpp) : <sub> 다중 보안 검증 파이프라인에서 핸들러들의 개별 검증 결과(`bool`)를 취합하기 위한 요청 데이터 </sub>
- 🏛️ `encoding`
    - `Base64` 인코딩/디코딩
        - [base64_encoding_decoding](example/core/encoding/test_base64_enc_dec.cpp) : 기본 문자열 인코딩 / 디코딩 테스트
        - [rfc_4648_encoding_decoding](example/core/encoding/test_rfc_4648.cpp) : RFC 4648 표준 테스트 벡터 및 패딩
        - [base64_error](example/core/encoding/test_base64_error.cpp) : 잘못된 형식의 Base64 입력 시 예외/에러 처리 검증
        - [base64_binary](example/core/encoding/test_base64_binary.cpp) : Base64 바이너리 데이터 테스트
    - `Hex`
        - [hex](example/core/encoding/test_hex.cpp) : Hex 상호 변환 및 대소문자 검증
        - [hex_error](example/core/encoding/test_hex_error.cpp) : Hex 비정상 입력 에러 처리 검증
    - `URL` 인코딩/디코딩
        - [url_encoding](example/core/encoding/test_url_encoding.cpp) : URL 인코딩/디코딩 검증(RFC 3986 및 공백 처리)
        - [url_korean](example/core/encoding/test_url_korean.cpp) : UTF-8 한글 문자열 URL 인코딩/디코딩 통합 검증
        - [test_invalid_url](example/core/encoding/test_invalid_url.cpp) : URL 디코딩 비정상 입력 에러 처리 검증
- 🏛️ [enum](example/core/enum/main.cpp)
    - 열거(`enum`) ↔ 문자열 변환
- 🏛️ [expected](example/core/expected/main.cpp)
    - 성공값/기대값(`T`) 또는 에러값(`E`) 중 하나를 처리하는 패턴
- 🏛️ [exception](example/core/exception/main.cpp)
    - `C++17`용 예외 처리 보완 기능 <sub> `std::source_location`, `std::stacktrace` 유사 기능 </sub>
- 🏛️ `file`
    - [executable_path](example/core/file/test_01_executable_path.cpp) : 실행 파일 경로
    - [path_from_utf8](example/core/file/test_02_path_from_utf8.cpp) : UTF8 문자열 → 경로 변환
    - [finfo](example/core/file/test_04_finfo.cpp) : 파일 정보
    - [permissions_info](example/core/file/test_05_permissions_info.cpp) : 파일 권한
    - [file_size_formatter](example/core/file/test_06_file_size_formatter.cpp) : 파일 크기 형식화
    - [file_finder](example/core/file/test_07_file_finder.cpp) : 파일 찾기
    - [file_watcher](example/core/file/test_08_file_watcher.cpp) : 파일 감시자
- 🏛️ [findfile](example/core/findfile/main.cpp)
    - 파일에서 찾기 (`Find in files`)
- 🏛️ `hash`
    - [MD5](example/core/hash/test_generate_md5.cpp)
    - [SHA-256](example/core/hash/test_generate_sha256.cpp)
    - [HMAC-SHA256](example/core/hash/test_generate_hmac_sha256.cpp)
    - [PBKDF2](example/core/hash/test_derive_key_pbkdf2.cpp)
- 🏛️ [ini](example/core/ini/main.cpp)
    - [`.ini`](example/core/ini/sample.ini) 파일 파서
- 🏛️ [json](example/core/json/main.cpp)
    - `.json` 직렬화/역직렬화
- 🏛️ `log`
    - [utility_functions](example/core/log/test_utility_functions.cpp) : 로깅 유틸리티
    - [console_sink](example/core/log/test_console_sink.cpp) : 콘솔 싱크
    - [rolling_file_sink](example/core/log/test_rolling_file_sink.cpp) : 회전 파일 싱크
    - [logger](example/core/log/test_logger.cpp) : 로거
- 🏛️ [macro](example/core/macro/main.cpp)
    - 함수 시도(`TRY_OPT`) 계열 매크로
- 🏛️ `memory`
    - [basic_and_pod](example/core/memory/test_basic_and_pod.cpp) : 기본 타입 및 POD 직렬화 역직렬화
    - [string_and_vector](example/core/memory/test_string_and_vector.cpp) : string 및 vector 직렬화/역직렬화
    - [vector_of_structs](example/core/memory/test_vector_of_structs.cpp) : 구조체 vector 직렬화/역직렬화
    - [deep_copier](example/core/memory/test_deep_copier.cpp) : 깊은 복사
    - [fixed_memory_pool](example/core/memory/test_fixed_memory_pool.cpp) : 메모리 풀 기본 블록 할당/해제
    - [pool_allocator_with_stl](example/core/memory/test_pool_allocator_with_stl.cpp) : 풀 할당자 및 STL 컨테이너 연동
- 🏛️ [notification](example/core/notification/main.cpp)
    - 옵저버 등록 및 경고 알림.
- 🏛️ [notifications](example/core/notifications/main.cpp)
    - 이벤트 등록 및 알림/해제. 
- 🏛️ [overload](example/core/overload/main.cpp)
    - 다중 타입 처리
- 🏛️ [pfr](example/core/pfr/main.cpp) : 플랫 리플렉션(PFR)
    - 구조체 필드의 값 및 타입 접근 
- 🏛️ [process_util](example/core/process_util/main.cpp)
    - 프로세스 목록 및 정보 얻기
- 🏛️ [reflect](example/core/reflect/main.cpp)
    - 구조체의 직렬화/역직렬화 매크로
- 🏛️ `resilience`
    - [`bulkhead`](example/core/resilience/test_bulkhead.cpp) : 동시 몇 개까지 제한
    - [`circuit breaker`](example/core/resilience/test_circuit_breaker.cpp) : 장애가 발생 시 호출 차단
    - [`paginated list`](example/core/resilience/test_paginated_list.cpp) : 페이지 단위로 나누어 조회
    - [`retry helper`](example/core/resilience/test_retry_helper.cpp) : 실패 가능성 작업을 자동 재시도
    - [`sliding window limiter`](example/core/resilience/test_sliding_window_limiter.cpp) : 최근 시간(윈도우) 이내 요청 횟수 제한
    - [`token bucket`](example/core/resilience/test_token_bucket.cpp) : 빈도를 조절할 때 사용
    - [test_combined](example/core/resilience/test_combined.cpp) : `bulkhead`, `token bucket`, `circuit breaker` 조합 예제
    - [rate_limiters](example/core/resilience/test_rate_limiters.cpp) : 레이트 리미터 예제1
    - [rate_limiter_example](example/core/resilience/test_rate_limiter_example.cpp) : 레이트 리미터 예제2
- 🏛️ [result](example/core/result/main.cpp)
    - 타입 별 성공/실패 처리
- 🏛️ `schedule`  
    - [task](example/core/schedule/task/main.cpp) : 단일/지연/주기 작업 등록·취소
    - [weekly](example/core/schedule/weekly/main.cpp) : 주 단위 반복 작업 스케줄
- 🏛️ [scope](example/core/scope/main.cpp)
    - 스코프 탈출 시 자동 처리
- 🏛️ [server](example/core/server/main.cpp) 
    - 서버를 위한 기본 구조
- 🏛️ [service](example/core/service/main.cpp) 
    - 서비스 등록, 시작/중지, 상태 확인
- 🏛️ [shared_memory](example/core/shared_memory/main.cpp) : 공유 메모리 `IPC` 
    - 메모리 매핑, 동기화(세마포어/뮤텍스), 데이터 일관성 관리 예.  
- 🏛️ [singleton](example/core/singleton/main.cpp)
    - 싱글톤 패턴  
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
    - [string ext](example/core/string/test_string_ext.cpp) : <sub> 한국어 초성 추출/검색, 자모 분해/재조합, 유니코드 및 전각/반각 변환, 네이밍 스타일 변환, URL 슬러그, 문자열 유사도, 퍼지 검색, 제로-할당 string_view 기반 유틸리티, 구조화 데이터 안전 토크나이저 & 포맷팅 치환 </sub>
    - [print](example/core/string/test_print.cpp) : `print*`,`eprint*` 계열 콘솔 출력 기능
    - [format](example/core/string/test_format.cpp) : 문자열 포맷팅 기능
- 🏛️ [subsystem](example/core/subsystem/) 
    - `std in/out/err`를 사용하여 마스터([example_master](example/core/subsystem/example_master.cpp))와 워커([example_worker](example/core/subsystem/example_worker.cpp)) 간 통신
- 🏛️ `system`
    - [command_line](example/core/system/test_command_line.cpp) : 명령줄
    - [device_id_generator](example/core/system/test_device_id_generator.cpp) : 장치 ID 생성
    - [resource_monitor](example/core/system/test_resource_monitor.cpp) : CPU, 메모리, 디스크, 네트워크 사용량 모니터링
    - [process_runner](example/core/system/test_process_runner.cpp) : 프로세스 실행
    - [crash_handler](example/core/system/test_crash_handler.cpp) : 크래시 핸들러
- 🏛️ [thread](example/core/thread/main.cpp)
    - 동적 스레드·동시성
- 🏛️ [time](example/core/time/main.cpp)
    - 타이머(`hierarchical_timer_wheel`) : 주기적, 1회성, 타이머 취소
- 🏛️ [toml](example/core/toml/main.cpp)
    - `.toml` 파싱·직렬화
- 🏛️ [tpm](example/core/tpm/main.cpp)
    - 인메모리 TP 모니터
- 🏛️ [type_traits](example/core/type_traits/main.cpp)
    - 타입 특성 검사
- 🏛️ [uuid](example/core/uuid/main.cpp)
    - 고유 ID 생성·파싱  
- 🏛️ [validation](example/core/validation/main.cpp)
    - 이메일, 전화번호, `URL`, `IP`, `Base64`, `HEX` 색, `JSON`, 주민번호 검증
- 🏛️ [work_stealing_pool](example/core/work_stealing_pool/main.cpp)
    - 작업 훔치기(`Work-Stealing`) 스레드 풀 
- 🏛️ [xml](example/core/xml/main.cpp)
    - `.xml` 파싱·직렬화
- 🏛️ [yaml](example/core/yaml/main.cpp)
    - `.yaml` 파싱·직렬화
- 🏛️ [zip](example/core/zip/main.cpp)
    - `deflate` `.zip` 압축
	
<br />

	
##### 📦 [external](example/external) <sub> 외부 라이브러리 사용 </sub>
- 📦 [json](example/external/json/main.cpp) : 📄 `nlohmann::json`확장 기능
    - [json2cpp.py](mino/external/include/mino/external/json/json2cpp.py) : 📄 `nlohmann::json` => `C++` 구조체 변환
- 📦 `log` : 로깅 어댑터/팩토리
    - [adapter](example/external/log/adapter/main.cpp) : `spdlog` 기능 확장 어댑터 <sub> ⚡ `spdlog` </sub>
    - [factory](example/external/log/factory/main.cpp) : `spdlog` 확장 팩토리 패턴 <sub> ⚡ `spdlog` </sub> 
    - [spd](example/external/log/spd/main.cpp) : `spdlog` 추가 기능 <sub> ⚡ `spdlog` </sub>
    - [sqlite](example/external/log/sqlite/main.cpp) : `spdlog` + `sqlite` 확장 <sub> 🗄️ `sqlite3` </sub>
- 📦 `schedule` : `core` 스케줄러 확장
    - [weekly](example/external/schedule/weekly/main.cpp) : 주간 스케줄러 어댑터 `nlohmann::json` 확장 <sub> 📄 `nlohmann::json` </sub> 
- 📦 [`xml`](example/external/xml/main.cpp) : `.xml` => `C++` 변환 구조체 사용 <sub> 🐶 `pugixml` </sub>  
    - [xml2cpp.py](mino/external/include/mino/external/xml/xml2cpp.py) : [`.xml`](example/external/xml/catalog.xml) => [`C++` 구조체](example/external/xml/catalog.hpp) 변환

<br />

##### 🔀 [network](example/network) <sub> 네트워크 </sub>
- 🔀 [ftp](example/network/ftp/tcp/main.cpp) 
    - `ftp`/`sftp` 클라이언트 <sub> `tcp` `ssh` </sub>
- 🔀 [interface](example/network/interface/main.cpp)
    - 네트워크 인터페이스 정보 조회
- 🔀 [log/manager](example/network/log/manager/main.cpp)
   - 로깅 `hard/soft/hot reloading` 관리자 <sub> `udp` </sub>
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
        - [broker](example/network/message_broker/broker/main.cpp) <sub> `tcp` </sub>
        - [pub](example/network/message_broker/pub/main.cpp) <sub> `tcp` </sub>
        - [sub](example/network/message_broker/sub/main.cpp) <sub> `tcp` </sub>
    - `python pub/sub`
        - [pub/message_publisher.py](example/network/message_broker/python/pub/message_publisher.py)
        - [sub/message_subscriber.py](example/network/message_broker/python/sub/message_subscriber.py)
    - 구조체 직렬화/역직렬화 
        - [pub-reflect](example/network/message_broker/pub-reflect/main.cpp)
           - [구조체](example/network/message_broker/reflect-sample.hpp) 직렬화 발행자 <sub> `core/reflect` `tcp` </sub>
        - [sub-reflect](example/network/message_broker/sub-reflect/main.cpp)
           - [구조체](example/network/message_broker/reflect-sample.hpp) 역직렬화 구독자 <sub> `core/reflect` `tcp` </sub>
- 🔀 `mqtt` <sub> (`Message Queuing Telemetry Transport`) </sub>
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
- 🔀 `rpc` <sub> `Remote Procedure Call` </sub>
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
- 🔀 [sftp](example/network/sftp/putty/main.cpp)
    - `psftp` 연동 클라이언트
- 🔀 [ssh](example/network/ssh/main.cpp)
    - 독립형 `ssh` 클라이언트 
- 🔀 `tcp`
    - `tcp` 서버, 클라이언트
        - [server](example/network/tcp/server/main.cpp) : `TCP` 서버
        - [client](example/network/tcp/client/main.cpp) : `TCP` 클라이언트
    - 수신 버퍼 처리
        - [buffer_server](example/network/tcp/buffer_server/main.cpp) : `TCP` 버퍼 서버
        - [buffer_client](example/network/tcp/buffer_client/main.cpp) : `TCP` 버퍼 클라이언트
- 🔀 `udp`
    - [receiver](example/network/udp/receiver/main.cpp) : `UDP` 수신
    - [sender](example/network/udp/sender/main.cpp) : `UDP` 송신
    - [buffer_receiver](example/network/udp/buffer_receiver/main.cpp) : `UDP` 수신 버퍼 처리
- 🔀 [util](example/network/util/main.cpp)
    - 네트워크 인터페이스 목록 얻기 및 `IP` 주소 검증 

<br />

##### 🔀 [network_curl](example/network_curl) <sub> `libcurl` 사용 네트워크 </sub>
- 🔀 [file-downloader](example/network_curl/file-downloader/main.cpp) 
    - 파일 다운로더 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>
- 🔀 [ftp](example/network_curl/ftp/main.cpp) 
    - `ftp`/`sftp` 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>  
- 🔀 [multipart-downloader](example/network_curl/multipart-downloader/main.cpp)  
    - 멀티파트 <sub> (`multipart/mixed`,`multipart/form-data`) </sub> 다운로더 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub> 
- 🔀 [rest](example/network_curl/rest/main.cpp)  
    - `REST API` `GET`/`POST` 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>
- 🔀 [socket-io](example/network_curl/socket-io/main.cpp)
    - `socket.io` 클라이언트 <sub> 🌐 `libcurl` </sub> 
- 🔀 [ws](example/network_curl/ws/main.cpp)
    - 웹소켓(`ws:`,`wss:`) 클라이언트 <sub> 🌐 `libcurl` </sub> <sub> 🔒 `openssl` </sub>  

<br />

##### 🔀 [network_libssh2](example/network_libssh2) <sub> `libssh2` 사용 네트워크 </sub>
- 🔀 [ssh2](example/network_libssh2/ssh2/main.cpp) 
    - 소형 `ssh` 클라이언트 <sub> 🔑 `libssh2` </sub>

<br />

##### 🔀 [network_openssl](example/network_openssl) <sub> `openssl` 사용 네트워크 </sub>
- 🔀 [ftps](example/network_openssl/ftps/main.cpp) 
    - `FTPS` 클라이언트 <sub> 🔒 `openssl` </sub> <sub> `tls` `tcp` </sub>
- 🔀 [multipart-downloader](example/network_openssl/multipart-downloader/main.cpp) <sub> 📡 `httplib` </sub> <sub> 🔒 `openssl` </sub>
    - 멀티파트 <sub> (`multipart/mixed`,`multipart/form-data`) </sub> 다운로더 
- 🔀 [redis](example/network_openssl/redis/main.cpp) 
    -  [`Redis`](https://github.com/redis/redis) 클라이언트 <sub> 🔒 `openssl` </sub>
- 🔀 [rest](example/network_openssl/rest/main.cpp)
    - `REST API` 클라이언트 <sub> 📡 `httplib` </sub> <sub> 🔒 `openssl` </sub>
- 🔀 `tls`
    - [server](example/network_openssl/tls/server/main.cpp) : `TLS` 서버 <sub> 🔒 `openssl` </sub>
    - [client](example/network_openssl/tls/client/main.cpp) : `TLS` 클라이언트 <sub> 🔒 `openssl` </sub>

<br />

##### 🧱 [template](example/template) : 템플릿 예제 프로젝트
- 🧱 [mino_all_example](example/template/mino_all_example/CMakeLists.txt)
   - 통합 예제 템플릿 <sub> `core` `network` `external` </sub>
- 🧱 [mino_core_example](example/template/mino_core_example/CMakeLists.txt)
   - `core` 예제 템플릿
- 🧱 [mino_external_example](example/template/mino_external_example/CMakeLists.txt)
   - `external` 예제 템플릿
- 🧱 [mino_network_example](example/template/mino_network_example/CMakeLists.txt)
   - `network` 예제 템플릿

<br />

### 🏗️ 빌드 도구

#### `CMake` 사용자 프리셋 설정 <sub> `CMakeUserPresets.json` </sub>

- 빌드 환경에 따라 프리셋 정보를 맞춰서 수정하여 사용.

```json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 24,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base-common",
      "hidden": true,
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    { // Windows Visual Studio preset
      "name": "windows-vs-base",
      "hidden": true,
      "inherits": "base-common",
      "generator": "Visual Studio 17 2022",
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "installDir": "${sourceDir}/out/install/${presetName}",
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
    { // Linux GCC preset
      "name": "linux-gcc-base",
      "hidden": true,
      "inherits": "base-common",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "installDir": "${sourceDir}/install/${presetName}",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_C_COMPILER": "/usr/bin/gcc",
        "CMAKE_CXX_COMPILER": "/usr/bin/g++"
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
  ],
  "buildPresets": [
    {
      "name": "vs-debug",
      "displayName": "Build VS (Debug)",
      "configurePreset": "vs-debug",
      "configuration": "Debug"
    },
    {
      "name": "vs-release",
      "displayName": "Build VS (Release)",
      "configurePreset": "vs-release",
      "configuration": "Release"
    },
    {
      "name": "linux-gcc-debug",
      "displayName": "Build Linux GCC (Debug)",
      "configurePreset": "linux-gcc-debug"
    },
    {
      "name": "linux-gcc-release",
      "displayName": "Build Linux GCC (Release)",
      "configurePreset": "linux-gcc-release"
    }
  ]
}
```

<br />

#### ⊞ Windows 환경 🧩
- 🛠️ `Visual Studio` <sub> (2022 이상) </sub>
- 🔨 `cmake` <sub> (3.24 이상) </sub>
- 🥷 `ninja` <sub> (1.12.1 이상) </sub>
- 📦 `vcpkg` <sub> (2023.06 이상) </sub>
    - 사전에 환경변수 `VCPKG_ROOT`를 `vcpkg.exe`가 설치된 경로로 설정.
       - `VCPKG_ROOT`는 `PATH` 경로에 추가하여야 함. 
    - :one: `Visual Studio` 인 경우,
       - `vcpkg integrate install` 명령 실행.
       - 또는 `Tools`/`Options`/`vcpkg Pacakage Manager`에 `VCPKG_ROOT` 경로 설정.
    - :two: `VS Code`인 경우 : `.vscode`/`settings.json`를 다음과 같이 설정.
       ```json
        {
            "cmake.configureSettings": {
              "CMAKE_TOOLCHAIN_FILE": "${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            }
        }
       ```
       - (1) `Ctrl` + `Shift` + `P` 누른 후, `CMake: Set Build and Launch/Debug Target`을 선택하고, 디버깅 대상인 프로그램을 선택.
       - (2) `F5` <sub> (또는 디버깅 단축키) </sub> 를 누르고 디버깅 실행.

<br />

#### 🐧 Linux 환경 
- 🦬 `gcc` <sub> (8.5 이상) </sub>
- 🔨 `cmake` <sub> (3.24 이상) </sub>
- 🥷 `ninja` <sub> (1.8.2 이상) </sub>
- `Linux`에서는 `vcpkg`는 **사용하지 않는** 것을 가정하였음.

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
- 라이브러리 빌드 모드 설정: `Debug`, `Release`
- 라이브러리 경로 설정: `C:\opt\mino`, `~/mino` 등

###### :one: 🛠️ `Visual Studio` + 📦 `vcpkg` 환경
```bat
::::::::::::::::::::::::::::::::::::::::::::::::::
:: 기존 작업 경로 삭제 (Windows)
rmdir /s /q build

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (vcpkg 사용 시) (Debug)
cmake -B build ^
  -S . ^
  -G "Ninja" ^
 -DCMAKE_BUILD_TYPE=Debug ^
 -DCMAKE_INSTALL_PREFIX="C:\opt\mino" ^
 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
:: -DCMAKE_CXX_STANDARD=17 를 이용하여 C++ 표준 버전 설정 가능.
:: %VCPKG_ROOT% 는 환경설정 정보로 vcpkg.exe가 있는 경로.

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (vcpkg 사용 시) (Release)
cmake -B build ^
 -S . ^
 -G "Ninja" ^
 -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_INSTALL_PREFIX="C:\opt\mino" ^
 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

::::::::::::::::::::::::::::::::::::::::::::::::::
:: cmake 설정 (Windows) (vcpkg 미사용) (Release)
cmake -B build ^
 -S . ^
 -G "Ninja" ^
 -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_INSTALL_PREFIX="C:\opt\mino"

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
cmake -B build \
 -S .\
 -G "Ninja" \
 -DCMAKE_CXX_STANDARD=17 \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX="~/mino"
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
 |   +- 📁 cmake/
 |   +- 📄 mino_*.lib
 +- 📁 third-party/

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
 |   +- 📁 cmake/
 |   +- 📄 libmino_*.a
 +- 📁 third-party/

 ```

#### 🌰 실행 프로그램 프로젝트 구성

- mino를 적용할 실행 프로그램 <sub>(또는 라이브러리)</sub> 의 `CMakeLists.txt` 구성 방법
```cmake
cmake_minimum_required(VERSION 3.24)

project(mino_all_example_project VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME "mino_all_example")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

###########################
# 추가할 부분 {{
if(NOT DEFINED MINO_DIR)
   set(MINO_DIR "${CMAKE_SOURCE_DIR}/mino")
endif()

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${EXE_NAME} ${SRC_FILES})

if(EXISTS "${CMAKE_SOURCE_DIR}/mino/cmake-module")
    file(GLOB MINO_CMAKE_FILES "${CMAKE_SOURCE_DIR}/mino/cmake-module/Use*.cmake")
    foreach(FILE ${MINO_CMAKE_FILES})
        include(${FILE})
    endforeach()    
else()
    if(EXISTS "${MINO_DIR}/lib/cmake/mino")
        file(GLOB MINO_CMAKE_FILES "${MINO_DIR}/lib/cmake/mino/Use*.cmake")
        foreach(FILE ${MINO_CMAKE_FILES})
            include(${FILE})
        endforeach()    
    else()
        message(FATAL_ERROR "Please check MINO_DIR path.")
    endif()
endif()
# 종속성이 있는 경우 종속성이 낮은 것을 가장 뒤에 위치함
use_mino_network_curl(${EXE_NAME} "${MINO_DIR}") # 종속성: mino_core, mino_network
use_mino_network_ssh2(${EXE_NAME} "${MINO_DIR}") # 종속성: mino_core, mino_network
use_mino_network_openssl(${EXE_NAME} "${MINO_DIR}") # 종속성: mino_core, mino_network
use_mino_network(${EXE_NAME} "${MINO_DIR}") # 종속성: mino_core
use_mino_external(${EXE_NAME} "${MINO_DIR}") # 종속성: mino_core
use_mino_core(${EXE_NAME} "${MINO_DIR}")

# 위의 모듈 중 사용하는 것만 추가하여 사용하면 됨

# 추가할 부분 }}
###########################

```

- 빌드 시 다음과 같이 `cmake`를 호출
```bash
# mino 라이브러리가 설치된 경로인 MINO_DIR 를 설정
cmake -S . -B build -G "Ninja" \
 -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Debug \
 -DMINO_DIR="/home/jaytwo/mino" 

# 실행 프로그램 빌드 
cmake --build build -j

```

<br />

---

### ©️ 라이선스
- `MIT License`
   - 상세 내용 [LICENSE](LICENSE) 참고
- 📜 외부 라이브러리 
    - 📦 `external` 모듈
        - 📄 [nlohmann/json](https://github.com/nlohmann/json) : `MIT License` <sub> header-only </sub>
        - ⚡ [spdlog](https://github.com/gabime/spdlog) : `MIT License` <sub> header-only </sub>
        - 🐶 [pugixml](https://github.com/zeux/pugixml) : `MIT License` <sub> header-only </sub>
        - 🗜️ [miniz-cpp](https://github.com/tfussell/miniz-cpp) : `MIT License` <sub> header-only </sub>
        - 🗄️ [sqlite3](https://www.sqlite.org/index.html) : `Public Domain` <sub> amalgamation </sub>
    - 🔀 `network` 모듈
        - 📡 [cpp-httplib](https://github.com/yhirose/cpp-httplib) : `MIT License` <sub> header-only </sub>
        - 🌐 [libcurl](https://curl.se/) : [`Curl License`](https://curl.se/docs/copyright.html) <sub> static or dynamic library </sub>
        - 🔑 [libssh2](https://www.libssh2.org/) : `BSD-3 License` <sub> static or dynamic library </sub>
        - 🔒 [openssl](https://www.openssl.org/) : `Apache License 2.0`  <sub> static or dynamic library </sub>
        - 🥖 [brotli](https://github.com/google/brotli) : `MIT License`  <sub> static or dynamic library </sub>


