# ClaudeCode Prompt - AimTargetData 命名收口

项目：`D:\UnrealProject\Apex`

用户已批准以下纯命名调整。不得改变任何运行行为。

## 允许修改

```text
Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

不得修改其他源码或 UE 资产，不使用 MCP，不执行 Git 操作。

## 精确重命名

```text
FWaitAimTargetDataDelegate
    -> FApexAimTargetDataReceivedDelegate

FWaitAimTargetDataCancelledDelegate
    -> FApexAimTargetDataCancelledDelegate

bTargetConfirmed
    -> bTargetDataResolved
```

要求：

- 更新声明和所有引用。
- 不修改 `AimDataReceived`、`AimDataCancelled` 属性名。
- 不改变 Data/Cancelled、RPC、PredictionWindow 或生命周期逻辑。
- 同步最终实施报告中的 Delegate 和成员名称。

## 自检

```text
rg -n "FWaitAimTargetDataDelegate|FWaitAimTargetDataCancelledDelegate|bTargetConfirmed" Source/Apex Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
rg -n "FApexAimTargetDataReceivedDelegate|FApexAimTargetDataCancelledDelegate|bTargetDataResolved" Source/Apex Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

第一条必须无结果，第二条必须命中新名称。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

若沙箱内因临时目录锁文件权限无法运行，记录真实情况；Codex 会在沙箱外复编译。完成后停止，等待 Codex 核对。
