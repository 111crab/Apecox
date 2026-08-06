# 生命涌动 V1-B 配置化技能实施报告

*创建日期：2026-07-08*
*实施者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-08_VitalSurge_V1B_Implementation.md*

## 1. 简要总结

在 Aura 项目中实现了第二个配置化技能：生命涌动 / Vital Surge。火球术 V1-A 路径完全保留，新路径并行接入。

新增机制：
- `Self` 目标 + `Direct` 送达 + `HoldRelease` 输入
- `FAuraSkillEffectEntry` 通用效果条目（替代临时字段）
- 引导状态 Tag + 移动输入取消
- 完整引导瞬间治疗 / 未完整引导持续治疗

```
按住 InputTag.2
→ 开始引导（State.Skill.Channeling）
→ 松开结算
→ 满 2 秒：FullCharge → 瞬间治疗 GE（SetByCaller.Healing = 100）
→ 不满 2 秒：PartialCharge → 持续治疗 GE（DA 填 SetByCaller.HealingPerTick = 25，运行时乘 PartialChargeScale 后实际每跳 12.5，4秒）
→ 引导期间尝试移动：取消技能，不治疗
```

## 2. 修改/新增文件列表

### 修改的文件

| 文件 | 变更范围 |
|------|----------|
| `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h` | 新增 4 个枚举、1 个结构体、6 个字段；显式赋值已有枚举值 |
| `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp` | 无变化（纯数据类） |
| `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h` | 新增 HoldRelease 路径方法（5个）+ 状态变量（3个）；重构为 Press/HoldRelease 分支 |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp` | 完整重写：ActivateAbility 分派、StartPressFlow、StartHoldReleaseFlow、ApplyEffectEntriesToSelf、ChannelingState |
| `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h` | 新增 `CancelActiveChannelingAbilities()` |
| `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp` | 实现移动取消逻辑 |
| `Source/Aura/Private/Player/AuraPlayerController.cpp` | `Move()` 中加入移动取消引导技能检测 |
| `Source/Aura/Public/AuraGameplayTags.h` | 新增 `SetByCaller_Healing`、`SetByCaller_HealingPerTick`、`State_Skill_Channeling` |
| `Source/Aura/Private/AuraGameplayTags.cpp` | 注册 3 个 native tags |
| `Config/DefaultGameplayTags.ini` | 新增 `Ability.Vital.VitalSurge` |

### 通过 MCP 创建/修改的 UE 资产

| 资产 | 操作 |
|------|------|
| `/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge` | 从 `DA_Fireball` 复制（需手动重配字段） |
| `/Game/GE_VitalSurge_InstantHeal` | 新建（需手动移至目标路径并配置） |
| `/Game/GE_VitalSurge_HealOverTime` | 新建（需手动移至目标路径并配置） |
| `BP_AuraCharacter.StartupSkillDefinitions` | 已添加 `DA_VitalSurge`（保留 `DA_Fireball`） |

**未修改**：`Aura.Build.cs`（所有新依赖已在模块依赖列表中）。

## 3. 新增字段、枚举、函数的职责说明

### 枚举

| 枚举 | 新增值 | 用途 |
|------|--------|------|
| `EAuraSkillTargetingMode` | `Self = 1` | 目标为施法者自身 |
| `EAuraSkillDeliveryMode` | `Direct = 1` | 直接应用 GE，不生成投射物 |
| `EAuraSkillInputMode` | `Press = 0`, `HoldRelease = 1` | 按键即执行 vs 按住引导松开结算 |
| `EAuraSkillEffectTarget` | `Self`, `Source`, `HitActor` | GE 应用目标 |
| `EAuraSkillEffectCondition` | `Always`, `FullCharge`, `PartialCharge` | GE 触发条件分档 |

**已有枚举值显式赋值**（`CursorHit = 0`, `Projectile = 0`），确保已有资产（`DA_Fireball`）的序列化整数值不被破坏。

### 结构体

`FAuraSkillEffectEntry`：通用效果条目。包含 `EffectClass`、`Target`、`Condition`、`SetByCallerMagnitudes`。一个技能可配置多个条目，按 `Condition` 分档触发。

### 新增字段

| 字段 | 类型 | 用途 |
|------|------|------|
| `InputMode` | `EAuraSkillInputMode` | 默认 `Press`；`HoldRelease` 启用引导释放路径 |
| `FullChargeDuration` | `float` | 完整引导需按住时长（秒） |
| `PartialChargeScale` | `float` | 未完整引导缩放系数（默认 0.5） |
| `bCancelOnMoveInputDuringChannel` | `bool` | 引导期间移动输入是否取消技能 |
| `EffectEntries` | `TArray<FAuraSkillEffectEntry>` | 通用效果条目列表 |

### 新增函数

| 函数 | 所属类 | 职责 |
|------|--------|------|
| `StartPressFlow` | `UAuraConfiguredActiveAbility` | Press 模式入口（原火球术逻辑） |
| `StartHoldReleaseFlow` | `UAuraConfiguredActiveAbility` | HoldRelease 模式入口：校验 Self+Direct、记录引导时间、添加引导状态、创建 WaitInputRelease |
| `OnHoldInputReleased` | `UAuraConfiguredActiveAbility` | 输入释放回调：判断 FullCharge/PartialCharge，服务端应用 EffectEntries |
| `ApplyEffectEntriesToSelf` | `UAuraConfiguredActiveAbility` | 遍历 EffectEntries，按 Condition 筛选，创建 GE Spec，注入 SetByCaller，应用到自身 |
| `AddChannelingState` | `UAuraConfiguredActiveAbility` | 添加 `State.Skill.Channeling` loose tag |
| `RemoveChannelingState` | `UAuraConfiguredActiveAbility` | 移除引导状态 tag（EndAbility 保证清理） |
| `CancelActiveChannelingAbilities` | `UAuraAbilitySystemComponent` | 遍历 Active Abilities，通过 SourceObject 读回 SkillDefinition，取消满足移动取消条件的 HoldRelease 技能 |

## 4. 为什么本轮采用 EffectEntry

火球术的 `DamageEffectClass` + `DamageTypes` 是为单一伤害技能设计的专用字段。生命涌动需要两个不同类型的 GE（瞬间治疗 vs 持续治疗），如果继续新增专用字段（如 `HealEffectClass`、`HoTEffectClass`），后续每个新技能都会导致字段膨胀。

`FAuraSkillEffectEntry` 数组天然支持：
- 多效果（治疗+护盾、伤害+减速）
- 条件分档（FullCharge / PartialCharge / Always）
- 目标选择（Self / HitActor / Source）
- 独立 SetByCaller 数值

这直接服务未来技能编辑器的"效果条目列表" UI。

## 5. 为什么移动取消放在输入层

速度检查只能知道角色有没有"动起来"，不能准确知道玩家有没有"尝试移动"。例如：
- 角色被定身 → 速度为 0 → 速度检查不会取消技能 → 但玩家按了移动键
- 放在输入层检测的是玩家意图，不是物理结果

`AAuraPlayerController::Move` 中：检测 `InputAxisVector.IsNearlyZero()` → 非零则调用 `ASC->CancelActiveChannelingAbilities()` → 取消了则 return（本帧不再 AddMovementInput）。

## 6. UE 资产创建状态

### 通过 MCP 已完成

| 操作 | 结果 |
|------|------|
| `DA_VitalSurge` 创建 | ✅ 从 `DA_Fireball` 复制，路径正确 |
| `GE_VitalSurge_InstantHeal` 创建 | ✅ 已创建（在 `/Game/` 根目录，需手动移动） |
| `GE_VitalSurge_HealOverTime` 创建 | ✅ 已创建（在 `/Game/` 根目录，需手动移动） |
| `BP_AuraCharacter` 引用更新 | ✅ `StartupSkillDefinitions` 含 `DA_Fireball` + `DA_VitalSurge` |

### MCP 无法完成（需手动配置）

**MCP 对 DataAsset（GE 也是 DataAsset）的 UPROPERTY 字段读写不支持**，以下必须手动完成：

#### 手动配置 DA_VitalSurge

1. 打开 `Content/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge`
2. 修改以下字段：

| 分组 | 字段 | 值 |
|------|------|------|
| Identity | SkillTag | `Ability.Vital.VitalSurge` |
| Identity | DisplayName | `生命涌动` |
| Grant\|Input | InputTag | `InputTag.2` |
| Grant\|Input | AbilityClass | `UAuraConfiguredActiveAbility` |
| Input\|Channel | InputMode | `Hold Release` |
| Input\|Channel | FullChargeDuration | `2.0` |
| Input\|Channel | PartialChargeScale | `0.5` |
| Input\|Channel | bCancelOnMoveInputDuringChannel | ✅ true |
| Targeting | TargetingMode | `Self` |
| Delivery | DeliveryMode | `Direct` |
| Effect | EffectEntries[0].EffectClass | `GE_VitalSurge_InstantHeal` |
| Effect | EffectEntries[0].Target | `Self` |
| Effect | EffectEntries[0].Condition | `FullCharge` |
| Effect | EffectEntries[0].SetByCallerMagnitudes | `SetByCaller.Healing` = 100 |
| Effect | EffectEntries[1].EffectClass | `GE_VitalSurge_HealOverTime` |
| Effect | EffectEntries[1].Target | `Self` |
| Effect | EffectEntries[1].Condition | `PartialCharge` |
| Effect | EffectEntries[1].SetByCallerMagnitudes | `SetByCaller.HealingPerTick` = 25 |

注意：`PartialCharge` 条目的 SetByCaller 写未缩放基准值。运行时会乘 `PartialChargeScale = 0.5`，所以实际每跳是 `12.5`，4 秒总共恢复 `50`。

3. 清空火球术特有字段（`ProjectileClass`、`DamageEffectClass`、`DamageTypes` 可以为空）

#### 手动配置 GE_VitalSurge_InstantHeal

1. 移动到 `/Game/Blueprints/AbilitySystem/GameplayEffects/`
2. 打开资产，配置：

| 配置项 | 值 |
|--------|-----|
| Duration Policy | `Instant` |
| Modifiers[0] Attribute | `AuraAttributeSet.Health` |
| Modifiers[0] Modifier Op | `Add` |
| Modifiers[0] Magnitude Calculation Type | `Set By Caller` |
| Modifiers[0] SetByCaller Tag | `SetByCaller.Healing` |

#### 手动配置 GE_VitalSurge_HealOverTime

1. 移动到 `/Game/Blueprints/AbilitySystem/GameplayEffects/`
2. 打开资产，配置：

| 配置项 | 值 |
|--------|-----|
| Duration Policy | `Has Duration` |
| Duration Magnitude | `4.0` |
| Period | `1.0` |
| Modifiers[0] Attribute | `AuraAttributeSet.Health` |
| Modifiers[0] Modifier Op | `Add` |
| Modifiers[0] Magnitude Calculation Type | `Set By Caller` |
| Modifiers[0] SetByCaller Tag | `SetByCaller.HealingPerTick` |

## 7. 执行的操作

| 操作 | 结果 |
|------|------|
| 编译 `AuraEditor Win64 Development` | ✅ `Result: Succeeded`，0 错误 |
| MCP 连接 UE 编辑器 | ✅ `unreal-mcp v0.6.0` |
| MCP `duplicate_asset` DA_Fireball → DA_VitalSurge | ✅ |
| MCP `create_gameplay_effect` ×2 | ✅（位置需手动调整） |
| MCP `set_property` BP_AuraCharacter.StartupSkillDefinitions | ✅ |
| MCP `save_all` | ✅ 2 个包 |
| 单人 PIE 验证 | ⚠️ 未执行（需先手动配置资产字段） |
| 多人验证 | ⚠️ 未执行 |

## 8. 单人验证步骤（需用户执行）

1. 按上述步骤手动配置 `DA_VitalSurge` 和两个 GE
2. 确认 `BP_AuraCharacter.StartupSkillDefinitions` 包含 `DA_VitalSurge`
3. PIE：
   - 按 `1` 确认火球术仍可用（不被破坏）
   - 按住 `2` 不到 2 秒后释放 → 确认持续治疗生效（4 秒内每秒跳 12.5）
   - 按住 `2` 超过 2 秒后释放 → 确认瞬间治疗 100
   - 引导期间按 WASD → 确认技能被取消，无治疗
   - 取消后可再次释放 `2` → 确认技能不卡死
   - 确认移动恢复（角色可以正常走动）
4. Output Log：
   - `[UAuraConfiguredActiveAbility] 技能 [生命涌动] 已添加引导状态`
   - `[UAuraConfiguredActiveAbility] 技能 [生命涌动] 释放：TimeHeld=X.XX, ChargeResult=...`
   - `[UAuraAbilitySystemComponent] 移动输入已取消引导技能`
   - 移动取消后无重复治疗

## 9. 多人验证

未执行。如环境能运行 Listen Server，请验证：
1. Host 短按/长按均按规则治疗
2. Client 短按/长按均按规则治疗
3. 治疗只由服务端结算
4. 一次释放不应用两次 GE（`bEffectEntriesApplied` 防护）
5. 引导取消后移动恢复
6. 其他端能看到 Health 同步

## 10. 已知问题

1. **`DA_VitalSurge` 是复制品**：字段值为火球术配置，需手动改为生命涌动配置（详见上方手动步骤）。
2. **GE 资产在 `/Game/` 根目录**：需手动移动到 `/Game/Blueprints/AbilitySystem/GameplayEffects/`。
3. **GE 默认 Duration Policy 为 Instant**：`GE_VitalSurge_HealOverTime` 需手动改为 `HasDuration` 并设置 Period。
4. **`AuraGameplayTags.cpp` 缩进不一致**：sed 替换导致部分区域使用 3-tab 缩进而非 2-tab。功能正确，但建议后续格式化。
5. **生命涌动无 Montage 表现**：本轮设计不使用 Montage，引导期间无视觉/音效反馈。后续可添加持续 Niagara 或 GameplayCue。

## 11. 下一步建议

1. **立即**：用户在 UE 编辑器中手动配置 `DA_VitalSurge` 和两个 GE，然后 PIE 验证。
2. **Codex 审查**：审查 HoldRelease 路径、EffectEntry 应用、移动取消的完整性和边界情况。
3. **格式化**：统一 `AuraGameplayTags.cpp` 缩进。
4. **后续技能**：V1-B 验证通过后，按 V1-A 顺序继续实施第三个技能。`FAuraSkillEffectEntry` 和 `EAuraSkillInputMode` 可被后续技能复用。
5. **UI 反馈**：后续可接入技能图标、冷却、引导进度条。
