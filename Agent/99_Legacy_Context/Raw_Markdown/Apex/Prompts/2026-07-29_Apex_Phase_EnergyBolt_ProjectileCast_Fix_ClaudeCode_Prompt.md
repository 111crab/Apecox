# ClaudeCode Fix Prompt - Apex ProjectileCast 第一轮审查修复

项目：`D:\UnrealProject\Apex`

## 目标

修复第一轮实施中已经确认的 P1/P2 问题，使代码达到“可进入用户命名审阅”的状态。

本批仍然只修改 C++、依赖声明和中文实施报告。禁止使用 MCP，禁止创建或修改 UE 资产。

## 开始前必须读取

```text
Agent/00_Coordination/Working_Agreement.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Agent/00_Coordination/Current_Code_Design.md
Agent/Reviews/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Review.md
Agent/Prompts/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_ClaudeCode_Prompt.md

当前所有本批新增/修改的 Source/Apex 文件
```

先运行 `git status --short`。保留用户和 Codex 已有文档改动，不得 reset、restore、checkout 或清理工作树。

需要核对 UE 5.8 / Lyra 源码的实际 API：

```text
E:\UE_5.8\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\
D:\UnrealProject\LyraStarterGame\Source\LyraGame\AbilitySystem\
```

## 允许修改

第一轮 Prompt 允许的全部 C++ 文件，以及：

```text
Apex.uproject
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Public/AbilitySystem/Shared/ApexSkillExecutionConfig.h
Source/Apex/Public/AbilitySystem/Data/ApexSkillExecutionConfig.h
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

说明：

- `Apex.uproject` 中启用 StructUtils 是 UE 5.8 使用该插件模块的必要依赖，本轮允许保留。
- 将 `ApexSkillExecutionConfig.h` 从 `AbilitySystem/Shared` 移到 `AbilitySystem/Data`；删除旧文件并更新 Include。
- `ApexAbilitySet.cpp` 只恢复第一轮前的原写法，不做其他重构。

需要范围外修改时停止并报告。

## 一、恢复无关 AbilitySet 改动

将：

```cpp
UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>(); FGameplayAbilitySpec Spec(AbilityCDO, Grant.AbilityLevel);
```

恢复为原有清晰写法：

```cpp
FGameplayAbilitySpec Spec(AbilityClass, Grant.AbilityLevel);
```

UE 5.8 明确提供 `TSubclassOf<UGameplayAbility>` 构造函数。不得再修改 AbilitySet 其他逻辑。

## 二、ExecutionConfig 回归纯数据

`FApexSkillExecutionConfig`：

- 移到 `Source/Apex/Public/AbilitySystem/Data/ApexSkillExecutionConfig.h`。
- 保持空的 `USTRUCT(BlueprintType)` 数据基结构。
- 删除虚函数 `IsValidConfig()`。
- 删除为该虚函数/复制行为添加的不必要 StructOps 特化。

`FApexProjectileCastConfig`：

- 保留四个已批准字段。
- 删除 `IsValidConfig()` override。
- 运行时和编辑器校验都由 GA 模板负责。

## 三、修复 Data Validation

`UApexGameplayAbility` 增加普通 C++ virtual：

```cpp
virtual const UScriptStruct* GetRequiredExecutionConfigStruct() const;

#if WITH_EDITOR
virtual EDataValidationResult ValidateSkillDefinition(
    const UApexSkillDefinition& SkillDefinition,
    FDataValidationContext& Context) const;
#endif
```

要求：

- `GetRequiredExecutionConfigStruct()` 不使用 `UFUNCTION` 或 `BlueprintNativeEvent`。
- 根类默认返回 `nullptr`。
- 根类校验公共配置；Cost/Cooldown 允许为空。
- `UApexProjectileCastAbility` override 返回 `FApexProjectileCastConfig::StaticStruct()`。
- ProjectileCast 校验：
  - ExecutionConfig 类型正确。
  - ExecutionEventTag 有效。
  - SpawnSocketName 非 None。
  - MaxAimDistance 有限且 > 0。
  - ProjectileDefinition 非空。
  - ActivationMontage Soft Pointer 非空。
- `UApexSkillDefinition::IsDataValid()`：
  - 检查 AbilityTemplateClass。
  - 检查 `ActivationGroup != MAX`。
  - 按 Required Struct 检查 ExecutionConfig 相同或派生。
  - 调用 GA 模板 CDO 的 `ValidateSkillDefinition()`。
  - 给每个错误输出具体中文原因。

`EApexAbilityActivationGroup` 增加：

```cpp
MAX UMETA(Hidden)
```

## 四、修复 ActivationGroup

删除无调用者的：

```text
NotifyAbilityActivated_Group
NotifyAbilityEnded_Group
```

在 `UApexAbilitySystemComponent` 正确覆写：

```cpp
virtual void NotifyAbilityActivated(
    FGameplayAbilitySpecHandle Handle,
    UGameplayAbility* Ability) override;

virtual void NotifyAbilityEnded(
    FGameplayAbilitySpecHandle Handle,
    UGameplayAbility* Ability,
    bool bWasCancelled) override;
```

实现要求：

1. 从传入 Handle 对应 Spec.SourceObject 读取 SkillDefinition.PolicyConfig.ActivationGroup。
2. `IsActivationGroupBlocked()`：
   - Independent 永不阻塞。
   - 两种 Exclusive 只在已有 ExclusiveBlocking 时被阻塞。
   - Replaceable 活跃不能阻止 Blocking；Blocking 激活后应取消 Replaceable。
3. 新 Exclusive 激活时取消现有 ExclusiveReplaceable，但必须忽略新激活的 Handle/Ability。
4. 正确成对增加/减少计数。
5. 非法枚举、重复增加、无计数移除必须有 `ensureMsgf` 和安全返回。
6. 不使用 Loose GameplayTag 计数。
7. 不修改现有 Pressed/Held/Released 输入路由。

`UApexGameplayAbility` 覆写：

```cpp
virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
```

当当前组为 `ExclusiveReplaceable` 时，拒绝 `SetCanBeCanceled(false)`，保持它可以被替换。

`CanActivateAbility()`：

- 继续使用传入 Handle 查 Spec，禁止依赖 `GetCurrentAbilitySpec()`。
- SkillDefinition 缺失或 ActivationGroup 非法时拒绝激活并写明确日志，不能返回 true。

## 五、修复 ProjectileCast GA 状态机

`UApexProjectileCastAbility`：

- 声明为 `UCLASS(NotBlueprintable)`，防止 Blueprint K2 Activate 绕开固定生命周期。
- `ActivateAbility()` 保持 `final`。
- Native Activate 不调用会进入 K2 Activate 的 `Super::ActivateAbility()`。
- 每次激活最开始重置：
  - `bExecutionEventReceived`
  - `bAimDataReceived`
  - `bProjectileSpawned`
  - Montage 完成状态
  - 缓存 TargetData
- 在 Commit 前完成 Runtime 配置、ProjectileDefinition 和 Montage 加载检查。
- Montage 缺失/加载失败时拒绝激活，不扣成本。

新增一个统一推进函数，建议命名：

```text
TryExecuteProjectileSpawn
```

要求：

- `OnExecutionEventReceived()` 和 `OnAimTargetDataReceived()` 都调用它。
- 只在 Authority、Event/Data 均已就绪且尚未生成时执行。
- Data 先到或 Event 先到都能正确工作。
- Local Client 只缓存 Data，不生成 Projectile。
- Montage 完成但从未收到执行 Event 时取消 Ability。
- Event 已到但等待可靠 TargetData 时可以继续等待。
- TargetData Cancelled/无效时客户端和服务器都正确取消 Ability。
- 投射物成功生成后服务端结束 Ability，已生成 Projectile 独立运行。
- 所有结束路径清理 Task 和委托。

不要加入 Tick 或任意 Step 解释器。

## 六、正确读取并验证 LocationInfo

不要再使用：

```text
HasHitResult
GetHitResult
```

使用 GAS 通用 TargetData 接口：

```text
HasEndPoint
GetEndPoint
```

增加一个清晰的提取/校验函数，职责：

- Data 至少包含一个有效条目。
- 条目 `HasEndPoint()`。
- AimPoint 的 X/Y/Z 都是有限值。
- Avatar 有效。
- Avatar 到 AimPoint 距离不超过 `MaxAimDistance` 加少量网络容差；超出时拒绝或钳制，但必须在服务器执行。
- 不能把无效 Data 静默转换成世界原点。

## 七、修复 Aim TargetData Task

本地数据：

- 同时填写 LocationInfo 的 SourceLocation 和 TargetLocation。
- SourceLocation 使用 Avatar 当前 Transform。
- TargetLocation 使用屏幕中心 Trace 的命中点或最大距离端点。
- Deproject/Controller/Avatar 失败时进入 TargetData Cancelled 路径。

网络：

- 只有 `LocallyControlled && !NetAuthority` 时调用服务器 TargetData RPC。
- Listen Server 本地玩家直接广播本地 Data，不向自己发送 RPC。
- 使用 UE 5.8 推荐的 `CallServerSetReplicatedTargetData()`/对应 Cancelled 入口。
- 远端服务器同时注册 DataSet 和 DataCancelled delegate。
- `OnDestroy()` 成对解除两个委托。

生命周期：

- 收到 Replicated Data 后，先取得安全的本地 Handle 所有权/副本。
- 不允许 Consume 后继续使用可能失效的传入引用。
- 完成服务器有限值和距离校验后再广播。
- 广播/处理完成后 Consume。
- Cancelled 时通知 Owning Ability 收束，不留下永久 Active GA。

可以新增一个 `AimDataCancelled` delegate，并由 ProjectileCast GA 绑定取消处理。

## 八、修复受控生成钩子

恢复已批准的职责：

```cpp
virtual bool BuildProjectileSpawnTransform(
    const FVector& AimPoint,
    FTransform& OutSpawnTransform) const;

virtual void ExecuteProjectileSpawnOnAuthority(
    const FVector& AimPoint,
    const FGameplayEffectSpecHandle& ImpactEffectSpec);

AApexProjectile* SpawnProjectileActorOnAuthority(
    const FTransform& SpawnTransform,
    const FGameplayEffectSpecHandle& ImpactEffectSpec);
```

要求：

- `BuildProjectileSpawnTransform()` 检查 Character、Mesh 和 Socket 是否存在。
- 在生成 Actor 前检查 World、SourceASC、ProjectileClass、ImpactEffectClass。
- 先创建并验证 Impact GE Spec，再调用可覆写生成钩子。
- `ExecuteProjectileSpawnOnAuthority()` 默认生成一枚。
- `SpawnProjectileActorOnAuthority()` 不重复构建 GE Spec。
- 任一步失败都取消 Ability 或销毁半成品，不得崩溃或留下无效 Projectile。

## 九、修复 AApexProjectile 权威碰撞

要求：

- `InitializeProjectile()` 能向调用者返回成功/失败。
- 初始化时验证 Definition 和 ImpactEffectSpec。
- 显式设置 ProjectileMovement 的 Velocity/方向和速度，不能只假设 InitialSpeed 会在错误时序自动生效。
- 对 Owner/Instigator 调用 `IgnoreActorWhenMoving()`。
- `HandleImpact()` 第一行检查 Authority；客户端不执行命中玩法状态。
- `HandleImpact()` 再次拒绝 Owner/Instigator，覆盖 Overlap 和 ProjectileStop 两条入口。
- 服务器第一处有效世界或玩家命中只处理一次。
- 无目标 ASC 的世界命中仍执行 Cue 并 Destroy。
- 有目标 ASC 时应用 GE Spec。
- 客户端只依赖 Replicated Movement 和服务端销毁；必要时禁用客户端碰撞。
- 初始化失败时生成者销毁 Deferred/已生成半成品。

可以继续使用 Sphere + ProjectileMovement，但碰撞响应必须保证：

- 不会出生时撞到释放者。
- 世界/Pawn 的第一处碰撞能稳定触发。
- 高速移动使用 ProjectileMovement 的 Sweep/Blocking 路径，不只依赖不稳定的客户端 Overlap。

## 十、保留与说明 StructUtils 依赖

保留：

```text
Source/Apex/Apex.Build.cs -> StructUtils
Apex.uproject -> StructUtils plugin enabled
```

修复报告必须明确：

- `.uproject` 修改超出了第一轮允许列表。
- 这是第一轮 Prompt 遗漏的 UE 5.8 必要插件依赖，经 Codex 审查后允许保留。
- 不得宣称“第一轮无范围外修改”。

## 十一、编译

执行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

必须 0 error。不要通过删除校验、网络处理或清理路径绕过错误。

## 十二、覆盖修复报告

覆盖：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

中文报告必须补全：

- 修复前后 `git status --short`。
- 实际文件范围和全部范围外改动。
- 所有类、结构体、枚举、父类和职责。
- 所有成员变量：名称、类型、UPROPERTY 和作用。
- 所有函数：名称、签名、virtual/final/UFUNCTION 和作用。
- ActivationGroup 的真实 GAS 激活/结束链路。
- Event/Data 任意先后顺序的统一推进链路。
- LocationInfo 的正确读取和服务器校验。
- TargetData Data/Cancelled 的客户端、Listen Server、远端服务器路径。
- Projectile 初始化、移动、忽略释放者、权威命中、GE、Cue 和销毁链路。
- Data Validation 规则。
- StructUtils 的 Build.cs 与 `.uproject` 原因。
- 编译命令和结果。
- 明确未创建 UE 资产、未 PIE 验证。

完成后停止，等待 Codex 第二轮审查。
