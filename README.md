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
    - `CMakePresets.json` 설정
    ```json
     {
       // ...
       "configurePresets": [
       {
          "cacheVariables": {
           "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
         }
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
sudo dnf install -y epel-release
sudo dnf install -y \
    spdlog-devel \
    json-devel \
    libcurl-devel \
    openssl-devel \
    cpp-httplib-devel \
    yaml-cpp \
    yaml-cpp-devel
```

- `Debian` 계열 (`Ubuntu`/`Debian`)
```bash
sudo apt update
sudo apt install -y \
  libspdlog-dev \
  nlohmann-json3-dev \
  libcurl4-openssl-dev \
  libssl-dev \
  libcpp-httplib-dev \
  libyaml-cpp-dev
```


##### 📦 라이브러리 설치 

- 라이브러리를 빌드 모드 설정 (`Release` 등)
- 라이브러리 경로 설정 (`C:\opt\mino` 등)

```bash
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:\opt\mino"
cmake --build build
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

