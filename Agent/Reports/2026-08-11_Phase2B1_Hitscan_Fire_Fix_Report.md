# Phase 2B-1 自动步枪腰射 Hitscan 修复报告

修复日期：2026-08-11  
构建结果：**通过**（31.51s，0 错误，0 新增警告）

## 修复总览

本轮修复 Codex 审查指出的 6 个 P1 阻断项 + Codex 建议的 3 个同步修复项。
修改文件：8 个代码文件 + 1 个设计文档。

---

## 修复 1：Owning Client 当前装备实例复制

**问题**：`CurrentWeaponInstance` 只在 Authority 可用，Owning Client 为 `nullptr`，导致 `IsSourceWeaponCurrentlyEquipped()` 在客户端失败，LocalPredicted Ability 激活被阻断。

**修复**：`UApecoxEquipmentComponent::CurrentWeaponInstance` 改为 OwnerOnly 复制。

| 文件 | 改动 |
| --- | --- |
| [ApecoxEquipmentComponent.h](Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h) | `CurrentWeaponInstance` UPROPERTY 增加 `Replicated`；更新注释 |
| [ApecoxEquipmentComponent.cpp](Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp) | `GetLifetimeReplicatedProps()` 新增 `DOREPLIFETIME_CONDITION(…, COND_OwnerOnly)`；Equip/Unequip 路径新增 `MARK_PROPERTY_DIRTY_FROM_NAME(…CurrentWeaponInstance…)`；新增 `Net/Core/PushModel/PushModel.h` include |

**效果**：Owning Client 能通过 `IsSourceWeaponCurrentlyEquipped()` 比较同一个 replicated WeaponInstance；Simulated Proxy 仍只看 `EquippedWeaponState` 摘要。私有库存和弹匣细节未暴露给其他玩家。

---

## 修复 2：TargetData 内存所有权

**问题**：`FGameplayAbilityTargetDataHandle::Add(&ShotData)` 传入栈变量地址，UE 5.8 内部由 `TSharedPtr` 接管并释放，导致悬空或错误 free。

**修复**：`BuildLocalShotTargetData()` 改为返回堆分配对象（`new FApecoxHitscanShotTargetData()`），调用方编入 Handle 后由 Handle 统一管理生命周期。

| 文件 | 改动 |
| --- | --- |
| [ApecoxHitscanFireAbility.h](Source/Apecox/Public/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.h) | `BuildLocalShotTargetData()` 返回类型从值改为 `FApecoxHitscanShotTargetData*` |
| [ApecoxHitscanFireAbility.cpp](Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp) | 函数内部改为 `new FApecoxHitscanShotTargetData()`，调用方不再传入 `&ShotData` |

**验证**：全仓库 Grok 确认不存在 `TargetDataHandle.Add(&`。

---

## 修复 3：Ability 生命周期重构（一次激活 = 一发结束 + RPM 门控）

**问题（原始代码）**：
1. `CommitAbility()` 在 `ActivateAbility()` 开始时无条件调用，验证失败后无法正确回滚
2. Owning Client 直接发送 TargetData 后永不回调、永不 `EndAbility()`
3. Authority 处理完成后也未调用 `EndAbility()`，Spec 保持 Active
4. RPM 检查在 `ActivateAbility()` 内，ASC 每帧仍然预测激活并网络请求

**修复**：

| 改动 | 细节 |
| --- | --- |
| 新增 `CanActivateAbility()` override | 先调用 Super 检查父类条件；Local 控制端调用 `CanStartLocalShot()` 做 RPM/弹匣门控；Remote Authority 不用本地门控替代 `CanCommitServerShot()` |
| `ActivateAbility()` 不再无条件 `CommitAbility()` | 先验证配置 + ASC + 装备关系，再注册委托；Remote Authority 调用 `CallReplicatedTargetDataDelegatesIfSet()` 等待；Local 路径生成 ShotId + 堆 TargetData → 显式调用统一 `OnTargetDataReady()` |
| `OnTargetDataReady()` 使用 `FScopedPredictionWindow` | 客户端路径：`CommitAbility()` → 记录未确认射击 → 预测 Fire Cue → `CallServerSetReplicatedTargetData()` → `EndAbility()`；Authority 路径：验证 → `CommitAbility()` → `ProcessAuthoritativeShot()` → `EndAbility()` |
| 所有失败路径对称结束 | invalid type / missing source / rejected / commit failure / world/actor invalid 均最终调用 `EndAbility()`，确保 delegate 移除 + TargetData 消费 |
| `EndAbility()` 遵守安全模式 | 先移除 delegate，再 `ConsumeClientReplicatedTargetData`，最后 `Super::EndAbility` |

**函数签名变化**：

```cpp
// 新增
virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const override;

// 返回类型变化
FApecoxHitscanShotTargetData* BuildLocalShotTargetData(float WorldTimeSeconds) const;
// （原返回 FApecoxHitscanShotTargetData）
```

**效果**：长按输入表现为 Ability 在一发结束后变为 Inactive，达到下一 RPM 时间点时由 Held 再次通过 `CanActivateAbility` 激活。

---

## 修复 4：通过 AbilitySystemInterface 获取目标 ASC

**问题**：`HitActor->FindComponentByClass<UAbilitySystemComponent>()` 只搜索 Character 自身组件，找不到 PlayerState 上的 ASC（Apecox 的既定架构是 PlayerState 拥有 ASC）。

**修复**：使用 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor)`。同时 Damage EffectContext 新增 `RangedWeaponInstance` 作为 SourceObject。

| 文件 | 行 | 改动 |
| --- | --- | --- |
| [ApecoxHitscanFireAbility.cpp](Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp) | ProcessAuthoritativeShot | `FindComponentByClass<UAbilitySystemComponent>()` → `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor()` |
| 同上 | ProcessAuthoritativeShot | Damage EffectContext 新增 `EffectContext.AddSourceObject(RangedWeaponInstance)` |

**其他改进**：
- `DamageEffectClass` 为非 Instant GE 时记录明确 Warning 并拒绝应用
- 只有 TargetASC 和有效 Spec 都存在时才标记 `ConfirmedHit`

**验证**：全仓库 Grok 确认不存在 `FindComponentByClass<UAbilitySystemComponent>`。

---

## 修复 5：两阶段 Trace 最终命中语义修复

**问题（原始代码）**：
1. Stage 2 从 FireOrigin 沿意图方向继续完整 MaxRange，可能越过意图点打中后方对象
2. Stage 2 未命中直接回退 Stage 1 相机 HitResult，绕过"枪口被遮挡"检查
3. `Acos` 前未钳制 DotProduct，可能产生 NaN

**修复**：

| 改动 | 细节 |
| --- | --- |
| Stage 2 只 Trace 到 IntentPoint | `SecondTraceEnd = IntentPoint + SafeDirection * 1.0f`（约 1cm 余量） |
| Stage 2 未命中 → Miss | 返回 `EmptyResult`，绝不回退 Stage 1 的相机 HitResult |
| FireOrigin 与 IntentPoint 过近保护 | 距离 < 1cm 时安全返回 Miss |
| Acos 前钳制 DotProduct | `FMath::Clamp(FVector::DotProduct(...), -1.0f, 1.0f)` |

**新增验证常量**（`ApecoxShotValidation` 命名空间）：

```cpp
static constexpr float Stage2EndpointMargin = 1.0f;
static constexpr float MinFireOriginToIntentDistance = 1.0f;
```

---

## 修复 6：Authority 认可后执行 Fire Cue

**问题**：Fire Cue 只在 Owning Client 本地执行。Authority 认可射击后没有执行 Fire Cue，Remote Client 收不到开火表现。

**修复**：抽取空指针安全的 `ExecuteWeaponFireCue()` 辅助函数。

| 调用方 | 时机 | 目的 |
| --- | --- | --- |
| Owning Client（`OnTargetDataReady` 客户端路径） | `FScopedPredictionWindow` 内 | 预测执行一次 Fire Cue |
| Authority（`ProcessAuthoritativeShot` Step 7） | `CommitServerShot()` 成功后 | 执行一次 Fire Cue，Remote Client 通过复制收到 |

**去重机制**：两端使用同一 `PredictionKey`，GAS 自动为 Owning Client 去重。Listen Host 只走 Authority 路径，不会本地预测一次 + Authority 一次。

**新增函数签名**：

```cpp
void ExecuteWeaponFireCue(const FGameplayAbilityActorInfo* ActorInfo,
    const UApecoxRangedWeaponInstance* RangedWeaponInstance) const;
```

**Impact Cue** 继续只由 Authority 最终 Stage 2 HitResult 触发。

---

## 修复 7：移除 StructUtils 弃用依赖

**问题**：UE 5.8 的 `TInstancedStruct` 实现已在 CoreUObject，旧 `StructUtils` 插件标记 `DeprecatedEngineVersion = 5.5`。

**修复**：

| 文件 | 改动 |
| --- | --- |
| [Apecox.Build.cs](Source/Apecox/Apecox.Build.cs) | PublicDependencyModuleNames 删除 `"StructUtils"` |
| [Apecox.uproject](Apecox.uproject) | Plugins 数组删除 `{"Name": "StructUtils", "Enabled": true}` 条目 |

**保留 `ModularGameplay`**：`UControllerComponent` 仍然需要它。

**验证**：`StructUtils/InstancedStruct.h` 位于 `Engine/Source/Runtime/CoreUObject/Public/StructUtils/InstancedStruct.h`，属于 CoreUObject 模块。构建通过，`TInstancedStruct` 正常编译。

---

## 修复 8：加固服务端弹匣提交

**问题**：`CommitServerShot()` 只靠调用者约定，未检查 Authority，也未二次保护弹匣下界。

**修复**：

| 改动 | 细节 |
| --- | --- |
| Authority guard | `GetTypedOuter<AActor>()` → `HasAuthority()`，非 Authority 记录 Warning 并返回 false |
| 修改前重检条件 | 内部再次调用 `CanCommitServerShot()`，防止调用方跳过 CanCommit 直接调用 Commit |
| 返回类型改为 `bool` | 调用方（`ProcessAuthoritativeShot`）只在 `true` 时才继续伤害和 Fire Cue/Impact Cue |
| 设计文档更新 | `Current_Code_Design.md` 更新 `CommitServerShot` 描述 |

**函数签名变化**：

```cpp
// 原：void CommitServerShot(uint32 ShotId, float WorldTimeSeconds);
// 新：
bool CommitServerShot(uint32 ShotId, float WorldTimeSeconds);
```

---

## 修复 9：人工配置名称准确

修复报告明确后续人工配置步骤（不操作 .uasset，仅列出）：

```text
/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle
  InstanceClass = UApecoxRangedWeaponInstance
  FireConfig = FApecoxHitscanFireConfig（类型选择器）
  FireConfig.MagazineCapacity = 30
  FireConfig.RoundsPerMinute = 600.0
  FireConfig.MaxRange = 10000.0
  FireConfig.TraceChannel = ECC_GameTraceChannel1
  FireConfig.GameplayFireOriginOffset = (50, 10, -10)
  FireConfig.DamageEffectClass = 将来配置 GE 资产
```

不再引用不存在的 `BP_RifleDefinition`。

---

## 构建结果

```
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 31.51 seconds
错误：0
新增警告：0
已有警告：1（ApecoxAbilitySystemComponent.cpp:228 NonInstanced deprecation，与本次修改无关）
```

## 静态确认清单

| 检查项 | 结果 |
| --- | --- |
| 全仓库无 `TargetDataHandle.Add(&` | ✅ |
| Hitscan GA 每个激活路径最终都有 `EndAbility()` | ✅ |
| 本地 RPM 门控发生在 `CanActivateAbility()` | ✅ |
| 目标 ASC 不再使用 `FindComponentByClass<UAbilitySystemComponent>` | ✅ |
| Stage 2 Miss 不再返回 Stage 1 Hit | ✅ |
| Authority 已执行认可 Fire Cue | ✅ |
| Equipment `CurrentWeaponInstance` 已 OwnerOnly 复制 | ✅ |
| `.uproject` / `Build.cs` 不含 `StructUtils` | ✅ |
| `ModularGameplay` 保留 | ✅ |

## 仍未完成的 UE 人工配置

项目既定授予架构通过 `UApecoxAbilitySet` → `EquippedAbilitySets` → `GrantToAbilitySystem(ASC, Handles, WeaponInstance)` 管道，
装备时以 WeaponInstance 作为 SourceObject 授予，卸下时按句柄撤销。不需要创建 GA 蓝图或改用 ASC DefaultAbilities。

具体人工步骤：

1. 创建 `UApecoxAbilitySet` DataAsset（资产名遵守 `DA_` 前缀，如 `DA_AbilitySet_Rifle`）。
2. `GrantedAbilities` 直接选择原生 `UApecoxHitscanFireAbility`，`InputTag = InputTag.Weapon.Fire`。
   `UApecoxHitscanFireAbility` 构造函数已固定 `LocalPredicted + WhileInputActive`，无需额外覆盖。
3. 将该 AbilitySet 加入 `/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle` 的 `EquippedAbilitySets`。
4. `DA_Weapon_Rifle.InstanceClass = UApecoxRangedWeaponInstance`。
5. `DA_Weapon_Rifle.FireConfig` 选择 `FApecoxHitscanFireConfig` 并填写参数。
6. `/Game/Blueprints/Inputs/IA_WeaponFire` InputAction 绑定 `InputTag.Weapon.Fire`。
7. `DamageEffectClass` GE 资产创建（必须 `DurationPolicy = Instant`）。

---

# 第二轮修复（2026-08-11 Codex 终审）

第二轮仅修复 Codex 终审发现的 3 个 P1 代码阻断项 + 1 个 P2 报告错误。
不新增玩法功能。第二轮重新构建通过后再由 Codex 终审。

## 第二轮修复 1：所有 TargetData 终止分支必须结束 Ability

**问题**：`OnTargetDataReady()` 在以下分支直接 `return` 不调用 `EndAbility()`：
- `InData.IsValid(0) == false`
- `CurrentActorInfo` 或 ASC 无效
- 非 Owner、非 Authority 的兜底路径

造成 Spec 保持 Active、TargetData 委托残留、Held 输入无法重新激活。

**修复**：将所有三个分支改为调用 `EndAbility()` 后返回。
同时将 `Handle` 和 `ActivationInfo` 提取到函数顶部，使早期返回分支也能安全结束。

文件：[ApecoxHitscanFireAbility.cpp](Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp)

## 第二轮修复 2：EndAbility 先遵守 UE 5.8 的有效性与 ScopeLock 模式

**问题**：派生 `EndAbility()` 先移除 TargetData 委托、消费 TargetData、清空 `CurrentShotId`，最后才调用 `Super::EndAbility()`。UE 5.8 原生 `UGameplayAbility::EndAbility()` 会先执行 `IsEndAbilityValid()` 和 `ScopeLockCount` 检查。当前顺序可能在 Ability 仍处于 ScopeLock 时提前拆掉派生状态。

**修复**（完全匹配 UE 5.8 原生模式）：

1. `IsEndAbilityValid(Handle, ActorInfo)` 无效 → 直接返回。
2. `ScopeLockCount > 0` → `WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &UApecoxHitscanFireAbility::EndAbility, …))` → 返回。
   关键：绑定 `&UApecoxHitscanFireAbility::EndAbility`（非基类），否则延迟执行时跳过派生清理。
3. 不在 ScopeLock 中 → 执行派生清理（移除 delegate、消费 TargetData、重置 CurrentShotId）。
4. 调用 `Super::EndAbility()` 完成通用 GAS 清理。

文件：[ApecoxHitscanFireAbility.cpp](Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp)

## 第二轮修复 3：非 Instant Damage GE 必须真正拒绝应用

**问题**：非 Instant GE 只输出 Warning，但随后的条件 `bHitActor && TargetASC && DamageGE` 仍为真，仍会调用 `ApplyGameplayEffectSpecToTarget()`。与报告中的"拒绝应用并按 Miss 处理"不一致，也可能留下无跟踪的 Active GE。

**修复**：

- 引入明确布尔值 `bValidInstantDamageEffect`：只有 `DamageGE` 存在且 CDO `DurationPolicy == Instant` 才为 `true`。
- 应用条件改为 `bHitActor && TargetASC && bValidInstantDamageEffect`。
- 非 Instant GE 只输出一次明确 Warning，不调用 `MakeOutgoingSpec` 或 `ApplyGameplayEffectSpecToTarget`，该发按 `Miss` 确认。
- 空 `DamageEffectClass` 仍是合法 Miss。

文件：[ApecoxHitscanFireAbility.cpp](Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp)

## 第二轮修复 4：修正报告的人工配置口径

**问题**：首轮报告写了"GA 蓝图创建及 ASC DefaultAbilities 注册"和"GA_RifleFire"，与项目既定 AbilitySet → EquippedAbilitySets → GrantToAbilitySystem 管道冲突。

**修复**：重写"仍未完成的 UE 人工配置"章节，描述正确的 AbilitySet 授予流程：

1. 创建 `UApecoxAbilitySet` DataAsset（`DA_` 前缀）。
2. `GrantedAbilities` 直接选择原生 `UApecoxHitscanFireAbility`，`InputTag = InputTag.Weapon.Fire`。
3. 加入 `DA_Weapon_Rifle.EquippedAbilitySets`。
4. `DA_Weapon_Rifle.InstanceClass = UApecoxRangedWeaponInstance`。

## 第二轮构建结果

```
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 6.35 seconds（增量构建，仅 HitscanFireAbility.cpp 变化）
错误：0
警告：0（新增）
既有警告：1（ApecoxAbilitySystemComponent.cpp NonInstanced deprecation，与此轮无关）
```

## 第二轮静态确认

| 检查项 | 结果 |
| --- | --- |
| 无效/空 TargetData 不直接静默返回 | ✅ |
| `CurrentActorInfo`/ASC 无效时调用 `EndAbility()` | ✅ |
| 非 Owner 非 Authority 兜底路径调用 `EndAbility()` | ✅ |
| 派生 `EndAbility()` 先检查 `IsEndAbilityValid` | ✅ |
| 派生 `EndAbility()` 先检查 `ScopeLockCount`，defer 绑定自身 | ✅ |
| 非 Instant Damage GE 不可能进入 `ApplyGameplayEffectSpecToTarget()` | ✅ |
| 不创建 GA 蓝图，不改用 ASC DefaultAbilities | ✅ |
| 构建 0 错误 | ✅ |
