# iOS 真机部署全链路教程

## 📱 目标设备信息

| 项目 | 值 |
|------|-----|
| **设备型号** | iPhone 16 Pro Max |
| **设备名称** | Jingyu的16 Pro Max / Jingyu's 16 Pro Max |
| **设备标识符** | `5CDCE6BC-D43E-59EC-AFD3-7D15E361C446` |
| **系统版本** | iOS 最新 |

---

## 🔐 签名与证书

### 1. 检查可用签名证书

```bash
security find-identity -v -p codesigning
```

如果有类似输出表示可签名：
```
F2F7BAE8A4E59C619372874DC8442A559A014551 "Apple Development: JINGYU MA (K493NUX7J9)"
```

如果没有有效证书，需要先添加 Apple ID 到 Xcode 并登录：
1. 打开 Xcode → Settings → Accounts
2. 点 `+` → 添加 Apple ID
3. 使用个人 Apple ID 登录

### 2. 签名团队

用户有两个团队 ID：
- `6Q2EK738ZP` → **Jingyu Ma**（开发团队，有免费 provision）
- `K493NUX7J9` → 开发证书关联的团队 ID

**始终使用 `6Q2EK738ZP` 作为 DEVELOPMENT_TEAM**。

---

## 🏗️ 通用构建命令

### 对于 Xcode 项目（.xcodeproj / .xcworkspace）

```bash
# 编译签名
xcodebuild -workspace <项目>.xcworkspace \
  -scheme <Scheme名称> \
  -destination 'generic/platform=iOS' \
  -allowProvisioningUpdates \
  build

# 或者对于 .xcodeproj
xcodebuild -project <项目>.xcodeproj \
  -scheme <Scheme名称> \
  -destination 'generic/platform=iOS' \
  -allowProvisioningUpdates \
  build
```

**关键参数解释：**
- `-allowProvisioningUpdates` → 允许 Xcode 自动管理证书和配置文件
- `-destination 'generic/platform=iOS'` → 构建为通用 iOS 设备（不指定具体机型）
- `-scheme` → 必须指定，通常是项目名称

---

## 📲 安装到 iPhone

### 方法一：devicectl（推荐，适配最新 Xcode）

```bash
# 1. 先构建
xcodebuild -workspace <项目>.xcworkspace -scheme <Scheme名称> \
  -destination 'generic/platform=iOS' -allowProvisioningUpdates build

# 2. 找到 .app 产物路径
# 通常在: ~/Library/Developer/Xcode/DerivedData/<项目名>/Build/Products/Debug-iphoneos/<应用名>.app

# 3. 安装到 iPhone
xcrun devicectl device install app \
  --device 5CDCE6BC-D43E-59EC-AFD3-7D15E361C446 \
  /path/to/YourApp.app
```

### 快速安装示例（单条命令，构建+安装一步到位）
```bash
xcodebuild -workspace <项目>.xcworkspace \
  -scheme <Scheme名称> \
  -destination 'generic/platform=iOS' \
  -allowProvisioningUpdates \
  build 2>&1 | tail -3 \
&& \
xcrun devicectl device install app \
  --device 5CDCE6BC-D43E-59EC-AFD3-7D15E361C446 \
  ~/Library/Developer/Xcode/DerivedData/*/Build/Products/Debug-iphoneos/<应用名>.app \
  2>&1 | grep "App installed"
```

### 方法二：xcodebuild install（备选，较慢）
```bash
xcodebuild -workspace <项目>.xcworkspace \
  -scheme <Scheme名称> \
  -destination 'platform=iOS,id=5CDCE6BC-D43E-59EC-AFD3-7D15E361C446' \
  -allowProvisioningUpdates \
  install
```

---

## 🔧 故障排除

### 1. 设备显示 "paired" 但不是 "connected"

`xcrun devicectl list devices` 输出显示 `available (paired)` 但无法安装：

**解决步骤：**
1. 使用 USB 数据线将 iPhone 连接到 Mac
2. 解锁 iPhone（输入锁屏密码）
3. 如果弹出"要信任此电脑吗？"，点"信任"并输入密码
4. 重新执行 install 命令

> ⚠️ `devicectl` 需要 USB 连接，无线连接不可用

### 2. 签名错误 "No Account for Team"

```bash
❌ error: No account for team "6Q2EK738ZP"
```

**解决：** Xcode → Settings → Accounts → 登录对应的 Apple ID

### 3. 签名错误 "Provisioning profile expired"

```bash
❌ error: No provisioning profiles matching "iOS Team Provisioning Profile"
```

**解决：** 确保 `-allowProvisioningUpdates` 参数已传，让 Xcode 自动生成/更新

### 4. 编译错误 "Undefined symbols"

```bash
❌ error: Undefined symbols for architecture arm64
```

**解决：** 检查代码中是否存在：
- 函数声明了未实现
- 重复定义的函数
- 缺失的 `#include` 或 `extern`

### 5. 应用安装后闪退

**可能原因：**
- 代码中有内存访问越界
- 缺少资源文件
- 签名过期

---

## 📋 项目配置快速参考

### Xcode 项目配置建议

```
Target → General → Identity:
  Bundle Identifier: com.yourname.appname（唯一即可）
  Version: 1.0
  
Target → Signing & Capabilities:
  Automatically manage signing: ✅
  Team: 6Q2EK738ZP
  Bundle Identifier: com.yourname.appname
```

### Info.plist 必要配置

```xml
<!-- 设备方向（通常游戏需要） -->
<key>UISupportedInterfaceOrientations</key>
<array>
  <string>UIInterfaceOrientationPortrait</string>
  <string>UIInterfaceOrientationLandscapeLeft</string>
  <string>UIInterfaceOrientationLandscapeRight</string>
</array>

<!-- 文件共享（方便传数据） -->
<key>UIFileSharingEnabled</key>
<true/>
<key>LSSupportsOpeningDocumentsInPlace</key>
<true/>
```

---

## ⏱️ 完整工作流示例

以部署一个新游戏/应用到 iPhone：

```bash
# 1️⃣ 检查设备和证书
echo "=== 设备状态 ==="
xcrun devicectl list devices | grep "16 Pro"
echo "=== 证书状态 ==="
security find-identity -v -p codesigning

# 2️⃣ 编译项目
echo "=== 开始编译 ==="
xcodebuild -workspace <项目>.xcworkspace \
  -scheme <Scheme名稱> \
  -destination 'generic/platform=iOS' \
  -allowProvisioningUpdates \
  build 2>&1 | grep -E "error:|BUILD"

# 3️⃣ 如果编译成功，安装到 iPhone
echo "=== 安装 ==="
xcrun devicectl device install app \
  --device 5CDCE6BC-D43E-59EC-AFD3-7D15E361C446 \
  ~/Library/Developer/Xcode/DerivedData/*/Build/Products/Debug-iphoneos/<应用名>.app \
  2>&1 | grep "App installed"

# 4️⃣ 如果失败，检查错误类型
echo "=== 检查错误 ==="
xcodebuild ... build 2>&1 | grep "error:" | head -10
```

> ⚠️ **重要注意事项：**
> 1. Apple 免费开发者证书有效期 **7 天**，过期后需要重新安装
> 2. 如果使用付费开发者账号（¥699/年），证书有效期为 **1 年**
> 3. 每次修改代码后需要重新构建 → 安装
> 4. iPhone 需要信任开发者证书：设置 → 通用 → VPN与设备管理 → 信任
