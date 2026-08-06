# ClaudeCode Final Closeout Prompt - Phase 能量弹最后收口

项目：`D:\UnrealProject\Apex`

这是最后一次微型收口。只允许修改：

```text
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

不得修改其他源码或 UE 资产，不使用 MCP，不执行 Git 操作。

## 1. 收口 WaitAimTargetData

调整 `ConfirmAimTarget()`、`CancelAimTarget()` 和 `SendAimTargetData()` 的空指针保护：

- `ConfirmAimTarget()` 不得直接解引用空 Ability / ActorInfo；失败时走 `CancelAimTarget()`。
- `CancelAimTarget()` 自身必须能够安全处理 Ability、ActorInfo 或 ASC 为空，不能为了取消再次空指针崩溃。
- `SendAimTargetData()` 必须先检查 Ability、ActorInfo、ASC、Avatar、World、PlayerController。
- `GetViewportSize()` 后检查 `VX > 0 && VY > 0`，否则 Cancel。
- 只有上述依赖全部有效后，才构造 `FScopedPredictionWindow`。
- Trace 使用已经验证过的 World 和 Avatar，不再重复 `GetWorld()`，SourceLocation 直接使用有效 Avatar。

保留已经正确的：

- Data/Cancelled 双 delegate；
- 预测客户端 TargetData RPC；
- Listen Server 本地广播；
- 服务器安全副本后 Consume；
- `ShouldBroadcastAbilityTaskDelegates()`。

## 2. 覆盖最终实施报告

必须真正覆盖：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

旧报告当前仍含错误内容，完成后不得再出现：

```text
AbilitySystem/Shared
GetRequiredExecutionConfigStruct(BlueprintNativeEvent)
Apex.Build.cs | +StructUtils
Apex.uproject | +StructUtils plugin
Total execution time: 5.42 seconds
```

报告至少包含：

- 最终新增/修改文件；
- 类、枚举、结构体及职责；
- 关键成员变量、函数签名和职责；
- SkillDefinition -> 模板 CDO Data Validation；
- ActivationGroup；
- TargetData 的预测客户端、Listen Server、远端服务器 Data/Cancelled 路径；
- Projectile 的初始化、Owner 忽略、服务器碰撞、GE、Cue 和销毁；
- UE 5.8 移除 StructUtils 依赖的原因；
- 最新真实编译结果；
- 未创建 UE 资产、未 PIE。

## 3. 自检并编译

```text
rg -n "AbilitySystem/Shared|BlueprintNativeEvent|\\+StructUtils|5\\.42" Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
rg -n "Avatar|VX <= 0|VY <= 0|FScopedPredictionWindow" Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
```

第一条必须无结果。

然后执行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

若沙箱内提示 `Build.bat is already running`，这是临时目录锁文件权限造成的误判，不得沿用旧结果或虚构结果；在报告中写明未能复编译。Codex 会在沙箱外最终复编译。

完成后停止，等待 Codex 最终确认。
