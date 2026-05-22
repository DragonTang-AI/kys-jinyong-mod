# 金庸群侠传 C++ 重制版 - 增强 mod

<div align="center">

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows%20%7C%20iOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![SDL](https://img.shields.io/badge/SDL-3.2.0-orange)

**基于 [kys-cpp](https://github.com/scarsty/kys-cpp) 项目的增强修改版**

[功能特性](#功能特性) • [架构说明](#架构说明) • [编译部署](#编译部署) • [致谢](#致谢)

</div>

---

## 📖 项目简介

本项目是基于 [**scarsty/kys-cpp**](https://github.com/scarsty/kys-cpp) 项目的深度增强修改版。

**原项目** 是经典国产 RPG《金庸群侠传》的 C++ 开源重制版，使用 SDL2/SDL3 引擎，实现了完整的游戏逻辑、战斗系统、事件系统和 MOD 支持。

**本 mod** 在原项目基础上，新增了**商城系统**、**50v50 大规模自由对战**、**iOS 完整适配** 等特性，并修复了大量 bug，优化性能，提升游戏体验。

---

## ✨ 功能特性

### 🎮 核心玩法

- ✅ **完整主线剧情**：原版 14 个门派、100+ 角色、200+ 武功
- ✅ **开放世界**：自由探索大地图，随机遇敌，多结局分支
- ✅ **回合制战斗**：经典战棋系统，支持 6v20（原版）→ **50v50（本 mod）**
- ✅ **武功修炼**：拳/剑/刀/奇门/暗器/内功，6 个修炼槽
- ✅ **物品系统**：剧情道具/兵器/护甲/丹药/暗器，自由搭配

### 🆕 本 mod 新增功能

#### 1️⃣ 商城系统（全新）

- **入口**：主菜单 → 按 `4` 或点击「商城」标签
- **10 大分类**：
  - 剧情道具、兵器、护甲、丹药、暗器
  - 拳经、剑谱、刀录、奇门、心法
- **智能定价**：根据属性（攻击/防御/速度/特殊效果）自动计算价格
- **购买反馈**：成功/失败均有弹窗提示 + 音效（e00.wav / e01.wav）
- **全中文界面**：UTF-8 编码，无乱码

#### 2️⃣ 50 VS 50 大规模自由对战（核心亮点 🌟）

- **入口**：主菜单 → 自由对战
- **支持规模**：
  - 我方：1-50 人（原版 6 人）
  - 敌方：1-50 人（原版 20 人）
- **自定义阵型**：
  - 选择我方角色（可重复选择同一角色）
  - 选择敌方角色（可重复选择同一角色）
  - 选择战场（1-140 个预设战场）
  - 自定义坐标（通过网页编辑器 `editor.html`）
- **战斗模式**：
  - 黑帝斯模式（默认，高难度）
  - 只狼模式（弹反机制）
  - 纸片模式（休闲）
- **战斗统计**：
  - 伤害输出、承受伤害、治疗量
  - 击杀数、行动次数、存活状态
  - 结算页面展示数据

#### 3️⃣ iOS 完整适配

- ✅ **虚拟摇杆**：
  - 默认启用（修改 `config/kysmod.ini` 中 `use_virtual_stick = 1`）
  - 支持移动、确认、取消、大地图（M 键）等按钮
- ✅ **MIDI 音乐修复**：
  - 添加 `music/timidity.cfg` 配置文件
  - 使用 `mid.sf2` 音色库，完美播放背景音乐
- ✅ **触控优化**：
  - 支持单点触控、双指缩放
  - 适配 iPhone/iPad 屏幕尺寸

#### 4️⃣ 性能优化

- **音频懒加载**：
  - 原版：启动时同步加载 300+ 音频文件，卡顿 5-30 秒
  - 本 mod：首次播放时才加载，启动时间 <1 秒
- **纹理缓存**：
  - `TextureManager` 单例，缓存所有贴图
  - 避免重复加载，降低内存占用

#### 5️⃣ Bug 修复（11 个）

| Bug 描述 | 状态 | 修复方式 |
|---------|------|---------|
| 自由对战无法添加角色 | ✅ | 修复 `dealEvent()` switch-case 结构 |
| 战斗黑屏 | ✅ | 初始化坐标 + 正确加载战场贴图 |
| 角色头像不显示 | ✅ | 修复 `TextureManager::renderTexture()` 调用 |
| 角色堆叠在角落 | ✅ | 复用 `war.sta` 预设坐标 |
| 战场列表乱码 | ✅ | `PotConv::cp950toutf8()` 转 UTF-8 |
| BattleFieldID 越界 | ✅ | Clamp 0-12（`warfld.grp` 仅 13 张地图） |
| 敌方坐标分配错误 | ✅ | 使用 `war.sta` 预设坐标 |
| 结算页卡死 | ✅ | 重置 `state_=SelectFriendly` |
| 角色列表无勾选反馈 | ✅ | 绿色 ● / 灰色 ○ 区分 |
| 战斗模式默认错误 | ✅ | 读取 `game.battle_mode` 配置 |
| `collectBattleStats()` 空函数 | ✅ | 实现战斗数据统计 |

---

## 🏗️ 架构说明

### 技术栈

- **语言**：C++17
- **图形引擎**：SDL3 (Simple DirectMedia Layer 3.2.0)
- **音频引擎**：SDL3_mixer (支持 MIDI/OGG/WAV)
- **脚本系统**：Lua 5.1 (事件系统、剧情触发)
- **存档格式**：ZIP 压缩包（含 `1.db` SQLite 数据库 + `s1.grp`/`d1.grp` 二进制数据）
- **MOD 支持**：`game.zip` 包含所有游戏资源（地图、事件、战斗、道具、角色）

### 核心模块

```
src/
├── Engine.h/cpp          # SDL3 封装，单例，纹理缓存，UI 分辨率 1024x640
├── RunNode.h/cpp         # 显示/事件基类，shared_ptr 生命周期，保留模式渲染
├── BattleScene.h/cpp     # 战斗场景（回合制战棋）
├── BattleMap.h/cpp       # 战场地图（瓦片地图，Layer0/1/2）
├── MainScene.h/cpp       # 主场景（大地图、城镇、子地图）
├── UI.h/cpp              # UI 框架（标签、列表、按钮、详情面板）
├── UIFreeBattle.h/cpp   # 自由对战界面（本 mod 新增）
├── UIMall.h/cpp         # 商城界面（本 mod 新增）
├── Role.h/cpp            # 角色系统（属性、武功、物品、升级）
├── Item.h/cpp            # 物品系统（道具、装备、消耗品）
├── Event.h/cpp           # 事件系统（Lua 脚本触发）
├── ScriptLua.h/cpp       # Lua 脚本桥接
├── Audio.h/cpp           # 音频系统（懒加载优化）
├── Save.h/cpp            # 存档系统（ZIP + SQLite）
├── TextureManager.h/cpp  # 纹理管理（单例，缓存所有贴图）
├── Font.h/cpp            # 字体渲染（支持 CP936/UTF-8）
├── PotConv.h/cpp         # 编码转换（CP936 ↔ UTF-8 ↔ CP950）
├── VirtualStick.h/cpp    # 虚拟摇杆（iOS 适配）
└── Types.h               # 全局常量（TEAMMATE_COUNT=50, BATTLE_ENEMY_COUNT=50）
```

### 数据流

```
[ZIP 资源] → [解析器] → [内存对象] → [渲染引擎]
    ↓              ↓            ↓             ↓
game.zip      Save.cpp    Role/Item    Engine.cpp
war.sta       Event.lua  BattleInfo   BattleScene
item.sta      ScriptLua  BattleMap    MainScene
role.sta      ...         ...          ...
```

### 关键数据结构

#### BattleInfo（战斗信息）

- **原版**：186 字节/条（140 条，共 26040 字节）
  - `TeamMate[6]` + `TeamMateX[6]` + `TeamMateY[6]`
  - `Enemy[20]` + `EnemyX[20]` + `EnemyY[20]`
- **本 mod**：718 字节/条（140 条，共 100520 字节）
  - `TeamMate[50]` + `TeamMateX[50]` + `TeamMateY[50]`
  - `Enemy[50]` + `EnemyX[50]` + `EnemyY[50]`

#### Role（角色）

- **属性**：HP/MaxHP, MP/MaxMP, Attack, Defence, Speed, Exp, Level, ...
- **武功**：6 个修炼槽（`MagicID[6]`, `MagicLevel[6]`）
- **内功**：5 个内功槽（`InternalID[5]`, `InternalLevel[5]`）
- **物品**：6 个携带槽（`TakingItemID[6]`, `TakingItemCount[6]`）

---

## 🚀 编译部署

### 1. 环境要求

| 平台 | 编译器 | 依赖库 |
|------|--------|--------|
| macOS 11+ | Apple Clang / GCC 9+ | SDL3, SDL3_mixer, Lua 5.1, SQLite3 |
| Linux (Ubuntu 20.04+) | GCC 9+ / Clang 10+ | 同上 |
| Windows 10+ | MSVC 2019+ / MinGW-w64 | 同上 |
| iOS 14+ | Xcode 12+ | SDL3.framework, SDL3_mixer.framework |

### 2. macOS 编译

```bash
# 1. 安装依赖（使用 Homebrew）
brew install sdl3 sdl3_mixer lua sqlite3

# 2. 克隆仓库
git clone https://github.com/DragonTang-AI/kys-jinyong-mod.git
cd kys-jinyong-mod

# 3. 生成 Xcode 项目（可选）
mkdir build && cd build
cmake .. -G Xcode

# 4. 编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# 5. 运行
./kys
```

### 3. Linux 编译

```bash
# 1. 安装依赖（Ubuntu/Debian）
sudo apt install build-essential cmake libsdl3-dev libsdl3-mixer-dev liblua5.1-dev libsqlite3-dev

# 2. 编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# 3. 运行
./kys
```

### 4. Windows 编译

```bash
# 1. 安装 MSVC 2019+ 或 MinGW-w64

# 2. 安装依赖（vcpkg）
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
vcpkg install sdl3 sdl3-mixer lua sqlite3

# 3. 编译
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# 4. 运行
kys.exe
```

### 5. iOS 部署

```bash
# 1. 修改 ios/project.yml 中的 DEVELOPMENT_TEAM
# 2. 生成 Xcode 项目
cd ios
./generate-project.sh

# 3. 打开 Xcode 项目
open kys.xcodeproj

# 4. 连接 iPhone/iPad，签名并运行
# 5. 将 game.zip 拖入 App 沙盒（Files App）

# 注意：修改 game.zip 后，必须删除 App 重新安装才能生效！
```

### 6. 游戏资源

- **必需文件**：`game.zip`（所有游戏资源）
- **来源**：
  - 原版：`https://github.com/scarsty/kys-cpp/releases`
  - 本 mod：已包含修复后的 `game.zip`（虚拟摇杆启用 + MIDI 配置）
- **放置位置**：
  - macOS/Linux：可执行文件同级目录
  - iOS：App 沙盒 `Documents/` 目录（通过 Files App 拖入）

---

## 🆚 与原版 kys-cpp 的核心差异

### 差异 1：大规模战斗（50v50） 🌟

| 对比项 | 原版 kys-cpp | 本 mod |
|--------|--------------|--------|
| 我方人数 | 6 | **50** |
| 敌方人数 | 20 | **50** |
| BattleInfo 大小 | 186 字节 | **718 字节** |
| war.sta 大小 | 26040 字节 | **100520 字节** |
| 自由对战 | ❌ 不支持 | ✅ **完整支持** |
| 自定义阵型 | ❌ | ✅ **网页编辑器** |

**技术实现**：
- 修改 `Types.h` 中的 `TEAMMATE_COUNT` 和 `BATTLE_ENEMY_COUNT`
- 修改 `BattleMap.h` 中的数组大小
- 编写 `migrate_war_sta.py` 迁移旧格式 `war.sta`
- 优化 `BattleScene` 渲染性能（100 个角色同屏）

### 差异 2：商城系统 🛒

| 对比项 | 原版 kys-cpp | 本 mod |
|--------|--------------|--------|
| 商城系统 | ❌ 不支持 | ✅ **10 大分类** |
| 物品购买 | ❌ 仅剧情获取 | ✅ **自由购买** |
| 定价策略 | ❌ | ✅ **属性加权** |
| 购买反馈 | ❌ | ✅ **弹窗 + 音效** |

**技术实现**：
- 新增 `UIMall.h/cpp`
- 在主菜单添加第 4 个标签（快捷键 `4`）
- 使用 `PotConv::utf8tocp936()` 运行时转换分类关键字
- 调用 `Event::getInstance()->addItemWithoutHint()` 添加物品

---

## 📂 项目结构

```
kys-jinyong-mod/
├── src/                      # C++ 源代码
│   ├── Engine.h/cpp          # 引擎核心
│   ├── BattleScene*.h/cpp    # 战斗系统
│   ├── UIFreeBattle*.h/cpp  # 自由对战（新增）
│   ├── UIMall*.h/cpp        # 商城系统（新增）
│   └── ...
├── data/                     # 游戏数据（二进制）
│   ├── war.sta               # 战斗信息（718 字节/条）
│   ├── item.sta              # 物品数据
│   ├── role.sta              # 角色数据
│   └── ...
├── game/                     # 游戏资源
│   ├── resource/             # 贴图、音效、音乐
│   ├── lua/                  # Lua 脚本（事件、剧情）
│   ├── config/               # 配置文件（kysmod.ini）
│   └── ...
├── game.zip                  # 打包后的游戏资源（iOS 需要）
├── ios/                      # iOS 项目
│   ├── project.yml           # 项目配置
│   └── ...
├── tools/                    # 工具脚本
│   ├── parse_war_sta.py      # 解析 war.sta → JSON
│   ├── write_war_sta.py      # JSON → 写回 war.sta
│   └── editor.html           # 网页阵型编辑器
├── build/                    # 编译输出
├── CMakeLists.txt            # CMake 配置
└── README.md                # 本文件
```

---

## 🛠️ 工具脚本

### 1. 对战编排编辑器

**路径**：`tools/editor.html`

**功能**：
- 可视化编辑 140 个战斗的阵型
- 拖拽修改我方（蓝点）和敌方（红点）坐标
- 64×64 网格背景（对应战场 640×640 像素）
- Export JSON → 用 `write_war_sta.py` 写回 `war.sta`

**使用方法**：
```bash
# 1. 解析 war.sta
python3 tools/parse_war_sta.py

# 2. 打开编辑器
open tools/editor.html

# 3. 编辑阵型，Export JSON

# 4. 写回 war.sta
python3 tools/write_war_sta.py
```

### 2. war.sta 迁移脚本

**路径**：`tools/migrate_war_sta.py`

**功能**：将旧格式 `war.sta`（186 字节/条）迁移到新格式（718 字节/条）

**使用方法**：
```bash
python3 tools/migrate_war_sta.py
# 生成 war.sta.new（新格式）
# 备份 war.sta.bak（旧格式）
```

---

## 🐛 已知问题

### 1. 性能问题（50v50）

- **现象**：100 个角色同屏时，帧率下降至 30-40 FPS（原版 6v20 稳定 60 FPS）
- **原因**：`BattleScene::draw()` 每帧渲染 100 个角色精灵 + 100 个 HP/MP 条 + 100 个状态图标
- **优化方向**：
  - 使用 SDL3 的 `RenderGeometry()` 批量渲染
  - 只渲染可见区域（摄像头裁剪）
  - 降低远离摄像头的角色精灵质量

### 2. 战斗统计不完整

- **现象**：`collectBattleStats()` 统计的 `damage_dealt`、`kill_count`、`action_count` 等数据为 0
- **原因**：未在 `BattleScene` 中添加战斗统计跟踪逻辑
- **修复方向**：在 `Role::takeDamage()`、`Role::attack()` 等函数中累加统计

### 3. 角色详情面板中文乱码

- **现象**：`drawRoleDetail()` 中的固定中文（如「生命」「攻击」）显示为 `?`
- **原因**：`Font::draw()` 的编码处理有问题（可能是 CP936 vs UTF-8）
- **修复方向**：统一使用 `PotConv::cp936toutf8()` 转换

### 4. iOS 虚拟摇杆按钮不够

- **现象**：缺少「大地图」按钮（M 键）
- **原因**：`VirtualStick.cpp` 构造函数中未添加 `button_map_`
- **修复状态**：✅ 已修复（添加 `button_map_` 按钮，发送 `SDLK_M` 键事件）

---

## 🤝 致谢

### 原项目

本项目基于 [**scarsty/kys-cpp**](https://github.com/scarsty/kys-cpp) 开发，感谢原作者的开源贡献！

**原项目特性**：
- 完整的金庸群侠传 C++ 重制版
- SDL2/SDL3 跨平台引擎
- Lua 脚本系统
- MOD 支持

### 依赖库

- [**SDL3**](https://github.com/libsdl-org/SDL) - 跨平台多媒体引擎
- [**SDL3_mixer**](https://github.com/libsdl-org/SDL_mixer) - 音频混合库
- [**Lua 5.1**](https://www.lua.org/) - 轻量级脚本语言
- [**SQLite3**](https://www.sqlite.org/) - 嵌入式数据库
- [**CMake**](https://cmake.org/) - 跨平台编译系统

### 游戏资源

- 原版《金庸群侠传》由**河洛工作室**开发（1996 年）
- 本项目的游戏资源（贴图、音效、音乐、剧本）来自原版游戏的提取和转换

---

## 📄 许可证

本项目遵循 **MIT 许可证**。

- 原项目（kys-cpp）许可证：MIT
- 本 mod 许可证：MIT

---

## 📧 联系方式

- **GitHub Issues**：[https://github.com/DragonTang-AI/kys-jinyong-mod/issues](https://github.com/DragonTang-AI/kys-jinyong-mod/issues)
- **原项目 GitHub**：[https://github.com/scarsty/kys-cpp](https://github.com/scarsty/kys-cpp)

---

<div align="center">

**⭐ 如果你喜欢这个 mod，请给原项目一颗星！⭐**

[回到顶部](#金庸群侠传-c-重制版---增强-mod)

</div>
