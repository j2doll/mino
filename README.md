# mino

- `mino`는 `C++` 범용 라이브러리로 다양한 기능들을 제공합니다.

---

### 🏗️ 빌드 도구

#### 🧩 Windows 환경

- `Visual Studio` (2022 이상)
- `cmake` (3.26 이상)
- `ninja` (1.8.2 이상)

##### 📦 라이브러리 설치 

- 라이브러리를 릴리즈(`Release`) 모드로 빌드
- 라이브러리를 `C:\opt\mino` 경로에 설치

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

---

