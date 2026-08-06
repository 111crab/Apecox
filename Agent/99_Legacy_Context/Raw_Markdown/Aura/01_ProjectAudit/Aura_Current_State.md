# Aura 当前状态

*最后更新：2026-06-26*

这是一份轻量级基线摘要，用于后续规划。它不是完整代码审查。

## 项目形态

- Unreal Engine 项目：`Aura`。
- 当前 EngineAssociation：`5.8`。
- 主模块：`Aura` Runtime 模块。
- 当前 Target：Game 和 Editor。
- 本地 Editor 插件：`McpAutomationBridge`。
- 已存在 GAS、Enhanced Input、Motion Warping、PCG、Niagara 相关依赖或插件配置。

## GAS 基础

项目已经有一套教程阶段形成的 GAS 基础。

关键类：
- `UAuraAbilitySystemComponent`
- `UAuraAbilitySystemGlobals`
- `UAuraAttributeSet`
- `UAuraGameplayAbility`
- `UAuraDamageGameplayAbility`
- `UAuraProjectileSpell`
- `UExecCalc_Damage`
- `UMMC_MaxHealth`
- `UMMC_MaxMana`
- `UTargetDataUnderMouse`

当前 ASC 归属：
- 玩家 ASC 在 `AAuraPlayerState` 上。
- 玩家 Avatar 是 `AAuraCharacter`。
- 玩家 ASC 复制模式是 `Mixed`。
- 敌人 ASC 在 `AAuraEnemy` 上。
- 敌人 ASC 复制模式是 `Minimal`。

当前初始化路径：
- 玩家服务端：`AAuraCharacter::PossessedBy`。
- 玩家客户端：`AAuraCharacter::OnRep_PlayerState`。
- 敌人：`AAuraEnemy::BeginPlay`。

当前技能模型：
- 角色初始技能通过 `TArray<TSubclassOf<UGameplayAbility>>` 配置。
- `UAuraGameplayAbility` 当前包含 `StartupInputTag`。
- `UAuraProjectileSpell` 能生成投射物，并把伤害 GE Spec 传给投射物。
- 伤害数值通过 GameplayTag 对应的 SetByCaller Magnitude 传递。

## 属性与标签

当前属性分组：
- Primary：Strength、Intelligence、Resilience、Vigor。
- Secondary：Armor、ArmorPenetration、BlockChance、CriticalHitChance、CriticalHitDamage、CriticalHitResistance、HealthRegeneration、ManaRegeneration、MaxHealth、MaxMana。
- Resistance：Fire、Lightning、Arcane、Physical。
- Vital：Health、Mana。
- Meta：IncomingDamage。

当前 GameplayTag 域：
- `Attributes.*`
- `InputTag.*`
- `Damage.*`
- `Effects.HitReact`
- `Event.Montage.FireBolt`
- `Message.*`

规划注意：
- 项目变大前最好统一标签命名。已观察到一个 native tag 是 `Attributes.Resistance.lightning`，其中 `lightning` 为小写。

## 数据资产与配置

已有数据驱动基础：
- `UCharacterClassInfo`：按角色职业配置 Primary Attributes，并保存通用 Secondary/Vital Attributes、通用技能、伤害系数表。
- `UAttributeInfo`：支持 UI 展示属性信息。
- `UAuraInputConfig`：把 InputAction 映射到 GameplayTag。
- `UAuraAssetManager`：初始化 Native GameplayTags。

这些基础对未来技能系统有价值，但还不是通用技能编辑器所需的技能配置模型。

## 战斗与表现层

已有战斗/表现相关内容：
- 投射物 Actor：`AAuraProjectile`。
- 效果 Actor：`AAuraEffectActor`。
- 战斗接口：`ICombatInterface`。
- 敌人高亮接口：`IEnemyInterface`。
- 敌人受击 HitReact Tag 流程。
- 浮动伤害文字 UI。
- 角色死亡包含 ragdoll 和 dissolve 表现。

规划注意：
- 项目应该已有教程资产，如动画、特效、UI、材质等。后续需要单独盘点 `Content/`。

## UI 基础

已有 UI 类：
- `AAuraHUD`
- `UAuraWidgetController`
- `UOverlayWidgetController`
- `UAttributeMenuWidgetController`
- `UAuraUserWidget`
- `UDamageTextComponent`

规划注意：
- 运行时技能 UI 和未来编辑器 UI 要分开设计。编辑器 UI 应进入 Editor 模块或编辑器插件。

## 近期风险 / 注意事项

- 当前工作区已有多类未提交改动，已经做过分类报告。
- 部分中文注释在 shell 输出里显示乱码，后续大规模编辑前要注意文件编码。
- `Aura.Build.cs` 中 `EnhancedInput` 出现两次，后续代码卫生整理时可以清理。
- 当前还没有 Aura 自己的 Editor 模块；未来技能编辑器需要新增 Editor 模块或插件。
- 当前技能结构偏“每个技能一个类”；通用技能编辑器大概率需要在其上增加数据驱动层。
- 多人、预测、客户端体验要求还没有最终定案，架构设计不能忽略这些问题。

## 建议后续审计文档

- `Code_Architecture_Audit.md`：更深入的类级结构审查。
- `Asset_Inventory.md`：盘点动画、Niagara、音效、材质、地图、蓝图。
- `External_SkillGraph_Review.md`：评审现有 `UE_GAS_SkillGraph_Design_Document.md`。
