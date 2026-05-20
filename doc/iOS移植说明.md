# kys-cpp 在 iPhone 上运行（说明）

本仓库**已加入与 Android 类似的移动端逻辑**（见 `src/PlatformMobile.h`）：

- **资源目录**：真机/模拟器上使用 `SDL_GetPrefPath("kys-cpp", "kys")` 下的可写 `game/`（存档、解压后的资源写在这里）。
- **首次启动**：从 App 包内根目录的 **`game.zip`** 解压到上述目录（与 Android 的 `extractAssetsIfNeeded` 同源逻辑）；包内需包含 `game.zip`，且 zip 根目录下直接是 `config/`、`resource/` 等（与当前 PC 版 `game/` 布局一致）。

若选用 **「Xcode + 手写/预编译静态库」**，请直接使用仓库 **`ios/`** 下的脚手架：**`ios/README-Xcode静态库.md`**、`./ios/generate-project.sh`（需 `brew install xcodegen`），会在 `ios/` 生成 **`KYSCPP.xcodeproj`**。

完成静态库、`game.zip` 与签名后，即可在模拟器或真机运行。

## 1. 打包 `game.zip`

在项目根目录执行（将 `game/` 打成 zip，且**不要**多包一层 `game` 目录名）：

```bash
cd "$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
( cd game && zip -r -q ../game.zip . )
```

得到的 `game.zip` 供 Xcode 打进 App 包；需要更新资源时提高版本或删除应用重装以清除 `.game_extracted` 标记（逻辑见 `Engine::extractAssetsIfNeeded`）。

## 2. 工程与依赖（工作量主要在这里）

与 Mac 桌面版相同，工程依赖 **SDL3、SDL3_ttf、SDL3_image、SDL3_mixer**（及链路上的 **FluidSynth/mpg123** 等）、**Lua、yaml-cpp、sqlite3、libzip** 等。Android 侧通过 **vcpkg `arm64-android-dynamic`** 提供库；iOS 需任选其一：

1. **vcpkg iOS triplet**（若你环境已支持）：为 `arm64-ios`、`arm64-ios-simulator` 等安装同名端口，再在 CMake/Xcode 里链接生成物。  
2. **手写 Xcode**：用 [SDL 官方 iOS 说明](https://wiki.libsdl.org/SDL3/README-ios) 建工程，对其余库逐项编译静态库或用第三方 Pod / 预编译包，最后与游戏源码一起链接。

可参考本仓库 **`kys-cpp-androidstudio`** 中的 **源文件列表**（`CMakeLists.txt`）与排除项：**Android 动态库未链 `opencc`**，若 iOS 也省略 `opencc`，需与现有 `SimpleCC`/字体逻辑一致。

## 3. Xcode 最小清单

- 语言：**C++23**；嵌入 **Lua、脚本、SQLite** 等按现有 Android/桌面需求配置。  
- **Info.plist**：横屏等与 `SDL_SetHint(SDL_HINT_ORIENTATIONS, ...)` 一致；按需声明相册/麦克风等权限（本作若未用可不写）。  
- **签名**：个人/开发者账号、`Bundle Identifier` 唯一。  
- **上架**：原作美术/音乐版权归原作者所有；**App Store 分发需自行确保授权合规**，个人侧载/T 测另论。

## 4. 与 PC/Mac 的差异小结

| 项目       | Mac/PC        | iOS（本改动）                    |
|------------|---------------|----------------------------------|
| 资源路径   | `../game/`    | 偏好目录下 `game/`（解压后）     |
| 包内资源   | 目录即可      | 需 `game.zip` + Copy Bundle Resources |
| 虚拟摇杆   | 默认关        | 与 Android 相同，默认可关可开（`use_virtual_stick`） |

若你后续需要**本仓库内自带可打开的 `.xcodeproj` + CMake 生成步骤**，可以再说一下你倾向 vcpkg 还是全静态手工库，我可以按该路线把工程骨架补全到可编译状态（仍依赖本机 Xcode/依赖库环境）。
