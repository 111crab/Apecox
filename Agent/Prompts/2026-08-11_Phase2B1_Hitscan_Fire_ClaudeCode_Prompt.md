# Claude Code 执行 Prompt：Phase 2B-1 自动步枪腰射 Hitscan

## 你的身份与协作边界

你是 Apecox 项目的具体实施代理。Codex 负责架构规划、代码审查、验证方案和 Git 收口；用户负责关键命名审阅与 UE 编辑器人工配置。

本窗口是新的 Claude Code 上下文时，也必须先阅读下列文件，不得依据旧 Aura/Apex 项目代码猜测：

```text
D:/UnrealProject/Apecox/Agent/README.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Working_Agreement.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Subagent_Review_Checklist.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Phase.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md
D:/UnrealProject/Apecox/Agent/02_CombatFramework/Apecox_Combat_Framework_Architecture_RFC.md
D:/UnrealProject/Apecox/.agents/ue-project-context.md
```

参考实现只用于理解职责，不得整段照搬：

```text
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.h
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Weapons/LyraRangedWeaponInstance.h
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Weapons/LyraWeaponStateComponent.h
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp
```

Lyra 中 `bIsTargetDataValid = true` 不是 Apecox 的安全口径，禁止照搬。

## 本轮目标

实现第一把自动步枪的可复用腰射 Hitscan C++ 闭环：

```text
WhileInputActive 输入
-> 一次 GA 激活处理一发
-> 客户端预测开火并发送自定义 TargetData
-> 服务端验证来源、Shot ID、RPM、弹药和瞄准意图
-> 服务端两阶段 Trace
-> Authority 扣弹、应用配置 Damage GE
-> GameplayCue + Client Shot Confirmation
```

本轮不创建或修改任何 `.uasset/.umap`，不使用 UE MCP，不生成 `.sln`，不执行 Git commit/push。

## 必须保留的现有事实

- ASC 由 `AApecoxPlayerState` 持有，`AApecoxPlayerCharacter` 是 Avatar。
- 武器运行时 UObject 的 Outer 为 PlayerController，并由 Inventory Registered Subobject 路径复制。
- 当前装备由 Character 上的 `UApecoxEquipmentComponent` 管理。
- 装备时 `UApecoxAbilitySet` 已将 `UApecoxWeaponInstance` 作为 AbilitySpec `SourceObject`。
- 武器私有实例和弹匣只对 Owner 可见；远端公开表现继续由 EquippedWeaponState 驱动。
- `UApecoxAbilitySystemComponent` 已实现 Pressed/Held/Released 输入缓存和 `WhileInputActive`。
- 新增业务 C++ 必须使用 `Public/Private` 镜像目录。
- 注释必须同时解释“代码做什么”和关键设计“为什么这样做”，但不要逐行复述。

## 批准的公开类型与文件

新增头/源文件，目录必须镜像：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponFireConfig.h
Source/Apecox/Private/Weapons/ApecoxWeaponFireConfig.cpp

Source/Apecox/Public/Weapons/ApecoxRangedWeaponInstance.h
Source/Apecox/Private/Weapons/ApecoxRangedWeaponInstance.cpp

Source/Apecox/Public/Weapons/ApecoxWeaponStateComponent.h
Source/Apecox/Private/Weapons/ApecoxWeaponStateComponent.cpp

Source/Apecox/Public/AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.h
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.cpp

Source/Apecox/Public/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.h
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp

Source/Apecox/Public/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.h
Source/Apecox/Private/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.cpp
```

批准的类型：

```text
FApecoxWeaponFireConfig
FApecoxHitscanFireConfig : FApecoxWeaponFireConfig
UApecoxRangedWeaponInstance : UApecoxWeaponInstance
UApecoxWeaponGameplayAbility : UApecoxGameplayAbility
UApecoxHitscanFireAbility : UApecoxWeaponGameplayAbility
FApecoxHitscanShotTargetData : FGameplayAbilityTargetData_SingleTargetHit
EApecoxShotConfirmation
UApecoxWeaponStateComponent : UControllerComponent
```

可修改：

```text
Source/Apecox/Public/Inventory/ApecoxInventoryItemInstance.h
Source/Apecox/Public/Weapons/ApecoxWeaponDefinition.h
Source/Apecox/Private/Weapons/ApecoxWeaponDefinition.cpp
Source/Apecox/Public/Player/ApecoxPlayerController.h
Source/Apecox/Private/Player/ApecoxPlayerController.cpp
Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h
Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp
Source/Apecox/Apecox.Build.cs（仅编译证明存在真实依赖时）
```

如果正确实现确实还需要修改现有 `.cpp`（例如 UObject 初始化/复制路径），先确认属于本轮职责，再最小修改，并在报告中单独说明原因。禁止顺手重构无关代码。

## 1. 类型化 FireConfig

在 `ApecoxWeaponFireConfig.h` 中建立：

```text
FApecoxWeaponFireConfig
  MagazineCapacity
  RoundsPerMinute
  GameplayFireOriginOffset

FApecoxHitscanFireConfig
  MaxRange
  TraceChannel
  DamageEffectClass
```

要求：

- 使用 `USTRUCT(BlueprintType)` 和清晰的 `ClampMin/UIMin` 元数据。
- `MagazineCapacity > 0`、`RoundsPerMinute > 0`、`MaxRange > 0` 才有效。
- `GameplayFireOriginOffset` 是 Gameplay Origin，不读取 FP/TP Weapon Mesh Socket 作为 Dedicated Server 真相。
- `DamageEffectClass` 允许空用于只测 Trace，但空时不得崩溃或伪称造成伤害；报告和日志必须明确。
- `UApecoxWeaponDefinition` 添加 `TInstancedStruct<FApecoxWeaponFireConfig> FireConfig`，include 使用 UE 5.8 `StructUtils/InstancedStruct.h`。
- 提供公共基类访问和类型安全模板访问；错误类型返回空，不做 reinterpret/static 强转。
- 不创建 FireMode GameplayTag，不开放任意 Fragment/Step 数组。

## 2. RangedWeaponInstance

`UApecoxInventoryItemInstance::Initialize()` 改成 virtual；`UApecoxRangedWeaponInstance` override：

- 必须先调用 `Super::Initialize()`。
- 只在 Authority 创建路径从有效 FireConfig 初始化 `CurrentMagazineAmmo`。
- `CurrentMagazineAmmo` 复制并提供 `OnRep`/只读 Getter，为未来 HUD 保留入口。
- 客户端预测射击只更新本地射速时间与 Shot ID，不权威修改弹匣。
- 服务端 `CommitServerShot()` 才能更新 `LastServerFireTimeSeconds`、`LastAcceptedShotId` 并扣一发。
- 合法 Miss 也消耗弹药；Rejected 不消耗。
- Shot ID 必须非零，处理 `uint32` 回绕，拒绝重复/倒序；不要把 Shot ID 做成 GameplayTag。
- 射速间隔由 `60.0 / RoundsPerMinute` 计算，比较时允许很小的浮点容差，不允许客户端通过高频激活绕过 Authority。
- 设计 Listen Server Host 路径，避免本地预测和 Authority 在同一个 UObject 上双扣弹。

建议公开/保护函数名遵守 `Current_Code_Design.md`；如果 UE API 约束迫使更名，先在报告中列为待审核，不得静默创建另一套概念。

## 3. 武器 GA 公共基类

`UApecoxWeaponGameplayAbility` 必须：

- 从 AbilitySpec 的 `SourceObject` 取得 `UApecoxWeaponInstance`。
- `CanActivateAbility()` 使用传入的 `Handle` 和 `ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle()`；禁止激活前依赖 `GetCurrentAbilitySpec()`。
- 验证 SourceObject 正是当前 `UApecoxEquipmentComponent::GetCurrentWeaponInstance()`。
- 武器已卸下、Spec/SourceObject 无效或类型不符时安全拒绝。
- 不缓存跨装备生命周期的裸 WeaponInstance 指针。

## 4. 自定义 TargetData

`FApecoxHitscanShotTargetData` 继承 `FGameplayAbilityTargetData_SingleTargetHit`，添加：

```text
uint32 ShotId
float ClientFireTimeSeconds
FVector_NetQuantize10 ViewOrigin
FVector_NetQuantizeNormal AimDirection
```

要求：

- override `GetScriptStruct()`。
- 实现 `NetSerialize()`，先序列化父类，再序列化新增字段，正确合并 `bOutSuccess`。
- 声明 `TStructOpsTypeTraits` 的 `WithNetSerializer = true`。
- 服务端读取 TargetData 时先验证数量、空指针和 ScriptStruct，再转换；禁止无类型检查的 `static_cast`。
- 继承的 HitResult 只作为客户端候选，不直接用于 Authority 伤害。

## 5. HitscanFireAbility 生命周期

构造函数固定：

```text
ActivationPolicy = WhileInputActive
NetExecutionPolicy = LocalPredicted
InstancingPolicy = InstancedPerActor
ActivationGroup = Independent
```

`ActivateAbility()`：

1. 调用父类并验证 RangedWeaponInstance、Hitscan 配置和当前装备关系。
2. 对称注册 `AbilityTargetDataSetDelegate(CurrentSpecHandle, PredictionKey)`。
3. 如果是 Owning Local Player，使用 PlayerController ViewPoint 构造本地候选 Trace：忽略 Avatar，自相交安全；生成 Shot ID 和客户端时间。
4. 记录未确认射击并立即触发预测 `GameplayCue.Weapon.Fire`。
5. 在 Prediction Window 中把 TargetData 发送给服务器。
6. Remote Authority 路径等待 Client Replicated TargetData；绑定后调用 `CallReplicatedTargetDataDelegatesIfSet`，覆盖数据先到的情况。

`OnTargetDataReady()`：

- Owning Client 非 Authority：调用 GAS 原生 `CallServerSetReplicatedTargetData()`；不得自定义一个重复的 ServerFire RPC。
- Authority：先执行输入/来源合理性检查，再调用 WeaponInstance 的 `CanCommitServerShot/CommitServerShot`，最后做 Trace/GE/Cue/确认。
- 非 Authority 永远不应用 Damage GE、不正式扣弹、不修改其他玩家 Health。
- TargetData 每次只消费一次；Late/duplicate data 安全拒绝。

`EndAbility()`：

- 对称移除 TargetData delegate。
- 消费 Client Replicated TargetData。
- 清理本次实例临时状态。
- ScopeLock 路径必须安全，不留下 delegate。

## 6. 服务端两阶段 Trace 与验证

V1 当前世界复核，不做历史回滚：

1. 验证 Ability SourceObject 仍是当前装备实例。
2. 验证 `ShotId`、RPM、弹匣和配置。
3. 使用 Authority Pawn 的 `GetPawnViewLocation()` 与 `GetBaseAimRotation()` 获取权威视点/方向。
4. 客户端 ViewOrigin 必须位于权威视点的有限容差内；AimDirection 必须与权威方向处于合理角度内。容差作为 C++ 中有名字的保守常量集中定义，不加入每武器配置，也不做 GameplayTag。
5. 第一次从权威视点沿合法 AimDirection Trace 到 `MaxRange`，取得意图点。
6. 使用 Authority Avatar Transform / BaseAimRotation 和 `GameplayFireOriginOffset` 构造 Gameplay Fire Origin。
7. 第二次从 Gameplay Fire Origin 指向意图点 Trace，最终 HitResult 才是服务器命中真相。
8. Trace 忽略 Avatar；不得依赖本地 CameraComponent、FP/TP Weapon Mesh、渲染 Socket 或 PlayerCameraManager 存在。
9. 通过所有请求验证后再 `CommitServerShot()`；合法 Miss 也扣弹。
10. 只有最终命中 Actor 具有 ASC 且 `DamageEffectClass` 有效时，服务端构造 Spec 并应用一次 GE。

执行顺序要避免：扣弹后发现请求本身非法；或先应用伤害后才发现 Shot ID 重复。

## 7. Shot Confirmation 与 GameplayCue

`EApecoxShotConfirmation`：

```text
Rejected
Miss
ConfirmedHit
```

`UApecoxWeaponStateComponent : UControllerComponent`：

- 在 `AApecoxPlayerController` 构造函数中 `CreateDefaultSubobject`，默认复制。
- 本地保存未确认 Shot ID 和候选是否命中；不能无限增长，确认/超时/Controller 销毁时清理。
- `ClientConfirmShot(ShotId, Result)` 使用 Client RPC 返回最终结果，并提供一个未来 HUD 可订阅的原生/动态委托或只读确认入口。
- 本轮日志应能区分 Predicted、Rejected、Miss、ConfirmedHit 以及 Authority/Client，但不要每 Tick 打印。
- 它不执行 Trace、不扣弹、不应用 GE。

新增 Native Tags：

```text
InputTag.Weapon.Fire
GameplayCue.Weapon.Fire
GameplayCue.Weapon.Impact
```

要求：

- `InputTag.Weapon.Fire` 只用于 IA -> AbilitySpec 路由。
- Fire Cue 在本地预测并由服务器认可路径向远端表达；必须利用 GAS PredictionKey/GameplayCue 语义避免 Owning Client 因客户端+服务器各执行一次而明显双播。
- Impact Cue 只由 Authority 最终 HitResult 触发，参数至少包含 Location、Normal、Instigator/EffectCauser 或 SourceObject 中可安全提供的字段。
- Cue 只负责表现，不应用伤害或修改弹药。
- 当前没有 GameplayCueNotify 资产也必须能够构建、射击和日志验证；不要用空资产阻断代码流程。

## 8. 伤害边界

- 本轮通过 `FApecoxHitscanFireConfig::DamageEffectClass` 指向编辑器创建的 Instant GE。
- GE 首版可以直接对 Health 做固定负 Modifier，目的只是验证端到端伤害和死亡重生。
- 不创建 SetByCaller Damage Tag，不实现正式伤害公式、Damage Meta Attribute、Shield、护甲、穿透、爆头或队伍规则。
- 命中没有 ASC 的世界物体时可以播放 Impact Cue，但 ShotConfirmation 为 `Miss`；只有成功向有效目标应用 GE 才是 `ConfirmedHit`。

## 9. 构建与静态检查

实现后运行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

如果 UE/Rider 正在占用导致构建无法开始，先报告，不要结束或杀死用户进程。

重点静态检查：

- `Public/Private` 镜像目录正确。
- 所有 UObject/Component/TargetData 反射与 include 能通过 UE 5.8 UHT/IWYU。
- TargetData delegate、replicated data 和 EndAbility 清理对称。
- SourceObject 从传入 SpecHandle 查找。
- WeaponInstance subobject 的 Ammo 复制路径成立。
- Host 不双扣弹，Client 不改权威 Health。
- 重复/倒序/超射速 Shot ID 不重复伤害。
- Dedicated Server 不依赖视觉组件。

## 10. 禁止事项

- 不修改 `.uasset/.umap`。
- 不调用 UE MCP；本轮所有 UE 资产配置由用户在代码审查后手工完成。
- 不生成项目文件或 `.sln`。
- 不运行 Git add/commit/push/reset/checkout。
- 不实现 ADS、Reload、备用弹药、散布、后坐力、命中标记 UI、Montage、Projectile、SSR、Shield 或正式伤害系统。
- 不复制 Lyra 的 GameFeature、Experience、Equipment 全套。
- 不创建万能 Weapon Fragment、任意 Step 流程或一个武器一个 GA 类。
- 不因编译错误重写已验证的 Inventory/Equipment/ASC 生命周期。

## 11. 报告

完成后创建中文报告：

```text
D:/UnrealProject/Apecox/Agent/Reports/2026-08-11_Phase2B1_Hitscan_Fire_Report.md
```

必须包含：

1. 修改/新增文件列表。
2. 新增类、结构体、枚举、公开函数、关键成员变量及作用。
3. 实际射击网络链路，分别说明 Owning Client、Authority、Remote Client。
4. TargetData 和 Shot ID 如何避免重复结算。
5. Client 预测了什么、没有预测什么；Server 验证了什么。
6. 与 Lyra 相同和不同的部分，特别说明没有照搬 `TargetData 恒有效`。
7. 构建命令和完整结果。
8. 尚未执行的 UE 人工资产配置。
9. 任何偏离 Prompt 的地方、原因和风险。
10. 下一步人工验证建议，但不要自行修改 UE 资产。

执行到 C++ 构建和中文报告完成后停止，等待 Codex 审查。不要继续实现后续功能。

