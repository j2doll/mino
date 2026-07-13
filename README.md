# mino

- `mino`는 `C++` 범용 라이브러리로 다양한 기능들을 제공합니다.

---

### 🏗️ 빌드 도구

#### 🧩 Windows 환경

- `Visual Studio` (2022 이상)
- `cmake` (3.30 이상)
- `ninja` (1.12.1 이상)

#### 🧩 Linux 환경

- `gcc` (8.5 이상)
- `cmake` (3.26 이상)
- `ninja` (1.8.2 이상)

##### 📦 라이브러리 설치 

- 라이브러리를 빌드 모드 설정 (`Release` 등)
- 라이브러리 경로 설정 (`C:\opt\mino` 등)

```
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
│       └── bit
│           ├── bit.hpp
│           └── bit_array.hpp
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
│       └── bit
│           ├── bit.hpp
│           └── bit_array.hpp
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

