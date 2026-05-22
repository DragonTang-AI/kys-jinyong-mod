# SDLPal iOS 部署 & 开发完整指南

## 📱 环境信息

- **设备**: iPhone 16 Pro Max (Jingyu)
- **开发环境**: macOS + Xcode 26.3
- **开发者账号**: 个人免费 Apple ID（¥699/年）
- **团队 ID**: `6Q2EK738ZP` (Jingyu Ma)
- **签名证书**: `Apple Development: JINGYU MA (K493NUX7J9)`
- **设备 UDID**: `5CDCE6BC-D43E-59EC-AFD3-7D15E361C446`

---

## 🚀 一键构建 & 安装

这是最常用的命令，每次改完代码后执行：

```bash
# 构建（同时自动签名）
export LANG=en_US.UTF-8
xcodebuild -workspace /Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal.xcworkspace \
  -scheme SDLPal \
  -destination 'generic/platform=iOS' \
  -allowProvisioningUpdates \
  build 2>&1 | tail -3

# 安装到 iPhone
xcrun devicectl device install app \
  --device 5CDCE6BC-D43E-59EC-AFD3-7D15E361C446 \
  /Users/admin/Library/Developer/Xcode/DerivedData/SDLPal-cakquryacsddqtdsxoctwbuelfan/Build/Products/Debug-iphoneos/SDLPal.app \
  2>&1 | grep "App installed"
```

---

## 🛠️ 项目结构

```
/Users/admin/Downloads/PAL/sdlpal-master/
├── ios/SDLPal/SDLPal.xcworkspace      ← iOS 项目入口
├── ios/SDLPal/SDLPal.xcodeproj        ← Xcode 项目配置
├── ios/SDLPal/SDLPal/                ← iOS 源代码目录
│   ├── util.mm                       ← iOS 工具函数（基础路径等）
│   ├── SDLPal_AppDelegate.m          ← App 生命周期
│   └── Resources/                    ← 资源文件（BGM、FON等）
├── play.c                            ← 主游戏循环
├── scene.c                           ← 场景/地图渲染
├── uigame.c                          ← UI/菜单/商城/辅助系统
├── uibattle.c                        ← 战斗UI
├── fight.c                           ← 战斗逻辑
├── script.c                          ← 脚本引擎
├── global.h                          ← 全局数据结构和变量
├── global.c                          ← 全局数据加载
├── text.c                            ← 文字/字体系统
├── font.c                            ← 字体渲染
├── fontglyph_tw.h                    ← 繁体中文16×16字形
└── input.c                           ← 键盘/触摸输入
```

### 资源文件路径
```
/Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal/Resources/
├── mgo.mkf       ← 场景 NPC/特效精灵
├── f.mkf         ← 玩家角色行走精灵
├── rgm.mkf       ← 对话框头像
├── ball.mkf       ← 道具图标位图
├── r.mkf          ← BGM 音乐
├── FANTI.FON     ← 繁体字体（备选）
└── sdlpal.cfg    ← 游戏配置
```

---

## 🔑 签名配置

项目已配置 `Automatic` 签名，团队 `6Q2EK738ZP`。不要更改 `DEVELOPMENT_TEAM`。

如果需要检查签名状态：

```bash
# 检查可用签名证书
security find-identity -v -p codesigning

# 检查项目团队配置
grep "DEVELOPMENT_TEAM" /Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal.xcodeproj/project.pbxproj
```

---

## 🧪 开发工作流

### 修改代码 → 构建 → 安装

1. **修改代码**（如 `uigame.c`、`scene.c` 等）
2. **构建**（编译 + 签名）
3. **安装到 iPhone**

### 常见操作

#### 快速编译安装（单条命令）
```bash
xcodebuild -workspace /Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal.xcworkspace -scheme SDLPal -destination 'generic/platform=iOS' -allowProvisioningUpdates build 2>&1 | tail -3 && xcrun devicectl device install app --device 5CDCE6BC-D43E-59EC-AFD3-7D15E361C446 /Users/admin/Library/Developer/Xcode/DerivedData/SDLPal-cakquryacsddqtdsxoctwbuelfan/Build/Products/Debug-iphoneos/SDLPal.app 2>&1 | grep "App installed"
```

#### 只编译（不安装）
```bash
xcodebuild -workspace /Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal.xcworkspace -scheme SDLPal -destination 'generic/platform=iOS' -allowProvisioningUpdates build
```

#### 检查编译错误
```bash
xcodebuild -workspace /Users/admin/Downloads/PAL/sdlpal-master/ios/SDLPal/SDLPal.xcworkspace -scheme SDLPal -destination 'generic/platform=iOS' -allowProvisioningUpdates build 2>&1 | grep "error:" | head -10
```

#### 检查链接错误
```bash
xcodebuild ... build 2>&1 | grep "Undefined\|duplicate symbol" | head -10
```

#### 检查设备连接
```bash
xcrun devicectl list devices | grep "16 Pro"
```

#### 重置设备连接
如果设备显示 `paired` 而不是 `connected`：
1. 确保 USB 线已连接
2. 解锁 iPhone
3. 如果有弹窗点"信任"
4. `devicectl` 会自动连接

---

## 📂 关键文件说明

### 核心修改文件

| 文件 | 作用 | 常用修改内容 |
|------|------|-------------|
| `uigame.c` | 菜单、辅助系统、商城 | 辅助功能、商店、召唤系统 |
| `scene.c` | 场景渲染、NPC | 飞剑特效、隐藏迷宫、NPC |
| `play.c` | 游戏主循环、小地图 | 小地图、按键绑定 |
| `fight.c` | 战斗逻辑 | AI 战斗、召唤攻击 |
| `uibattle.c` | 战斗UI | 血量显示、战斗HUD |
| `global.h` | 全局变量 | 添加新 flag/switches |
| `script.c` | 脚本引擎 | 遇敌开关 |

### 编码注意事项

- **文字编码**: 直接使用 `L"..."` 宽字符串
- **中文字体**: 繁体中文 16×16 字形在 `fontglyph_tw.h`（约13000字）
- **文字渲染**: 
  - `PAL_DrawText(..., TRUE, FALSE, FALSE)` = 使用 Unicode 字体
  - `PAL_DrawText(..., TRUE, FALSE, TRUE)` = 使用 8×8 ASCII 字体（不支持中文）
- **对话框系统**: 使用 `PAL_StartDialog` / `PAL_ShowDialogText` / `PAL_EndDialog`

---

## 📋 当前辅助功能列表

| # | 功能 | 说明 |
|:--:|------|------|
| 1 | 天降橫財 | +9999 金钱 |
| 2 | 起死回生 | 全队满血满魔 |
| 3 | 逆天改命 | Lv.99 + 全属性MAX |
| 4 | 無盡酒神 | 99壶酒 |
| 5 | 全隊升一級 | Lv+1 |
| 6 | 御劍飛行 | 穿墙 + 飞剑特效 |
| 7 | AI自動戰鬥 | 智能战斗AI |
| 8 | 無遇敵模式 | 关闭随机遇敌 |
| 9 | 地圖全亮 | 小地图迷雾全开 |
| 10 | 全物品獲取 | 全部道具×99（快照ON/OFF）|
| 11 | 召喚系統 | 选怪物做召唤兽 |

### 特殊按键

| 按键 | 功能 |
|:----:|------|
| X | 商城（代替原法术菜单）|
| Y | 菜单 → 辅助/系统 |
| ↑↓←→ | 选择/翻页 |
| A | 确认/交互 |
| B | 取消/返回 |

---

## 🧩 已知问题

1. **字库缺失**: `fontglyph_tw.h` 未包含所有汉字，中文字体名可能不显示
2. **商城物品名**: 部分物品使用 `PAL_GetWord(ID)` 可能返回空，需显示 `#ID`
3. **静电容值**: 不同 Xcode 版本的设备 ID 可能变化
4. **召唤兽**: 目前只在战斗中自动攻击，没有战场精灵和地图跟随

---

## 🔄 文件恢复

如果不小心把文件改坏了，从 GitHub 恢复原始版：

```bash
curl -sL "https://raw.githubusercontent.com/sdlpal/sdlpal/master/FILENAME.c" -o /Users/admin/Downloads/PAL/sdlpal-master/FILENAME.c
```

替换 `FILENAME` 为需要的文件名（`play`、`scene`、`uigame`、`uibattle`、`fight`、`global` 等）。

---

## ⚠️ 注意事项

1. **签名团队不要改**：`DEVELOPMENT_TEAM = 6Q2EK738ZP`
2. **编译参数不要省**：必须加 `-allowProvisioningUpdates`
3. **安装前先 build**：install 命令需要 build 产物路径
4. **GAMEMENU_LABEL_CHEAT = 607** 定义在 `ui.h`，已在 `PAL_GetWord` 中特殊处理
