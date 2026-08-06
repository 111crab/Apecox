# ClaudeCode Prompt - Apex Gameplay Framework 基类归类

生成日期：2026-07-21

## 你的身份与协作背景

你是 ClaudeCode，负责在明确范围内执行 C++ 文件移动、最小声明修正、编译验证并输出中文实施报告。

- 用户负责最终决策、UE 编辑器人工操作和 PIE 验证。
- Codex 负责规划、命名预审、代码审查和经验记录。
- 当前项目是 `Apex`，不是旧项目 `Aura`。

项目路径：

```text
D:\UnrealProject\Apex
```

## 当前任务目标

将项目模板已经生成、并被现有蓝图继承使用的 `AApexGameMode` 与 `AApexPlayerController` 归入长期 `Public/Private` 目录。

本批只做：

1. 使用 `git mv` 移动四个现有源码文件。
2. 为两个移入 `Public/` 的类补充 `APEX_API`。
3. 修正两个 `.cpp` 对自身头文件的 include。
4. 保持所有现有成员、函数、UPROPERTY、行为和蓝图类身份不变。
5. 编译验证并写中文报告。

这是源码归类，不是 GAS 功能实现。

## 必须先读取

```text
D:\UnrealProject\Apex\.agents\ue-project-context.md
D:\UnrealProject\Apex\Agent\00_Coordination\Working_Agreement.md
D:\UnrealProject\Apex\Agent\00_Coordination\Subagent_Review_Checklist.md
D:\UnrealProject\Apex\Agent\00_Coordination\Current_Code_Design.md
D:\UnrealProject\Apex\Source\Apex\ApexGameMode.h
D:\UnrealProject\Apex\Source\Apex\ApexGameMode.cpp
D:\UnrealProject\Apex\Source\Apex\ApexPlayerController.h
D:\UnrealProject\Apex\Source\Apex\ApexPlayerController.cpp
D:\UnrealProject\Apex\Source\Apex\Apex.Build.cs
```

以 `Current_Code_Design.md` 为本次已批准设计。若实际文件与设计不一致，或移动会要求修改范围外文件，请停止扩展并在回复中说明，不得自行改架构。

## 开始前工作区保护

先运行只读检查：

```text
git status --short
```

当前工作区已经可能包含用户或 Codex 的未提交修改，至少可能包括：

```text
Agent/00_Coordination/Current_Phase.md
Agent/00_Coordination/Current_Code_Design.md
Agent/00_Coordination/Decision_Log.md
Agent/Prompts/2026-07-21_Apex_Gameplay_Framework_Classification_ClaudeCode_Prompt.md
Config/DefaultEditor.ini
```

这些改动不是清理目标。不得 reset、restore、checkout、覆盖、删除或格式化它们。只报告观察到的额外改动。

## 允许的文件操作

### 使用 `git mv` 移动

```text
Source/Apex/ApexGameMode.h
-> Source/Apex/Public/GameModes/ApexGameMode.h

Source/Apex/ApexGameMode.cpp
-> Source/Apex/Private/GameModes/ApexGameMode.cpp

Source/Apex/ApexPlayerController.h
-> Source/Apex/Public/Player/ApexPlayerController.h

Source/Apex/ApexPlayerController.cpp
-> Source/Apex/Private/Player/ApexPlayerController.cpp
```

如果目标目录不存在，可以创建：

```text
Source/Apex/Public/GameModes/
Source/Apex/Private/GameModes/
Source/Apex/Public/Player/
Source/Apex/Private/Player/
```

### 只允许修改移动后的四个文件

```text
Source/Apex/Public/GameModes/ApexGameMode.h
Source/Apex/Private/GameModes/ApexGameMode.cpp
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
```

### 只允许新增报告

```text
Agent/Reports/2026-07-21_Apex_Gameplay_Framework_Classification_Report.md
```

若编译确实要求修改其他源码，先停止并报告，不要自行扩大范围。

## 明确禁止

- 不重命名 `AApexGameMode` 或 `AApexPlayerController`。
- 不改变两个类的父类或移除 `UCLASS(abstract)`。
- 不新增或删除成员变量、函数、UPROPERTY、UFUNCTION。
- 不重命名现有成员，避免蓝图序列化配置丢失。
- 不改变 `BeginPlay`、`SetupInputComponent`、`ShouldUseTouchControls` 或构造函数逻辑。
- 不修改 `Apex.Build.cs`、`.uproject`、任何 `.ini` 或 Target 文件。
- 不创建或修改 `.uasset`、蓝图、地图、输入资产或动画资产。
- 不新增 `AApexPlayerState`、ASC、AttributeSet、GA、GE、GameplayTag、DataAsset、GameplayCue。
- 不使用 UE MCP 或其他 MCP 操作编辑器。
- 不主动读取或复制 Lyra、Aura、学习包或第三方项目代码。
- 不重新生成 `.sln` / `.slnx`；按协作约定，项目文件在 Codex 审查和用户确认后统一生成。
- 不执行 Git add、commit、push、reset、restore 或清理命令。

## 精确代码要求

### AApexGameMode

移动后文件：

```text
Source/Apex/Public/GameModes/ApexGameMode.h
Source/Apex/Private/GameModes/ApexGameMode.cpp
```

保持：

```cpp
UCLASS(abstract)
```

类声明只补模块导出宏：

```cpp
class APEX_API AApexGameMode : public AGameModeBase
```

实现文件自身 include 改为：

```cpp
#include "GameModes/ApexGameMode.h"
```

构造函数继续为空壳，不添加默认 Pawn、Controller、PlayerState、GameState、GAS 或游戏规则。

### AApexPlayerController

移动后文件：

```text
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
```

保持：

```cpp
UCLASS(abstract)
```

类声明只补模块导出宏：

```cpp
class APEX_API AApexPlayerController : public APlayerController
```

实现文件自身 include 改为：

```cpp
#include "Player/ApexPlayerController.h"
```

以下成员及其类型、UPROPERTY、访问级别必须保持不变：

```text
DefaultMappingContexts
MobileExcludedMappingContexts
MobileControlsWidgetClass
MobileControlsWidget
bForceTouchControls
```

以下函数及其签名、访问级别和逻辑必须保持不变：

```text
BeginPlay()
SetupInputComponent()
ShouldUseTouchControls() const
```

## Include 与反射要求

- 每个 `.cpp` 首个项目 include 应为自己的新头文件路径。
- `ApexGameMode.generated.h` 与 `ApexPlayerController.generated.h` 名称保持不变。
- 不新增 Core Redirect；模块名和 C++ 类名没有变化。
- 不为 `Public/GameModes` 或 `Public/Player` 修改 `PublicIncludePaths`；UBT 会识别模块 `Public/` 根目录。
- 移动后搜索整个 `Source/Apex`，确认没有旧的 `#include "ApexGameMode.h"`、`#include "ApexPlayerController.h"`，也没有根目录重复文件。

## 注释要求

可以把两个类的模板英文类注释改成简洁中文，但不得借注释调整扩大代码范围。

有价值的注释应说明：

- `AApexGameMode` 是服务端游戏规则入口，本批不承载技能逻辑。
- `AApexPlayerController` 是本地玩家输入与 UI 入口，战斗状态和技能执行不放在这里。
- Mapping Context 继续由当前 Controller 管理，是为了保留已经验证过的输入行为；后续 GAS 输入路由另行设计。

不要逐行翻译代码，不添加未来尚未实现的占位函数。

## 静态复核

移动和修改后执行：

1. 检查旧四个根目录文件已经不存在。
2. 检查新四个文件存在且 Git 识别为 rename 或等价移动。
3. 使用 `rg` 搜索旧 include 和两个类的全部源码引用。
4. 检查 `git diff -- Source/Apex`，确认除路径、两个导出宏、两个自身 include 和可选类注释外没有行为差异。
5. 检查 `git status --short`，报告范围外已有改动，但不要处理。

## 编译验证

不要重新生成项目文件。直接尝试：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如果 UE 编辑器、Live Coding 或环境状态阻止编译，保留完整错误并在报告中说明。不得通过修改配置、关闭进程或扩大源码范围规避。

本批不需要 ClaudeCode 执行 PIE；PIE 由用户在 Codex 审查后完成。

## 中文实施报告

创建：

```text
D:\UnrealProject\Apex\Agent\Reports\2026-07-21_Apex_Gameplay_Framework_Classification_Report.md
```

报告必须包含：

1. 实际移动、修改和新增了哪些文件，是否超出允许范围。
2. `AApexGameMode`、`AApexPlayerController` 的父类、职责和新文件路径。
3. 两个类是否仍为 `abstract`，是否补充 `APEX_API`。
4. `AApexPlayerController` 所有现有成员：名称、类型、UPROPERTY、访问级别和作用。
5. 两个类所有现有函数：签名、访问级别、作用、是否 UFUNCTION。
6. 是否出现任何运行逻辑、序列化字段或公开接口差异。
7. 是否修改 Build.cs、配置、蓝图、地图或资产。
8. 是否新增 GameplayTag、Attribute、DataAsset、GA、GE、Cue、ASC。
9. 编译命令、结果和关键错误（如有）。
10. 开始和结束时的相关 `git status --short` 摘要；明确未处理的范围外改动。
11. 尚未执行的步骤：Codex 审查、用户确认、项目文件重新生成、蓝图父类检查和 PIE 冒烟验证。
12. 需要 Codex 重点审查的风险或设计偏差。

## 完成后回复

简要回复：

- 是否编译成功。
- 四个文件是否全部移动到目标目录。
- 是否只补充导出宏、include 和可选职责注释，没有行为变化。
- 是否完全遵守范围。
- 报告文件路径。
- 若失败，失败原因和停止位置。

