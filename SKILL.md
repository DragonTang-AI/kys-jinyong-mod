---
name: aqara-life-h5-plugin-generation
description: 生成或修改符合当前 AqaraLife App 桥契约的 H5 插件工程，默认只依赖 Skill 包内的 canonical 规则、模板、SDK 和脚手架，自动完成工程生成、打包、自检和宿主验证步骤整理。适用于用户提到生成 H5 插件、灯控面板插件、灯光互动/游戏插件、`plugin.zip`、AqaraLife App 插件、插件无法在宿主中运行或需要修复 bridge 适配时使用。
---

# AqaraLife H5 Plugin Generation

这是一个 **完整文件包 Skill**，不是单独依赖 `SKILL.md` 的单文件 Skill。使用时必须连同以下目录一起分发：

- `assets/`
- `references/`
- `scripts/`

先读取以下文件：

1. `references/host-dependencies.md`
2. `references/generation-playbook.md`
3. `references/canonical-implementations.md`

收尾时再读取：

- `references/review-checklist.md`

`references/native-source-map.md` 仅作为维护者附录，不属于公开平台 Agent 的默认执行路径。

## 推荐最简触发方式

对外使用时，优先接受这类短指令，不要要求用户重复描述内部流程：

- `用 aqara-life-h5-plugin-generation 生成一个灯控面板插件，名字叫 light-demo`
- `生成一个灯光互动插件，名字叫 jump-demo，输出到 ./output`
- `帮我做一个 AqaraLife App 可运行的 H5 灯控插件`

只要用户已经表达“要生成什么插件”，就默认由 Skill 自己补全生成、打包、自检和验证说明。

## 默认自动执行

除非用户明确要求关闭，否则默认自动完成：

- 选择最合适的内置 archetype 和 bridge 形态
- 生成完整插件工程
- 复用 Skill 自带真实 `al-h5-sdk.js`
- 产出 `dist/plugin.zip`
- 按 `references/review-checklist.md` 做自检
- 在最终回复中附上宿主 App 验证步骤清单

## 最少追问策略

优先推断，不要轻易反问。默认规则：

- 插件类型可从用户语义推断时，不追问
- 插件名未给出时，按插件类型生成一个稳定默认名
- 输出目录未给出时，默认使用 `./output/<plugin-name>`
- 面板类默认走 `device mode + snapshot bridge`
- 互动/游戏类默认走 `command bridge`；若用户明确要联动真实设备，则默认 `device mode`，否则默认 `demo mode`

只有在以下情况才追问：

- 无法判断是面板类还是互动/游戏类
- 用户要求的设备品类超出当前支持范围
- 用户给出的约束彼此冲突
- 当前任务缺少会直接影响生成结果正确性的关键业务信息

## 内置 Canonical 资产

默认优先使用 Skill 自带资产，不要先去外部仓库找参考文件：

- 初始化脚本：`scripts/init-plugin.js`
- 真实运行时 SDK：`assets/runtime/al-h5-sdk.js`
- 灯控面板 bridge 模板：`assets/templates/light-panel/device-bridge.js`
- 灯控面板 i18n 模板：`assets/templates/light-panel/i18n.js`
- 灯光互动/游戏 bridge 模板：`assets/templates/jump-game/device-bridge.js`

这些资产已经覆盖公开平台开发者生成插件时最关键、最不该漂移的内容：

- 工程骨架
- 真实 bridge runtime
- 灯光面板 bridge 实现骨架
- 灯光互动/游戏 bridge 实现骨架
- few-shot 级别的 canonical 行为约束

## 默认目标

默认目标是产出一个可运行在 **当前 AqaraLife App 桥契约** 下的 H5 插件。

产物必须满足：

- 可被 AqaraLife 原生 WebView 本地加载
- 可通过当前 AqaraLife 原生 bridge 契约识别并与设备交互
- 若当前宿主环境存在构建链，可执行构建并产出 `dist/plugin.zip`
- 若当前宿主环境不存在构建链，也要能手工产出 AqaraLife App 可消费的 `dist/plugin.zip`

## 当前支持范围

当前只稳定支持两类、且都收敛到 **灯光设备谱系**：

- 灯控面板类
  - 设备能力集中在 `Output`、`LevelControl`、`ColorControl`
  - 典型 trait 包括 `OnOff`、`CurrentLevel`、`ColorTemperature`、`Hue`、`Saturation`、`ColorMode`
- 灯光联动的互动/游戏类
  - 仍以同一类灯光设备为控制目标
  - 允许把 UI 或游戏事件映射成灯光颜色、开关等命令

暂不把以下类型当成当前 Skill 的稳定目标：

- 门锁
- 窗帘
- 温控
- 传感器看板
- 多端点、多品类混控
- 未在内置 canonical 模板中覆盖的新设备族

如果用户要求超出上述范围，不要硬生成“看起来像能跑”的实现；应先说明超出当前受支持品类，或退回到方案设计/规范补充。

## 执行顺序

1. 先判断插件类型：
   - `device mode` 还是 `demo mode`
   - `snapshot bridge` 还是 `command bridge`
   - 控制面板类还是游戏/互动类
2. 优先使用 Skill 自带资产：
   - 先看 `references/canonical-implementations.md`
   - 再用 `scripts/init-plugin.js` 建立工程骨架
   - 再复用 `assets/runtime/al-h5-sdk.js`
   - 再按插件类型挑选 `assets/templates/*`
3. 如果允许执行脚本，优先运行：
   - `node scripts/init-plugin.js <plugin-name> --output <dir> --template blank|light-panel|jump-game`
4. 如果当前宿主不方便执行脚本，再手工生成等价目录结构与文件
5. 将脚手架视为“目录与基础脚本生成器”，优先信任这些产物：
   - `package.json`
   - `plugin.config.js`
   - `start-server.sh`
   - `start-server.bat`
   - 基础目录结构
   - `src/al-h5-sdk.js`
6. 必须生成或补全以下文件：
   - `src/index.html`
   - `src/app.js`
   - `src/device-bridge.js`
   - `src/i18n.js`
7. `src/al-h5-sdk.js` 默认直接复制 `assets/runtime/al-h5-sdk.js`，不要自行改写底层
8. 如需要外联样式或资源，再新增：
   - `src/styles.css`
   - `src/assets/*`
9. 若新增文件，更新 `plugin.config.js` 的 `build.copyFiles`
10. 执行验证：
   - 如果当前宿主有构建链，执行构建
   - 如果没有构建链，手工生成可运行的 `dist/` 和 `dist/plugin.zip`

## 硬性规则

- 不要自行重写 SDK 底层；优先使用 `assets/runtime/al-h5-sdk.js`
- 不要把任何外部仓库路径当成默认前提
- 所有设备相关 HTTP 请求都必须走 `queryTransfer`，不能直接 `fetch`
- `queryTransfer` 的 `modeType` 固定为 `'sdkMode'`
- OpenAPI 路径使用完整前缀 `/open/api/v1/...`
- H5 入口默认固定为 `src/index.html`
- 脚本加载顺序固定为：
  - `al-h5-sdk.js`
  - `device-bridge.js`
  - `i18n.js`
  - `app.js`
- `device mode` 下缺失 `did` 要明确报错；`demo mode` 下要允许缺失并降级运行
- 只使用原生稳定支持的桥消息格式：
  - `methodName%%jsonParams`
  - `{ action: "methodName", data: {...} }`
- 不要把任意 `window.ReactNativeWebView.postMessage(JSON.stringify({ type: ... }))` 当成稳定原生契约
- 灯控类涉及 `ColorTemperature` 时，UI 层与接口层之间必须处理 Kelvin 与 mired 的双向转换
- 当前只在已验证的灯光设备品类内生成实现；不要对未覆盖设备族臆造 endpoint、functionCode、traitCode

## 生成策略

如果用户没有明确指定实现谱系，按以下规则决定：

- 面板、滑块、开关、trait 映射：优先参考内置 `light-panel` archetype
- 游戏、颜色同步、页面关闭、轻量命令：优先参考内置 `jump-game` archetype
- 需要首屏读取真实设备状态并驱动 UI：走 `snapshot bridge`
- 主要是命令触发、允许脱离设备演示：走 `command bridge`
- 面板类默认 `device mode + snapshot bridge`
- 互动/游戏类默认 `command bridge`，再根据是否控制真实设备决定 `device mode` 或 `demo mode`

生成前默认优先读取 Skill 自带的：

- `assets/templates/light-panel/device-bridge.js`
- `assets/templates/light-panel/i18n.js`
- `assets/templates/jump-game/device-bridge.js`
- `references/canonical-implementations.md`

## 输出要求

完成实现后，最终回复应至少说明：

- 生成或修改了哪个插件目录
- 该插件属于哪种模式与 bridge 形态
- 目标设备品类是否仍在当前受支持范围内
- 是否已替换脚手架基础模板中的业务实现
- 是否已完成构建验证
- 是否已产出 `dist/plugin.zip`
- 自检结果是否通过
- 是否已使用 Skill 自带真实 `al-h5-sdk.js`
- 宿主 App 验证步骤清单
- 若仍有风险，风险具体在哪个 bridge 或 trait 映射点

## 参考入口

- 宿主依赖与完整包要求：`references/host-dependencies.md`
- 生成规范：`references/generation-playbook.md`
- Canonical 实现参考：`references/canonical-implementations.md`
- 收尾检查：`references/review-checklist.md`
- 维护者附录：`references/native-source-map.md`
