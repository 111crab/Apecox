# Apex Phase EnergyBolt 投射物移动运行时修复 Prompt

## 一、你的身份与本次目标

你是 Apex 项目的 C++ 实施子代理。项目根目录：

```text
D:\UnrealProject\Apex
```

本次只修复 Phase EnergyBolt 投射物“服务器日志显示已生成、视觉出现在正确 Socket，但 Actor 不飞行并在 MaxLifetime 后原地消失”的问题。

已经由 Codex 与用户确认：

- Montage、GameplayEvent、Mana Cost、Cooldown 和生成时机均正常。
- `BP_Apex_Projectile_Phase_EnergyBolt` 的粒子组件能够显示。
- `P_Phase_PrimaryTrails3` 会在正确时间和手部位置出现，但不会随投射物飞行。
- 当前根因集中在 `UProjectileMovementComponent` 的初始化/激活时机。
- 不要使用 MCP；本次不修改任何 UE 资产。

## 二、已确认的技术原因

当前实现中：

1. `AApexProjectile` 构造函数将 `ProjectileMovement->bAutoActivate` 设为 `false`。
2. `SpawnProjectileActorOnAuthority()` 在 `FinishSpawning()` 之前调用 `InitializeProjectile()`。
3. `InitializeProjectile()` 在 Actor 尚未完成生成时调用 `ProjectileMovement->Activate()`。
4. UE 5.8 的 `UMovementComponent::RegisterComponentTickFunctions()` / `UpdateTickRegistration()` 会在组件注册阶段依据 `bAutoActivate` 更新 Tick 状态。

因此，过早执行的 `Activate()` 可能在后续组件注册或 Blueprint 构造阶段被抵消，使移动组件没有 Tick。

## 三、允许修改的文件

只允许修改：

```text
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
```

除非编译明确证明声明必须调整，否则不要修改头文件。

禁止：

- 修改 Blueprint、DataAsset、GameplayEffect、GameplayCue、Montage 或 GameplayTag。
- 修改技能数值、碰撞响应、网络策略、能力流程或瞄准逻辑。
- 重构无关代码。
- 生成或修改 `.sln`。
- 执行 Git commit/push。
- 使用 MCP。

## 四、必须实施的修复

### 4.1 显式设置 UpdatedComponent

在 `AApexProjectile::AApexProjectile()` 中，创建 `ProjectileMovement` 后，显式调用：

```cpp
ProjectileMovement->SetUpdatedComponent(CollisionSphere);
```

设计意图：

- 明确由 `ProjectileMovement` 推动根组件 `CollisionSphere`。
- 所有挂在根组件下的表现组件随 Actor 一起移动。
- 不依赖移动组件注册阶段自动寻找根组件。

保留：

```cpp
ProjectileMovement->bAutoActivate = false;
```

因为速度、重力和寿命来自运行时 `UApexProjectileDefinition`，必须在配置完成后才激活。

### 4.2 调整 Deferred Spawn 顺序

在 `UApexProjectileCastAbility::SpawnProjectileActorOnAuthority()` 中，将流程调整为：

```text
SpawnActorDeferred
-> FinishSpawning
-> 检查 Actor 仍然有效
-> InitializeProjectile
-> 初始化失败则 Destroy 并返回 nullptr
-> 成功则返回投射物
```

核心要求：

- 不再于 `FinishSpawning()` 之前调用 `InitializeProjectile()`。
- `InitializeProjectile()` 内现有的速度设置和 `ProjectileMovement->Activate()` 必须发生在 Actor 与组件完成注册之后。
- 保留初始化失败销毁半成品的行为。
- 不要改成普通 `SpawnActor`，继续使用 Deferred Spawn。

### 4.3 增加一条成功初始化日志

在 `AApexProjectile::InitializeProjectile()` 成功完成配置和激活后，增加一条 `LogApex` 的 `Log` 级别日志，至少输出：

- Actor Location
- `ProjectileMovement->Velocity`
- `ProjectileMovement->IsActive()`
- `ProjectileMovement->UpdatedComponent` 名称

日志前缀使用：

```text
[ApexProjectile]
```

示例语义：

```text
[ApexProjectile] 初始化完成：Location=..., Velocity=..., Active=true, UpdatedComponent=CollisionSphere
```

日志只用于证明初始化状态，不要开启 Actor Tick 或每帧输出日志。

## 五、保持不变的行为

- 投射物仍只由服务器权威生成。
- `bReplicates=true` 与移动复制保持不变。
- 客户端仍关闭投射物碰撞。
- Owner 和 Instigator 忽略规则保持不变。
- InitialSpeed、MaxLifetime、CollisionRadius、GravityScale 继续来自 ProjectileDefinition。
- 命中仍由服务器应用 Damage GE、执行 Impact GameplayCue 并销毁 Actor。
- 不改变 `P_Phase_PrimaryTrails3` 或 `P_PhaseProjectileRibbons` 的资产配置。

## 六、注释要求

保留现有中文注释和文件编码。

只在调整后的 Deferred Spawn / 初始化顺序附近添加一条简短中文注释，解释：

> 必须先完成 Actor/组件注册，再配置并激活 ProjectileMovement，避免 bAutoActivate=false 在注册阶段关闭移动 Tick。

不要添加大段教程式注释。

## 七、验证要求

### 7.1 静态审查

确认：

- `SetUpdatedComponent(CollisionSphere)` 在构造函数中执行。
- `FinishSpawning()` 位于 `InitializeProjectile()` 之前。
- 初始化失败仍会 Destroy。
- 没有修改无关文件和资产。

### 7.2 编译

优先使用项目现有构建方式；引擎路径：

```text
E:\UE_5.8
```

目标：

```text
ApexEditor Win64 Development
```

若 UE Editor 或 Live Coding 导致外部构建无法执行，记录具体原因，不要强行终止用户进程。

### 7.3 用户后续 PIE 成功标准

报告中列明以下人工验证标准：

1. 按 Q 后 Montage 正常播放。
2. GameplayEvent 关键帧在手部 Socket 生成投射物。
3. 日志显示：

```text
[ApexProjectile] ... Active=true ... UpdatedComponent=CollisionSphere
```

4. 投射物核心特效与拖尾沿屏幕中心方向飞行，而非停在手边。
5. 投射物撞墙后销毁并触发 Impact GameplayCue。
6. 未命中时在 MaxLifetime 后销毁。
7. Mana 与 Cooldown 行为不发生回归。

## 八、完成报告

完成后新建中文报告：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileMovement_Runtime_Fix_Report.md
```

报告必须包含：

1. 根因说明。
2. 实际修改文件。
3. 修改前后的调用顺序。
4. 新增或调整的函数、成员变量、日志及其作用。
5. 编译命令和完整结果摘要。
6. 未完成事项或风险。
7. 用户在 UE 编辑器中的最小验证步骤与通过标准。

不要声称已经验证 PIE；PIE 由用户在 Codex 审查通过后执行。
