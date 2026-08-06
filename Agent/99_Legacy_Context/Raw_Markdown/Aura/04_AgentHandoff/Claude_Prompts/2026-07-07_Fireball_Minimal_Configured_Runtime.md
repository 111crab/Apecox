# Claude 实施 Prompt：火球术最小配置化运行时

*创建日期：2026-07-07*
*状态：已获用户批准，可交给 Claude 执行*
*负责人：Claude 执行，Codex 审查，用户最终确认*

## 任务目标

在 Aura 项目中实现 V1-A 第一个配置化技能闭环：火球术。

本任务不是重写整个技能系统，也不是实现完整 SkillGraph。目标是用最小但正确的代码证明这条路径可行：

```text
UAuraSkillDefinition
  -> FGameplayAbilitySpec.SourceObject
  -> UAuraConfiguredActiveAbility
  -> 鼠标目标 TargetData
  -> 服务器生成配置化投射物
  -> GE_Damage + Damage.Fire SetByCaller
  -> 现有火球表现资产
```

## 必读文件

实施前请阅读：

- `.agents/ue-project-context.md`
- `Agent/00_Coordination/WorkingAgreement.md`
- `Agent/00_Coordination/Decision_Log.md`
- `Agent/02_SkillSystem/Skill_Runtime_Model.md`
- `Agent/02_SkillSystem/Fireball_Minimal_Asset_Inventory.md`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraGameplayAbility.h`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraDamageGameplayAbility.h`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraProjectileSpell.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraProjectileSpell.cpp`
- `Source/Aura/Public/Actor/AuraProjectile.h`
- `Source/Aura/Private/Actor/AuraProjectile.cpp`
- `Source/Aura/Public/AbilitySystem/AbilityTasks/TargetDataUnderMouse.h`
- `Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/Character/AuraCharacterBase.h`
- `Source/Aura/Private/Character/AuraCharacterBase.cpp`
- `Source/Aura/Private/Player/AuraPlayerController.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`

建议使用的技能/知识：
- `ue-gameplay-abilities`
- `ue-data-assets-tables`
- `ue-animation-system`
- `ue-niagara-effects`
- `ue-networking-replication`
- `ue-cpp-foundations`
- `ue-module-build-system`

## 允许修改的范围

允许新增或修改：

- `Source/Aura/Public/AbilitySystem/Data/`
- `Source/Aura/Private/AbilitySystem/Data/`
- `Source/Aura/Public/AbilitySystem/Abilities/`
- `Source/Aura/Private/AbilitySystem/Abilities/`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/Character/AuraCharacterBase.h`
- `Source/Aura/Private/Character/AuraCharacterBase.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`
- 必要时 `Source/Aura/Aura.Build.cs`
- `Agent/02_SkillSystem/` 下的中文实现说明文档
- `Agent/04_AgentHandoff/Claude_Reports/` 下的实施报告

如 MCP/编辑器可用，允许创建：

- `Content/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball.uasset`

如 MCP/编辑器不可用，不要用脚本硬写 `.uasset`，改为在报告里写清楚手动创建步骤。

## 禁止事项

- 不删除旧 `GA_FireBolt`。
- 不删除或重写 `UAuraProjectileSpell`。
- 不整体重构输入系统。
- 不把 Editor-only 依赖加入 `Aura` Runtime 模块。
- 不实现完整 SkillGraph / IR / VM。
- 不一次性实现复苏脉冲、烈焰领域、蓄力贯穿箭、三段裂刃、荆棘护盾。
- 不为火球术新建一套无法复用的专用 C++ 技能类。
- 不在没有说明的情况下把大量软引用改成硬引用。

## C++ 实施要求

### 1. 新增 `UAuraSkillDefinition`

建议路径：
- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp`

类型：
- 继承 `UPrimaryDataAsset`
- `BlueprintType`

第一版最小字段建议：

- Identity
  - `FGameplayTag SkillTag`
  - `FText DisplayName`
  - `FText Description`
  - `TSoftObjectPtr<UTexture2D> Icon`

- Grant / Input
  - `TSubclassOf<UGameplayAbility> AbilityClass`
  - `FGameplayTag InputTag`
  - `int32 DefaultLevel = 1`

- Activation / Timing
  - `TSoftObjectPtr<UAnimMontage> CastMontage`
  - `FGameplayTag MontageSpawnEventTag`

- Targeting
  - `EAuraSkillTargetingMode TargetingMode`
  - 第一版至少支持 `CursorHit`

- Delivery
  - `EAuraSkillDeliveryMode DeliveryMode`
  - 第一版至少支持 `Projectile`
  - `TSubclassOf<AAuraProjectile> ProjectileClass`

- Effect
  - `TSubclassOf<UGameplayEffect> DamageEffectClass`
  - `TMap<FGameplayTag, FScalableFloat> DamageTypes`

字段命名可以按项目习惯微调，但必须保持含义清晰。

注释要求：
- 解释为什么 `UAuraSkillDefinition` 是数据资产而不是每个技能一个 GA 类。
- 解释哪些字段属于 V1-A 临时最小字段，哪些是后续技能编辑器会继续扩展的方向。
- 如果 `CastMontage`/`Icon` 第一版使用同步加载，注释说明这是小资产、原型期折中，未来可改为 AssetManager bundle/异步加载。

### 2. 新增 `UAuraConfiguredActiveAbility`

建议路径：
- `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp`

职责：
- 从当前 `FGameplayAbilitySpec.SourceObject` 读取 `UAuraSkillDefinition`。
- `CanActivateAbility` 或 `ActivateAbility` 中校验配置是否存在。
- 调用 `CommitAbility`。
- 使用 `UTargetDataUnderMouse` 获取鼠标命中点。
- 播放 `CastMontage`，并等待 `MontageSpawnEventTag` 后生成投射物。
- 如果没有 Montage 或事件配置，允许 fallback 为拿到 TargetData 后直接生成投射物，并写日志。
- 只在服务器生成投射物。
- 通过 `DamageEffectClass` 和 `DamageTypes` 创建 `GameplayEffectSpec`，写入 `Damage.Fire` 等 SetByCaller。
- 把 `DamageEffectSpecHandle` 传给投射物。
- 正确结束 Ability；取消或目标数据失败时也要结束。

可以借鉴但不要直接依赖旧 `UAuraProjectileSpell::SpawnProjectile`。

注释要求：
- 解释为什么玩法结果必须在服务器产生。
- 解释 `SourceObject` 如何把“同一个通用 GA”绑定到“不同技能配置资产”。
- 解释 TargetData 中客户端意图与服务端权威的关系。
- 解释 GameplayCue/音效/Niagara 是表现，不决定伤害。

### 3. 新增配置技能授予路径

在 `UAuraAbilitySystemComponent` 中新增类似：

```cpp
void AddCharacterSkillDefinitions(const TArray<UAuraSkillDefinition*>& StartupSkillDefinitions);
```

行为：
- 仅服务器授予。
- 对每个有效 `UAuraSkillDefinition` 创建 `FGameplayAbilitySpec`。
- `AbilityClass` 优先来自 SkillDefinition；为空时可 fallback 到 `UAuraConfiguredActiveAbility::StaticClass()`。
- `Level` 使用 `DefaultLevel`。
- `SourceObject` 设置为当前 SkillDefinition。
- `DynamicAbilityTags` 添加 `InputTag`。
- 调用 `GiveAbility`。

在 `AAuraCharacterBase` 中新增：

```cpp
TArray<TObjectPtr<UAuraSkillDefinition>> StartupSkillDefinitions;
```

并在 `AddCharacterAbilities()` 中保留旧 `StartupAbilities` 授予，同时调用新配置技能授予路径。

注释要求：
- 解释为什么旧 GA 路径暂时保留。
- 解释为什么输入 Tag 放在 `DynamicAbilityTags`，而不是写死在通用 GA 类中。

### 4. GameplayTag

请新增或确认：

- `Ability.Fire.Fireball`

已有可复用：
- `InputTag.LMB`
- `InputTag.RMB`
- `InputTag.1` 到 `InputTag.4`
- `Damage.Fire`
- `Event.Montage.FireBolt`

如果新增 native tag，请同步更新 `.h/.cpp`。如果只作为配置资产使用，也可以先写入 `DefaultGameplayTags.ini`。请在报告中说明选择。

## 推荐火球数据资产配置

如果 MCP/编辑器可用，请创建：

`/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball`

建议字段：

- `DisplayName = 火球术`
- `SkillTag = Ability.Fire.Fireball`
- `InputTag = InputTag.1`
- `AbilityClass = UAuraConfiguredActiveAbility`
- `DefaultLevel = 1`
- `TargetingMode = CursorHit`
- `DeliveryMode = Projectile`
- `ProjectileClass = /Game/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/BP_FireBolt.BP_FireBolt_C`
- `DamageEffectClass = /Game/Blueprints/AbilitySystem/GameplayEffects/GE_Damage.GE_Damage_C`
- `DamageTypes = { Damage.Fire: 50 }`
- `CastMontage = /Game/Assets/Characters/Aura/Animations/Abilities/AM_Cast_FireBolt.AM_Cast_FireBolt`
- `MontageSpawnEventTag = Event.Montage.FireBolt`
- `Icon = /Game/Assets/UI/Spells/FireBolt.FireBolt`

然后把 `DA_Fireball` 加到 `BP_AuraCharacter` 的 `StartupSkillDefinitions`。

注意：第一版推荐用 `InputTag.1`，避免和旧左键火球/移动逻辑冲突。如果你发现旧 `GA_FireBolt` 已经绑定 `InputTag.1`，请不要制造双触发，改用一个未占用输入或在报告中提示需要用户确认。

## 验证要求

### 必做验证

1. C++ 编译通过。
2. UE 编辑器能打开项目。
3. 新 C++ 类能在编辑器中被识别。
4. `DA_Fireball` 能创建并填写字段，或报告中给出手动创建步骤。
5. 单人 PIE：
   - 玩家按推荐输入键。
   - 播放 `AM_Cast_FireBolt` 或 fallback 生成投射物。
   - 投射物从武器/战斗 Socket 生成。
   - 投射物朝鼠标命中点移动。
   - 命中敌人后应用 `GE_Damage`。
   - 目标生命变化或出现伤害数字。
   - 命中特效/音效正常。

### 多人验证

如本轮环境能运行 Listen Server / 两客户端，请验证：

1. 客户端释放火球，服务端生成投射物。
2. 另一个客户端能看到投射物和命中表现。
3. 伤害只由服务器权威应用。
4. Host 不出现明显重复命中特效/音效。
5. 投射物生命周期正常清理。

如果环境无法验证多人，请不要假装通过；在报告中写明未验证原因和建议手动验证步骤。

## 必须输出的报告

请写入：

`Agent/04_AgentHandoff/Claude_Reports/2026-07-07_Fireball_Minimal_Configured_Runtime_Report.md`

报告必须包含：

- 简要总结。
- 修改/新增文件列表。
- 新增类的职责解释。
- 为什么这样设计，而不是每个技能一个 GA。
- 是否创建了 `DA_Fireball`；如果没有，写出手动创建步骤。
- 运行了哪些编译/测试/编辑器/MCP 操作。
- 单人验证结果。
- 多人验证结果，或未验证原因。
- 已知问题。
- 下一步建议。

## 验收标准

本任务完成时，至少应满足：

- 旧教程火球代码仍保留。
- 新配置技能路径存在并可授予。
- 火球术至少能通过 `UAuraSkillDefinition` 驱动一次完整施放。
- 伤害通过 `GE_Damage` + `Damage.Fire` SetByCaller 应用。
- 代码有学习型注释，能帮助用户理解 GAS、SourceObject、服务器权威和配置化设计。
- 报告清楚说明哪些内容已验证，哪些还需要用户在 UE 编辑器中确认。
