# Xcode + 手写/预编译静态库（脚手架说明）

本目录提供 **XcodeGen 工程描述** 与 **共享 xcconfig**。你在 `thirdparty/prebuilt/` 中放入自行编译的 `.a`/头文件后，在本目录执行 `./generate-project.sh`，即可生成 `KYSCPP.xcodeproj`。

## 0. 依赖

```bash
brew install xcodegen
```

仍需 **Xcode 15+**（支持 C++23）。若 Xcode 版本较低，可把 xcconfig 中的 `gnu++23` 改为项目实际支持的 dialect（工程可能无法用 C++20 以下的标准编过）。

## 1. 静态库目录约定（推荐）

在 `ios/thirdparty/prebuilt/` 布局如下（可自行改名，但必须同步修改 `xcconfig/Base.xcconfig`）：

```text
thirdparty/prebuilt/
├── include/              ← 聚合头文件（可把各库的 include  symlink 进来）
├── iphoneos/lib/        ← 真机 .a（及 pkg 用到的 .cmake 可不放）
└── iphonesimulator/lib/ ← 模拟器 .a（或只用 arm64 simulator 这一种）
```

- `HEADER_SEARCH_PATHS` 已在 `xcconfig/Base.xcconfig` 中指到 `$(THIRDPARTY_HEADERS)`，默认为 `thirdparty/prebuilt/include`。**请保证**这里有 `SDL3/SDL.h`、`SDL3_mixer`、`SDL3_ttf`、`SDL3_image`、`lua.hpp`、`yaml-cpp`、`zip.h`、`sqlite3.h` 等。
- **`$(PLATFORM_NAME)`** 会自动选用 `iphoneos` 或 `iphonesimulator` 对应的 `LIBRARY_SEARCH_PATHS`。

需要链接哪些系统框架，请对照你构建 **libSDL3.a** 时官方文档的最终链接列表：[SDL README / iOS](https://wiki.libsdl.org/SDL3/README-ios)。常见会与 Metal、音频、陀螺仪等相关框架一起链；按需追加到 Xcode target 的 **Link Binary With Libraries**（或通过 XcodeGen `dependencies.sdk`）。

## 2. game.zip（必做）

在项目根目录执行：

```bash
./scripts/pack_game_zip.sh
```

将生成的 **`game.zip`** 拷贝到 **`ios/BundledResources/game.zip`**（文件名固定，便于 Xcode 打进包内；该路径已写入 `project.yml`，若改名请同步 YAML）。

初次换资源包可删手机里 App **重装**，或删掉沙盒里的 `.game_extracted`（见 `Engine::extractAssetsIfNeeded`）。

## 3. 生成 Xcode 工程

```bash
cd ios
./generate-project.sh
open KYSCPP.xcodeproj
```

在 Xcode 中：

1. 选中 target **KYSCPP**，Signing 选你的 Team，`Bundle Identifier` 唯一。
2. **Link Binary With Libraries**：除 xcconfig 里 `-lSDL3 ...` 外，补上 SDL 所需的 **Apple 系统框架**（以你编的 SDL 版本为准）。
3. 首次编译若缺符号，逐项检查是哪个静态库没带进来或链接顺序不对（静态链常见）。

## 4. Lua 头路径说明

源码在 iOS 上走 `lua.hpp`（与 Android 对齐）。请将 **Lua** 安装头放进 `thirdparty/prebuilt/include`，并保持与 **liblua.a** ABI 一致（建议同一套脚本编的 Lua）。

## 5. 可选：`opencc` / `fluidsynth`

- **Mac 桌面 CMake** 链接了 **opencc**；**Android JNI** 列表未链接 opencc。**iOS** 若暂不链 opencc，需自行确认游戏里简繁与其它路径仍可接受（与 SimpleCC 等逻辑一致）。
- **SDL_mixer** 若启用 **FluidSynth**，需额外为 iOS 准备 **FluidSynth** 与各 **SoundFont**（体积与版权问题单独评估）；可先试 **仅用内置 mp3/wav**，降低首版难度。

## 6. 与仓库源码的对应关系

`project.yml` 中的源文件与 **`kys-cpp-androidstudio/app/jni/src/CMakeLists.txt`** 一致，其中 **`Cifa.cpp`** 使用仓库根目录 **`cifa/Cifa.cpp`**（Android 工程里写的是 `mlcc/Cifa.cpp`，该路径在磁盘上并不存在，应避免照抄）。

更宏观的流程仍见仓库根目录 **`doc/iOS移植说明.md`**。
