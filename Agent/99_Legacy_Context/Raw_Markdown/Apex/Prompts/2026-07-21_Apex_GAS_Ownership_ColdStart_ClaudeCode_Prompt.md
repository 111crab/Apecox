# ClaudeCode Prompt - Apex GAS 所有权冷启动

项目：`D:\UnrealProject\Apex`

## 目标

严格按 `Agent/00_Coordination/Current_Code_Design.md` 实现玩家 PlayerState 持有 ASC、PlayerCharacter 作为 Avatar 的初始化闭环，并编译验证。

## 开始前读取

```text
.agents/ue-project-context.md
Agent/00_Coordination/Working_Agreement.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Agent/00_Coordination/Current_Code_Design.md
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Private/GameModes/ApexGameMode.cpp
Source/Apex/Apex.Build.cs
Apex.uproject
```

先运行 `git status --short`。保留所有现有未提交改动，不得 reset、restore、清理或覆盖范围外文件。

## 允许修改

新增：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/Player/ApexPlayerState.h
Source/Apex/Private/Player/ApexPlayerState.cpp
Agent/Reports/2026-07-21_Apex_GAS_Ownership_ColdStart_Report.md
```

修改：

```text
Apex.uproject
Source/Apex/Apex.Build.cs
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Private/GameModes/ApexGameMode.cpp
```

需要修改范围外文件时停止并报告。

## 实施要求

- `UApexAbilitySystemComponent` 继承 `UAbilitySystemComponent`，使用 `APEX_API`，本批不添加业务成员或函数。
- `AApexPlayerState` 继承 `APlayerState` 并实现 UE 自带的 `IAbilitySystemInterface`。
- PlayerState 私有持有 `UApexAbilitySystemComponent`，构造时创建、复制、Mixed 模式、网络更新频率 100。
- `AApexPlayerCharacter` 也实现 `IAbilitySystemInterface`，但只把查询转发给 PlayerState，不创建第二个 ASC。
- 服务器在 `PossessedBy()`、客户端在 `OnRep_PlayerState()` 调用同一个 `InitializeAbilitySystemActorInfo()`。
- ActorInfo 必须是 PlayerState Owner / Character Avatar。
- `AApexGameMode` 默认使用 `AApexPlayerState`。
- `.uproject` 启用 GameplayAbilities；Build.cs 加入 GameplayAbilities、GameplayTags、GameplayTasks。
- 使用简洁中文注释解释 Owner/Avatar、服务器/客户端双初始化原因，不逐行翻译代码。

不得创建 AttributeSet、GA、GE、Tag、输入配置、蓝图或资产；不得使用 MCP；不得生成项目文件；不得执行 Git add/commit/push。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

## 报告

写入：

```text
Agent/Reports/2026-07-21_Apex_GAS_Ownership_ColdStart_Report.md
```

中文报告简要列出：实际文件、类与父类、全部新增成员和函数、接口来源与用途、初始化时序、模块变化、编译结果、范围外改动、未执行的 UE/多人验证。完成后停止，等待 Codex 审查。
