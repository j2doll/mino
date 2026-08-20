# `mino`

- `mino`는 `C++` 범용 라이브러리로 다양한 기능들을 제공합니다.

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
sudo dnf install -y epel-release dnf-plugins-core
sudo dnf config-manager --set-enabled powertools

sudo dnf swap -y libcurl-minimal libcurl

sudo dnf install -y \
    gcc-c++ \
    cmake \
    make \
    pkgconfig \
    openssl-devel \
    libssh2-devel \
    libssh-devel \
    libcurl-devel \
    spdlog-devel \
    json-devel \
    yaml-cpp \
    yaml-cpp-devel \
    cpp-httplib-devel \
    ca-certificates
```

- `Debian` 계열 (`Ubuntu`/`Debian`)
```bash
sudo apt update -y

sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libssl-dev \
    libssh2-1-dev \
    libssh-dev \
    libcurl4-openssl-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libyaml-cpp-dev \
    libcpp-httplib-dev \
    ca-certificates
```

##### 📦 라이브러리 설치 

- 라이브러리 빌드 모드 설정 (`Debug`, `Release`)
- 라이브러리 경로 설정 (`C:\opt\mino` 등)

```bat
:: 작업 경로 삭제 (Windows)
rmdir /s /q build

:: 작업 경로 삭제 (Linux)
rm -rf build

:: cmake 설정 (Windows) (vcpkg 미사용)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:\opt\mino"

:: -DCMAKE_CXX_STANDARD=17 를 이용하여 C++ 표준 버전 설정 가능

:: cmake 설정 (vcpkg 사용 시) (Release)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:\opt\mino" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

:: cmake 설정 (vcpkg 사용 시) (Debug)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="C:\opt\mino" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

:: cmake 설정 (Linux)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/home/jaytwo/workspace/mino"

:: 빌드
cmake --build build -j

:: 빌드 (코어 갯수 한정. $(nproc) 대신 숫자로 정의)
cmake --build build -j "$(nproc)"

:: Linux 전용 빌드 (빌드만 빠르게 진행)
rm -rf build; cmake -S . -B build -G "Ninja" -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Debug; cmake --build build -j "$(nproc)"

:: 설치
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
    │       ├── minoConfig.cmake
    │       ├── minoConfigVersion.cmake
    │       ├── minoTargets-release.cmake
    │       └── minoTargets.cmake
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
    │       ├── minoConfig.cmake
    │       ├── minoConfigVersion.cmake
    │       ├── minoTargets-release.cmake
    │       └── minoTargets.cmake
    └── libmino_core.a
```

---

