# Apecox 初始项目盘点

盘点日期：2026-08-05
项目路径：`D:/UnrealProject/Apecox`

## 结论

Apecox 是一个足够干净的 UE 5.8 C++ 冷启动项目，适合重新建立架构。当前没有需要兼容的业务代码，也没有已有玩法资产形成的耦合。

真正需要先处理的是工程基线，而不是马上写 GAS：

- 建立项目自有默认地图。
- 关闭当前无价值的光追和 Substrate。
- 显式启用官方 GAS，并确认是否保留 Epic Experimental GASToolsets。
- 确认内容目录与 Git/LFS 策略。
- 之后再设计 Gameplay Framework 类。

## 根目录

存在：

- `Apecox.uproject`
- `Apecox.sln` / `Apecox.slnx`
- `Automation_Apecox.sln` / `Automation_Apecox.slnx`
- `Source`、`Config`、`Content`
- `Binaries`、`DerivedDataCache`、`Intermediate`、`Saved`、`.vs`

当前不存在：

- Git 仓库。
- Agent 工作区（本轮已新建）。
- Gameplay Framework 业务类。
- GAS 运行时代码。

Automation 解决方案是 UBT 辅助输出，不是 Rider 应打开的主项目方案。

## uproject

- EngineAssociation：`5.8`
- Runtime Module：`Apecox`
- `ModelingToolsEditorMode`：仅编辑器启用。
- `GASToolsets`：已启用；位于 `Engine/Plugins/Experimental/Toolsets/GASToolsets`，由 Epic 提供，仅 Editor 加载，依赖 `GameplayAbilities` 与 `ToolsetRegistry`。当前建议禁用，避免无需求的实验性编辑器依赖。

## Source

`Source/Apecox` 当前只有：

- `Apecox.Build.cs`
- `Apecox.cpp`
- `Apecox.h`

`Build.cs` 公共依赖：

- Core
- CoreUObject
- Engine
- InputCore
- EnhancedInput

尚未加入 GameplayAbilities、GameplayTags、GameplayTasks、NetCore 或 UI 等业务依赖。这是合理的空白状态。

后续开始业务代码时，再建立 `Public` / `Private` 对称目录，不为当前三个主模块文件做无意义移动。

## Config

### 地图

`GameDefaultMap=/Engine/Maps/Templates/OpenWorld`。

问题：它属于 Engine 模板，不是项目资产，无法承载 Apecox 的 GameMode、测试场和版本历史。

### 输入

`DefaultInput.ini` 已使用：

- `EnhancedPlayerInput`
- `EnhancedInputComponent`

无需回退到旧 Input 系统。

### 渲染

当前配置：

- DX12 + SM6。
- Lumen GI 和 Reflection。
- Virtual Shadow Maps。
- Mesh Distance Fields。
- Hardware Ray Tracing：开启。
- Substrate：开启。

建议保留前三项，关闭后两项。项目重点是网络、GAS 和射击架构，不是光追或新材质管线。

## Content

当前 Content 文件数为 0，没有形成项目自有玩法资产基线。第三方武器、动画和课程资产尚未迁入，这是好事：可以先确定目录、骨骼和表示层策略，再按垂直切片迁移。

## Git

当前未初始化。由于项目将包含大量二进制资产，初始化前必须先定：

- Git 忽略规则。
- 项目自建 `.uasset/.umap` 是否使用 LFS。
- 第三方资产不进入公开仓库的规则。
- GitHub 新仓库地址。

## 当前风险

- Hardware Ray Tracing/Substrate 带来的显存和 Shader 成本。
- 通过 GASToolsets 间接启用 GameplayAbilities，导致正式运行时依赖不够显式。
- 在产品边界和角色表示未定前过早迁入大量资产。
- 把 Lyra 当模板复制，导致项目规模和复杂度失控。
- 因追求万能配置而提前创建第二套技能解释器。

## 推荐顺序

1. 完成 `Current_UE_Manual_Steps.md` 中的低风险设置。
2. 确认 `/Game/Apecox/...` 目录、GASToolsets 与 Git/LFS。
3. 创建项目自有开发地图并形成初始 Git 基线。
4. 讨论 Phase 1 Gameplay Framework RFC。
5. 批准类名和职责后再开始第一批 C++。
