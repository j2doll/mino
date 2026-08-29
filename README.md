# `mino` 

> :kr: **This project and the documentation were written in Korean.**

## 개요

### 프로젝트 개요
- `mino`는 `C++` 기반의 범용 라이브러리.
- 빠른 프로토타이핑과 재사용 가능한 컴포넌트 제공 목표.

### 아키텍처 요약
- ⚙️ [**core**](mino/core/include/mino/core)
   - 코어 기능. 외부 종속성이 없는 모듈.
- 📦 [**external**](mino/external/include/mino/external)
   - 외부 라이브러리 종속성이 있는 모듈.
- 🔀 [**network**](mino/network/include/mino/network)
   - 네트워크 기능 모듈.

### 예제 
#### `example` <sub> 예제 루트 경로 </sub>
##### ⚙️ `core` <sub> 코어 라이브러리 예제 </sub>
- [bit](example/core/bit/main.cpp) — 비트 연산 및 비트맵 처리  
    - 초기화 및 크기 관리, 비트 조작, 슬라이싱 & 병합, 연산자 오버로딩
    - 변환·디버깅: 변환, 비트 순서 역전, 출력 포맷 예시.
- [broker](example/core/broker/main.cpp) — 로컬/경량 메시지 브로커  
    - 타입과 이름(문자열)을 키로 객체를 등록·조회하는 서비스 로케이터 예제.  
- config — 설정 파일 로드·파싱  
    - 설정 파일의 주석·데이터 라인 분리와 타입별 안전 조회 예제입니다.  
- container — 컨테이너 유틸리티  
    - 컨테이너 어댑터, 범용 반복자 유틸, 변환·검색 편의 함수 사용 예제.  
    - 사용 패턴: 복합 컨테이너 조작, 성능/메모리 고려된 반복 및 뷰 활용 사례.  
- convert — 데이터 변환·직렬화  
    - 문자열↔숫자/시간 변환, 바이트 오더 변환, Base64/Hex 인코딩·디코딩 예제.  
    - 사용자 타입 직렬화: 구조체 ↔ 바이너리/문자열 직렬화 샘플과 에러 처리 관례.  
- crypt — 암호화·복호화  
    - 대칭/비대칭 암호화, 해시(sha/md), HMAC, 키 파생(KDF) 기본 사용 예.  
    - 스트림 암복호화: 파일·스트림에 대한 블록 처리와 패딩/IV 관리 예.  
- csv — CSV 입출력 및 파싱  
    - 구분자·인용 처리, 스트리밍 파싱 및 대용량 파일 처리 패턴.  
    - 매핑·검증: 필드 매핑, 타입 변환, 오류 라인 건너뛰기/보고 방식 예시.  
- daemon — 데몬/서비스 실행  
    - POSIX/Windows 서비스 초기화, 백그라운드 전환, 신호 및 종료 핸들링 패턴.  
    - 운영 관례: PID 파일 관리, 로그 초기화, 안전한 종료 시퀀스 예.  
- `datetime`  
    - unit — 단위 테스트 및 유틸 검증(경계·윤년 등)  
    - util — 포맷/파싱, ISO 표기, 타임존 보정 예제  
- encoding — 문자 인코딩·디코딩  
    - UTF-8/16/32 변환, BOM 처리, 로케일별 콘솔 출력 예시로 다국어 대응.  
- enum — 열거형 헬퍼  
    - enum↔문자열 변환, 비트플래그 조작, 안전한 직렬화/파싱 예제.  
- expected — `expected<T,E>` 스타일 오류 처리  
    - 성공·실패 체이닝, 에러 전파 예제 및 디버깅용 메시지 수집 패턴.  
- file — 파일 I/O 유틸  
    - 원자적 쓰기(atomic write), 파일 잠금(lock), 메모리 매핑, 임시 파일 안전 사용법.  
- hash — 해시·체크섬 생성  
    - MD5/SHA/CRC 등 해시 생성과 스트림/파일 연동 사용 예.  
- ini — INI 파서  
    - 섹션·키 파싱, 원본 주석 보존, 타입별 안전 조회(`sample.ini` 포함).  
- json — JSON 직렬화/역직렬화  
    - nlohmann::json 연동, 사용자 타입 어댑터, 스트림 입출력 및 성능 팁.  
- log — 로깅 사용 예  
    - 로거 초기화, 레벨·포맷 설정, sink 교체 및 비동기 로깅 설정 예.  
- macro — 매크로·템플릿 메타프로그래밍  
    - 컴파일 타임 유틸리티, SFINAE/컨셉 기반 예제, 안전한 매크로 패턴.  
- memory — 메모리 유틸리티  
    - 풀 allocator, 스마트 포인터 확장, 메모리 누수 검사 보조 유틸.  
- notification — 단일 알림 패턴  
    - 콜백 등록/해제, 동기·비동기 알림 처리 예.  
- notifications — 복합 알림·구독  
    - 토픽 기반 배포, 다중 구독자, 필터링 및 스레드 안전 배포 예.  
- overload — 오버로드 유틸  
    - 여러 callable 결합, 오버로드 해상도 예시, 가변 인자 처리 패턴.  
- pfr — 플랫 리플렉션(PFR) 예제  
    - 구조체 필드 자동 접근·순회, 자동 직렬화·비교 샘플.  
- process_util — 프로세스 유틸리티  
    - 자식 프로세스 실행·출력 캡처·타임아웃 처리 및 종료 코드 분석.  
- reflect — 리플렉션·메타데이터  
    - 런타임/컴파일타임 리플렉션 사용 패턴, 메타데이터 주입 예.  
- resilience — 복원력 패턴  
    - 재시도 전략, 지수 백오프, 서킷 브레이커 기본 예제.  
- result — `result<T,E>` 관례 예제  
    - 에러 캡슐화, 안전한 전파·변환 패턴.  
- `schedule`  
    - task — 단일/지연/주기 작업 등록·취소 예  
    - weekly — 요일 기반 반복 작업 스케줄 예  
- server — 간단 서버 예제  
    - 요청 수신·응답 루프, 멀티스레드 처리 골격, 종료 관리.  
- service — 서비스 레이어 샘플  
    - 비즈니스 로직 분리, 의존성 주입·인터페이스 모킹 예.  
- shared_memory — 공유 메모리 IPC  
    - 메모리 매핑, 동기화(세마포어/뮤텍스), 데이터 일관성 관리 예.  
- singleton — 싱글톤 패턴  
    - 안전한 지연 초기화, 멀티스레드 환경에서의 접근 패턴.  
- string — 문자열 유틸리티  
    - 트림/스플릿/포맷/유효성 검사, 성능 고려 사례.  
- system — 시스템 헬퍼  
    - 환경변수, 경로 변환, 호스트/프로세스 정보 조회 예.  
- thread — 스레드·동시성 유틸  
    - 스레드풀, 동기화 프리미티브, 작업 큐 패턴.  
- tpm — TPM(보안 모듈) 워크플로우  
    - TPM 초기화, 키 생성·서명·검증(실환경에서 동작 여부 확인 필요).  
- uuid — UUID 생성·파싱  
    - 버전별 생성, 문자열 ↔ UUID 변환, 비교 예제.  
- validation — 입력/모델 검증  
    - 필드 규칙 정의, 체이닝 검증, 에러 메시지 집계.  
- xml — XML 파싱·직렬화  
    - DOM/스트리밍 파싱, 네임스페이스·XPath 유사 접근 예.  
- yaml — YAML 파싱·직렬화  
    - 설정 로드, 노드 탐색, 사용자 타입 매핑 예.  
##### 📦 `external` <sub> 외부 라이브러리 사용 예제 </sub>
- json — nlohmann::json 확장 연동  
    - 외부 JSON 라이브러리 커스터마이즈, 사용자 타입 어댑터 및 성능 팁.  
- `log` — 외부 로깅 어댑터/팩토리  
    - adapter — 내부 로그 추상화층에 외부 로거 연결 예.  
    - factory — 런타임 로거 구성 변경·팩토리 패턴 예.  
    - spd — spdlog 연동: sink/formatter/비동기 설정 예.  
- `schedule` — 외부 스케줄러 연동  
    - weekly — 외부 스케줄러 어댑터 통합 예.  
##### 🔀 `network` <sub> 네트워크 관련 예제 </sub>
- `download` — 다운로드 클라이언트
    - curl — libcurl 기반 다운로드: 파일/스트림, 재시도, 프로그레스 처리.  
    - httplib — httplib 기반 간단 HTTP 다운로드 예.  
- `ftp` — FTP 연동
    - curl — curl을 이용한 업/다운로드, 인증 처리 예.  
    - tcp — 저수준 TCP로 FTP 프로토콜 처리하는 실습 예.  
- interface — 네트워크 추상화  
    - 플랫폼 간 소켓 추상화, 동기/비동기 IO 모델 비교 예.  
- `log` — 네트워크 로깅
    - manager — 원격 로그 전송 파이프라인과 설정 예(`logger_manager_config.ini`).  
- `memory_store` — 네트워크 기반 메모리 저장소  
    - client / server — 간단한 키-값 프로토콜, 동시성/직렬화 예.  
- `message_broker` — 분산 메시지 브로커  
    - broker — 퍼블리시/구독 모델, 토픽 라우팅 예.  
    - pub / sub — 퍼블리셔/구독자 샘플과 직렬화 포맷.  
    - `python` 연동: pub / sub — 언어 경계 통합 시나리오.  
- `rest` — REST 클라이언트/서버  
    - curl / httplib — 요청/응답, 헤더·쿼리 처리, JSON 페이로드 전송 예.  
- `rpc` — RPC 클라이언트/서버  
    - client / server — 직렬화·버전 호환 패턴과 인터페이스 정의 예.  
    - 공통: `rpc_example_common.hpp`.  
- `sftp` — SFTP 연동  
    - putty — 외부 툴(psftp) 연동 자동화 샘플.  
- `tcp` — TCP 소켓 예제  
    - client / server — 연결 관리, 멀티플렉싱, 프래밍 처리 예.  
- `udp` — UDP 통신 예제  
    - receiver / sender — 비연결 전송, 멀티캐스트·브로드캐스트 예.  
- util — 네트워크 헬퍼  
    - 주소 변환, 타임아웃 헬퍼, 재시도/회복 패턴 유틸 예.  
##### 🧱 `template` <sub> 템플릿 예제 프로젝트 </sub> 
- mino_all_example — 코어·외부·네트워크 통합 예: 앱 초기화·모듈 통합 흐름 시연.  
- mino_core_example — 코어 기능만으로 구성한 최소 실행 샘플.  
- mino_external_example — vcpkg로 설치한 외부 종속성과의 통합 흐름 예.  
- mino_network_example — 네트워크 기능 중심의 통합 데모 (클라이언트/서버 메시지 흐름).

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
