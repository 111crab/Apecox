# Apex ProjectileCast 第二轮代码审查

审查日期：2026-07-29

结论：**第一轮问题只修复了一部分，当前仍有 P1，暂不进入 UE 资产配置。**

## 已修复

- `EApexAbilityActivationGroup` 已增加隐藏 `MAX`。
- ASC 已覆写 `NotifyAbilityActivated()` / `NotifyAbilityEnded()`。
- 新 Exclusive Ability 会忽略自身并取消旧 Replaceable。
- `ExclusiveBlocking` 的阻塞判断方向已修正。
- `FApexSkillExecutionConfig` 已恢复纯数据并移到 `AbilitySystem/Data`。
- ProjectileCast 已改为 `NotBlueprintable`，Activate 保持 `final`。
- 每次激活会重置 Event/Data/Projectile 状态。
- LocationInfo 已使用 `HasEndPoint()` / `GetEndPoint()`。
- Event 与 TargetData 两个回调都会调用 `TryExecuteProjectileSpawn()`。
- Impact GE Spec 已在服务端生成投射物前构建并检查。
- Build/Socket/Source ASC 的一部分运行时保护已加入。

## 仍未修复的 P1

### 1. Aim TargetData Task 基本保持第一轮实现

文件：

```text
Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
```

仍存在：

- Listen Server 仍向自己调用 `ServerSetReplicatedTargetData()`。
- 没有 `AimDataCancelled`。
- 没有注册/解除 `AbilityTargetDataCancelledDelegate`。
- Controller、Avatar 或 Deproject 失败时直接 return，GA 永久等待。
- LocationInfo 没有填写 SourceLocation。
- Replicated Data 仍先 Consume，再广播传入引用，存在引用失效风险。
- 没有使用 UE 5.8 推荐的 `CallServerSetReplicatedTargetData()` 分支。

### 2. Projectile 权威碰撞基本保持第一轮实现

文件：

```text
Source/Apex/Public/CombatEntities/Projectile/ApexProjectile.h
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp
```

仍存在：

- `InitializeProjectile()` 仍返回 void，调用者无法处理失败。
- 没有检查 Impact Spec。
- 没有显式设置 Velocity。
- 没有 `IgnoreActorWhenMoving(Owner/Instigator)`。
- `OnProjectileStop()` 仍可把 Owner/Instigator 送入 `HandleImpact()`。
- `HandleImpact()` 不是 Authority-first，客户端仍修改命中状态。
- 客户端碰撞未关闭。
- 仍使用 Overlap 主路径，没有落实服务端 ProjectileMovement Sweep/Blocking 第一命中边界。
- Spawn 函数忽略 Initialize 的成功/失败。

### 3. Montage 加载失败仍会扣费并永久 Active

文件：

```text
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:61
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:71
```

当前先 Commit，随后才 `LoadSynchronous()`；Soft Pointer 非空但加载失败时没有 else 收束。

必须在 Commit 前成功加载 Montage。

## 仍未修复的 P2

### 1. SkillDefinition 没有委托模板 Data Validation

`UApexGameplayAbility::ValidateSkillDefinition()` 和 ProjectileCast override 已存在，但：

```text
UApexSkillDefinition::IsDataValid()
```

没有调用它，也没有检查 ActivationGroup MAX。当前模板校验仍是死代码。

### 2. CanActivateAbility 对缺失 SkillDefinition 仍返回 true

这会让没有 SourceObject 的能力绕过配置、成本、冷却和 Policy。

### 3. 数值有限性不完整

`MaxAimDistance`、Projectile Speed/Lifetime/Radius/Gravity 的 Data Validation 只比较大小，没有统一检查 NaN/Infinity；运行时也未完整保护。

### 4. 修复报告没有更新

报告仍然是第一轮旧内容：

- 仍写旧 `Shared` 路径。
- 仍写 `BlueprintNativeEvent`。
- 仍写“无范围外改动”。
- 编译时间仍是第一轮的 5.42 秒。
- 没有第二轮函数、成员和网络链路说明。

## 新发现：StructUtils 插件在 UE 5.8 已弃用

Codex 实际重新编译得到：

```text
Project 'ApexEditor' depends on plugin 'StructUtils' which was deprecated in 5.5 and will soon be removed.
```

本机 UE 5.8 的：

```text
Engine/Source/Runtime/CoreUObject/Public/StructUtils/InstancedStruct.h
```

已经使用 `COREUOBJECT_API`。项目本来就依赖 CoreUObject，因此：

- 保留 `#include "StructUtils/InstancedStruct.h"`。
- 删除 `Apex.Build.cs` 的 StructUtils 模块依赖。
- 删除 `Apex.uproject` 的 StructUtils 插件启用项。

这修正了第一轮 Prompt 对 UE 5.8 依赖位置的错误判断。

## 编译实测

Codex 执行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project=D:\UnrealProject\Apex\Apex.uproject -WaitMutex
```

结果：

```text
Result: Succeeded
Total execution time: 1.28 seconds
```

但有 StructUtils 弃用警告，且 Runtime P1 尚未解决，因此不能视为通过审查。
