# 技能运行时模型

*创建日期：2026-07-07*  
*状态：Phase 2 运行时模型已确认，可进入轻量资产盘点与第一份 Claude 实施 Prompt。*

## 目的

本文件把已经确认的“克制版方案 C”落成具体运行时模型：

- 少量通用 GA。
- `DataAsset` 技能配置资产。
- 少量原生机制片段。
- V1-A 6 个技能逐个实现和验证。

本文件仍然不是 C++ 实施 Prompt。它用于确认类、数据结构、运行流程和边界。用户已确认本模型后，下一步做轻量资产盘点，再写第一份 Claude 实施 Prompt。

## 当前 Aura 起点

现有 Aura 已经具备：

- `UAuraGameplayAbility`：当前有 `StartupInputTag`。
- `UAuraDamageGameplayAbility`：当前有 `DamageEffectClass` 和 `DamageTypes`。
- `UAuraProjectileSpell`：当前的教程式火球/投射物技能样本。
- `AAuraProjectile`：复制投射物，命中后在服务器应用 `DamageEffectSpecHandle`。
- `UAuraAbilitySystemComponent`：授予初始 GA，并通过输入 Tag 激活 AbilitySpec。
- `UAuraAttributeSet`：生命、法力、抗性、伤害 Meta Attribute 等基础属性。
- 玩家 ASC 在 `AAuraPlayerState`，复制模式 `Mixed`。
- 敌人 ASC 在 `AAuraEnemy`，复制模式 `Minimal`。

因此 V1 不从零重写，而是沿着现有 GAS 教程结构演进。

## 总体结构

V1 运行时由 5 类对象组成：

| 类型 | 建议名称 | 职责 |
| --- | --- | --- |
| 技能配置资产 | `UAuraSkillDefinition` | 保存一个技能的可编辑配置，例如火球术的数据 |
| 通用主动 GA | `UAuraConfiguredActiveAbility` | 读取 SkillDefinition，统一执行主动技能生命周期 |
| 通用被动 GA | `UAuraConfiguredPassiveAbility`，V1-A 可后置 | 读取 SkillDefinition，注册/注销被动监听 |
| 原生机制片段 | Targeting / Delivery / Effect / Presentation / Cleanup | 处理投射物、范围、蓄力、连招、被动等 C++ 机制 |
| 运行时上下文 | `FAuraSkillRuntimeContext` | 在一次技能激活期间传递 ActorInfo、ASC、目标、Spec、配置等数据 |

简化流程：

```text
InputTag
  -> UAuraAbilitySystemComponent 找到 AbilitySpec
  -> UAuraConfiguredActiveAbility 激活
  -> 从 AbilitySpec.SourceObject 读取 UAuraSkillDefinition
  -> Commit 成本/冷却
  -> Targeting 产生目标数据
  -> Delivery 把效果送到目标
  -> Effect 创建并应用 GameplayEffectSpec
  -> Presentation 触发 Montage / GameplayCue / SFX / UI
  -> Cleanup 清理持续对象和监听
```

## 为什么需要 AbilitySpec.SourceObject

如果每个技能一个 GA 类，`UGA_Fireball` 可以直接在类默认值里配置火球数据。但我们选择少量通用 GA 后，一个 `UAuraConfiguredActiveAbility` 要代表很多技能。

所以技能数据不能只写在 GA 类默认值里。更合理的方式是：

- 所有主动配置技能共享 `UAuraConfiguredActiveAbility` 类。
- 授予技能时创建 `FGameplayAbilitySpec`。
- 把具体 `UAuraSkillDefinition` 放到 `AbilitySpec.SourceObject` 或等价字段中。
- 激活时通用 GA 从当前 Spec 取回配置资产。

这样才能做到：

- 火球术、复苏脉冲、烈焰领域都可以用同一个通用 GA。
- 每个技能仍然有自己的输入 Tag、技能 Tag、成本、冷却、表现和效果。
- 后续技能编辑器只需要保存/修改 `UAuraSkillDefinition` 配置资产。

这也是方案 C 成立的关键。

## `UAuraSkillDefinition`

建议使用 `UPrimaryDataAsset`，方便未来 Asset Manager 扫描、编辑器列举、按技能 ID 加载。

核心字段建议：

| 字段组 | 字段例子 | 说明 |
| --- | --- | --- |
| Identity | `SkillTag`、`DisplayName`、`Description`、`Icon` | 技能身份、UI、编辑器显示 |
| Grant/Input | `AbilityClass`、`InputTag`、`DefaultLevel` | 授予哪个通用 GA，用哪个输入触发 |
| Activation | `ActivationPolicy`、`bCanBeCanceled`、`BlockedTags` | 主动、按住、松开、被动、开关等 |
| Cost/Cooldown | `CostEffect`、`CooldownEffect`、`ManaCost`、`CooldownDuration` | 优先复用 GAS 成本/冷却 |
| Targeting | `TargetingMode`、`Range`、`Radius`、`TraceChannel` | 自身、单体、地面点、范围、方向等 |
| Delivery | `DeliveryMode`、`ProjectileClass`、`AreaClass`、`Speed`、`LifeTime` | 直接、投射物、区域、Trace 等 |
| Effect | `EffectEntries`、`DamageTypes`、`SetByCallerValues` | 应用哪些 GE，运行时注入哪些数值 |
| Presentation | `CastMontage`、`GameplayCues`、`Sound`、`PreviewAssets` | 表现配置，不决定权威玩法 |
| Validation | `bAllowFriendlyTargets`、`bAllowEnemyTargets`、`RequiredTags` | 阵营、Tag、目标合法性 |
| Editor Preview | `PreviewPawn`、`PreviewTarget`、`PreviewNotes` | 未来技能编辑器预览使用 |

V1-A 不需要一次实现所有字段。建议第一版只实现能支撑火球术和后续 V1-A 的最小字段，但结构上保留分组。

## 通用 GA

### `UAuraConfiguredActiveAbility`

职责：

- 从当前 `FGameplayAbilitySpec` 读取 `UAuraSkillDefinition`。
- 统一处理 `CanActivateAbility`。
- 统一处理 `CommitAbility`。
- 建立 `FAuraSkillRuntimeContext`。
- 根据配置调用 Targeting / Delivery / Effect / Presentation。
- 正确 `EndAbility` 或等待异步任务完成。
- 处理取消、目标失效、Actor 清理。

它不应该做的事：

- 不直接写死“火球术逻辑”。
- 不直接决定某个技能的伤害数值。
- 不把所有机制塞进一个巨大 `switch` 后长期不拆。
- 不在客户端决定权威伤害、治疗、Buff、Debuff。

### `UAuraConfiguredPassiveAbility`

V1-A 的荆棘护盾可能需要“主动 Buff + 受击触发”，不一定第一版就单独做被动 GA。

建议：

- V1-A 前 5 个技能先用 `UAuraConfiguredActiveAbility`。
- 荆棘护盾实施前再判断：是主动技能授予一个监听型 GE/组件，还是引入 `UAuraConfiguredPassiveAbility`。
- 不为了尚未实现的被动体系提前做复杂框架。

## 运行时上下文

建议设计一个轻量结构：

```cpp
FAuraSkillRuntimeContext
```

它在一次技能执行期间保存：

- `SkillDefinition`
- `Ability`
- `ActorInfo`
- `ActivationInfo`
- `SourceASC`
- `SourceAvatar`
- `SourceController`
- `AbilityLevel`
- `TargetData`
- `EffectContext`
- `OutgoingSpecs`
- `SpawnedActors`
- `Prediction / Authority` 相关信息

这个上下文不需要复制。它是执行期间的临时数据。真正需要同步的状态应走 GAS 的 GE、Attribute、GameplayTag，或复制 Actor。

## 原生机制片段

“原生机制片段”不是随便拼节点，也不是每个技能一套 C++。

它指一类机制有一份稳定 C++ 实现，并由 `UAuraSkillDefinition` 配置调用。

V1-A 建议先定义这些片段边界：

| 片段 | 职责 | V1-A 使用者 |
| --- | --- | --- |
| Targeting | 产生目标数据：自身、目标点、方向、范围、近战窗口 | 全部主动技能 |
| Delivery | 把效果送到目标：直接、投射物、区域、Trace | 火球术、烈焰领域、三段裂刃 |
| Effect | 创建和应用 GE Spec，设置 SetByCaller | 全部有玩法结果的技能 |
| Presentation | 播放 Montage、GameplayCue、音效、UI 反馈 | 全部技能 |
| Lifetime/Cleanup | 管理投射物、区域 Actor、监听句柄、持续效果 | 火球术、烈焰领域、荆棘护盾 |

V1 实现时可以先用 `USTRUCT` 配置 + 原生执行函数，不急着把每个片段做成独立 UObject 类。等机制变多，再把片段升级为策略类或可扩展对象。

这能避免第一版出现几十个过度抽象类。

## 分层边界

### Targeting

Targeting 只负责“找目标或位置”，不负责应用效果。

V1-A 需要：

- Self：复苏脉冲、荆棘护盾。
- Direction / Mouse Point：火球术、蓄力贯穿箭。
- Ground Point + Radius：烈焰领域。
- Melee Trace Window：三段裂刃。

注意：

- 客户端可以提供意图，例如鼠标点、方向。
- 服务器必须验证目标合法性。
- TargetData 可以借助 GAS TargetData 或自定义轻量结构，V1 不必先做完整 TargetActor 体系。

### Delivery

Delivery 负责“效果怎么抵达目标”。

V1-A 需要：

- Direct：复苏脉冲、荆棘护盾。
- Projectile：火球术，蓄力贯穿箭可选。
- Area：烈焰领域。
- Trace：三段裂刃。

关键规则：

- 投射物和区域 Actor 由服务器生成。
- 投射物/区域表现可以复制或由 Cue 表现。
- 命中、治疗、伤害、Buff、Debuff 由服务器权威应用。

### Effect

Effect 负责创建并应用 `GameplayEffectSpec`。

V1-A 建议：

- 继续复用现有 `DamageEffectClass` / `ExecCalc_Damage` 思路。
- 伤害数值通过 SetByCaller 注入。
- 治疗可以先使用 Instant GE 或 SetByCaller 治疗值。
- 持续伤害/治疗使用 Duration + Periodic GE。
- 冷却和成本尽量使用 GAS Cost / Cooldown GE。

不建议：

- 每个技能都创建一堆专用 GE C++ 类。
- 在 GameplayCue 或 Niagara Notify 中修改属性。
- 在 `PreAttributeChange` 里触发复杂玩法事件。

### Presentation

Presentation 只负责表现：

- Montage。
- GameplayCue。
- Niagara。
- 音效。
- UI 消息。
- 范围预览。

表现层不决定：

- 是否命中。
- 伤害多少。
- 是否施加 Buff。
- 是否触发反弹。

这些必须由服务器/GAS 路径决定。

### Cleanup

Cleanup 负责：

- 投射物命中或超时销毁。
- 区域 Actor 到期清理。
- Ability 被取消时停止 Montage / Cue / 监听。
- 目标死亡或失效时安全结束。
- Listen Server 下避免 Host 重复表现。

这部分必须显式设计，因为技能系统最容易在这里产生隐藏 bug。

## 授予技能流程

当前 `UAuraAbilitySystemComponent::AddCharacterAbilities` 接收 `TArray<TSubclassOf<UGameplayAbility>>`。

为了支持配置技能，建议新增一条授予路径：

```text
StartupSkillDefinitions: TArray<UAuraSkillDefinition*>
  -> 对每个 SkillDefinition 创建 FGameplayAbilitySpec
  -> AbilityClass = SkillDefinition.AbilityClass 或默认 UAuraConfiguredActiveAbility
  -> Level = SkillDefinition.DefaultLevel
  -> SourceObject = SkillDefinition
  -> DynamicAbilityTags 添加 SkillDefinition.InputTag
  -> GiveAbility
```

这样保留旧教程 Ability 授予路径，同时新增配置技能路径。

V1 不建议直接删除旧路径。先并存，等配置技能稳定后再讨论迁移。

## V1-A 技能映射

| 技能 | 通用 GA | Targeting | Delivery | Effect | Presentation | 特殊机制片段 |
| --- | --- | --- | --- | --- | --- | --- |
| 火球术 | `UAuraConfiguredActiveAbility` | 鼠标点/方向 | Projectile | Fire Damage GE | Cast Montage + Impact Cue | 服务器生成投射物 |
| 复苏脉冲 | `UAuraConfiguredActiveAbility` | Self / Friendly | Direct | Instant Heal + HoT | Heal Cue + UI | 友方过滤 |
| 烈焰领域 | `UAuraConfiguredActiveAbility` | Ground Point + Radius | Area Actor | Periodic Fire Damage | Area Loop Cue | 区域生命周期 |
| 蓄力贯穿箭 | `UAuraConfiguredActiveAbility` | Direction / Path | Projectile 或 Trace | Damage x Charge | Charge + Release Cue | 按住/松开、蓄力曲线 |
| 三段裂刃 | `UAuraConfiguredActiveAbility` 或后置 Combo 子类 | Melee Trace Window | Trace | Melee Damage GE | Montage Sections | 连招阶段、输入窗口 |
| 荆棘护盾 | `UAuraConfiguredActiveAbility`，被动部分可后置 | Self / Attacker | Direct | Shield + Reflect Damage | Shield Cue | 受击监听、防递归 |

## 火球术示例

火球术的 `UAuraSkillDefinition` 可能配置：

- `SkillTag = Ability.Fire.Fireball`
- `InputTag = InputTag.LMB`
- `AbilityClass = UAuraConfiguredActiveAbility`
- `ActivationPolicy = OnInputTriggered`
- `CostEffect = GE_Cost_Mana`
- `CooldownEffect = GE_Cooldown_Fireball`
- `TargetingMode = MousePoint`
- `DeliveryMode = Projectile`
- `ProjectileClass = BP_AuraConfiguredProjectile`
- `DamageEffect = GE_Damage`
- `SetByCaller Damage.Fire = 50`
- `CastMontage = AM_Cast_Fire`
- `GameplayCue.Cast = GameplayCue.Skill.Fireball.Cast`
- `GameplayCue.Impact = GameplayCue.Skill.Fireball.Impact`

运行时：

1. 玩家按输入键。
2. ASC 找到带 `InputTag.LMB` 的 AbilitySpec。
3. `UAuraConfiguredActiveAbility` 激活。
4. Ability 从 SourceObject 读取火球术配置。
5. Ability Commit 成本和冷却。
6. Targeting 取得鼠标点。
7. Delivery 在服务器生成投射物。
8. 投射物命中后，服务器使用配置创建 GE Spec，并写入 `Damage.Fire` SetByCaller。
9. 目标 ASC 应用伤害 GE。
10. GameplayCue 播放爆炸表现。
11. 投射物销毁，Ability 结束或等待投射物生命周期回调。

## 多人边界

V1-A 每个技能都必须遵守：

- 玩家客户端可以发起激活请求。
- 服务器是玩法结果权威。
- Attribute、GE、GameplayTag 走 GAS 同步。
- 投射物、区域 Actor 由服务器生成并复制，或通过服务器触发 Cue 表现。
- GameplayCue / 音效 / 镜头属于表现，不作为玩法来源。
- Listen Server 下 Host 同时是服务器和本地玩家，表现逻辑要避免重复。
- 延迟加入玩家至少能拿到仍然存在的关键状态，例如持续 GE、仍存在的区域 Actor。

V1-A 暂不承诺完整 Dedicated Server 压测，但模型不能阻断 Dedicated Server 演进。

## 资产引用策略

`UAuraSkillDefinition` 中的表现资产建议优先使用软引用：

- 图标。
- Niagara。
- 音效。
- Montage。
- 预览用 Mesh / Pawn。

理由：

- 技能配置资产会越来越多，硬引用所有表现资产会造成加载膨胀。
- 未来技能编辑器预览和游戏运行可以选择不同加载 bundle。
- Aura 已经有 `UAuraAssetManager`，后续可接入 Primary Asset 扫描。

V1-A 可以先同步加载少量资产以降低实现难度，但文档和代码注释要说明这是阶段性折中。

具体技能实施前，资产查找顺序为：

1. 优先在 `Content/Assets` 中寻找 Aura 教程已提供的动画、Niagara、材质、音效、Mesh 或图标。
2. 如果教程资产不够，再考虑 UE 官方内容、简单材质/占位表现，或用现有 Niagara/材质做最小可读的替代。
3. 如果仍然缺关键资产，Codex/Claude 应明确列出缺口，让用户决定是否从网上补充。

资产字段默认倾向软引用；只有在 V1-A 为了快速跑通、资产数量很少且加载路径明确时，才允许阶段性使用硬引用或同步加载，并在注释/文档中说明原因。

## 直接添加 UE C++ 文件的规则

本项目后续会直接创建 `.h` / `.cpp` 文件，而不一定通过 UE 编辑器的“添加 C++ 类”向导。这在 UE C++ 项目中是正常做法，只要遵守以下规则，编辑器和 IDE 都可以识别：

- Runtime 代码放在 `Source/Aura/Public` 和 `Source/Aura/Private`，不要把 Editor-only 依赖放进 `Aura` Runtime 模块。
- `UCLASS`、`USTRUCT`、`UENUM`、`UINTERFACE` 等反射类型必须正确包含对应的 `.generated.h`，并遵守 UE Header Tool 的包含顺序。
- 新增模块依赖时必须同步修改 `Aura.Build.cs`，例如 GameplayTasks、Niagara、Slate、UnrealEd 等依赖要区分 Runtime 与 Editor。
- 新文件通常需要重新生成项目文件、刷新 IDE、重新编译，或者重启 UE 编辑器后才能完全显示。
- 直接添加文件不生成静态技能类的问题：我们的长期方向是生成/编辑 `UAuraSkillDefinition` 资产，不为每个技能生成新的 GA/GE C++ 类。

因此，后续 Claude 直接新增 `.h` / `.cpp` 是允许的，但 Prompt 必须要求它说明文件路径、模块归属、Build.cs 依赖和是否需要重新生成项目文件。

## 旧教程代码处理原则

当前教程代码已经通过 Git tag `tutorial-baseline-before-v1-skill-runtime` 保存为可回退基线。后续可以重构，但仍然遵守渐进迁移：

- 先保留旧教程 GA 路径，同时新增配置技能路径。
- 每次改旧类前先判断它是“继续复用”“迁移改造”还是“被新体系替代”。
- 不为了架构洁癖提前删除能帮助验证的教程代码。
- 如果旧代码命名、Tag 或职责不符合新体系，先在文档/Prompt 中说明迁移理由，再实施。

## 命名与输入绑定

Codex 可以为内部类名、字段名、GameplayTag 命名提出默认方案，并在文档或注释中解释命名意图。

需要玩家实际按键触发的技能，如果会影响用户操作习惯，Codex 应显式提出建议绑定并让用户确认。若只是第一轮原型验证，可先沿用当前教程输入体系，例如 `InputTag.LMB`、`InputTag.RMB`、`InputTag.1` 到 `InputTag.4`。

## 与技能编辑器的关系

未来技能编辑器不是编辑 GA 类，而是编辑 `UAuraSkillDefinition`。

编辑器可以围绕这些区域组织 UI：

- 基础信息。
- 输入与激活。
- 成本与冷却。
- 目标选择。
- Delivery。
- Effect。
- 表现。
- 预览。
- 数据校验。

这对应用户提出的 Montage 类比：

- 细节面板改配置。
- 预览区域播放技能效果。
- 保存为配置资产。
- Play 时可被角色使用。

因此运行时模型必须保持“配置资产可解释、可验证、可预览”。

## V1-A 最小实现建议

第一批 Claude 实施不应该一次做全模型。

建议第一份实现只围绕火球术建立最小闭环：

1. `UAuraSkillDefinition` 最小字段。
2. `UAuraConfiguredActiveAbility` 最小执行流程。
3. 新增配置技能授予路径。
4. Projectile Delivery 最小片段。
5. Effect Spec + SetByCaller 伤害应用。
6. GameplayCue / 现有 Niagara/Sound 表现接入。
7. 单技能中文说明文档和学习型注释。

火球术跑通后，再按 V1-A 顺序扩展字段和片段。

用户已确认第一份 Claude 实施只围绕火球术最小闭环，不一次实现全部 V1-A 机制。

## 暂不实现

V1-A 第一阶段暂不实现：

- 完整 SkillGraph。
- IR / VM。
- 任意函数调用节点。
- 完整自定义资产编辑器。
- Dedicated Server 压测。
- 所有机制片段 UObject 化。
- 通用召唤/陷阱/位移系统。

这些不是放弃，而是等 V1-A 基础闭环稳定后再扩展。

## 已确认事项

1. 认可 `UAuraSkillDefinition + UAuraConfiguredActiveAbility + AbilitySpec.SourceObject` 作为 V1 配置技能主路径。
2. 认可 V1-A 先保留旧教程 GA 路径，并新增配置技能路径，而不是立刻迁移/删除旧类。
3. 认可第一份 Claude 实施只围绕火球术最小闭环，不一次实现全部 V1-A 机制。
4. 旧教程代码已通过 Git tag `tutorial-baseline-before-v1-skill-runtime` 保存为重构前基线。
5. 正式代码必须包含学习型注释；如果设计思想不适合塞进代码注释，则补充中文 MD 文档。
