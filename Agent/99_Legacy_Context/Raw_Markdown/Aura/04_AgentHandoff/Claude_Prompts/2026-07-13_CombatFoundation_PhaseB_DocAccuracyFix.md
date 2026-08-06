# ClaudeCode 修复 Prompt：Combat Foundation Phase B 文档准确性修正

创建日期：2026-07-13  
状态：Codex 审查后要求修复  
范围：只修中文报告和说明文档，不改 C++，不改 GameplayTags 配置

## 任务目标

本轮只修正 Phase B 报告和运行时说明中的事实错误。不要改代码，不要改 `.uasset`，不要重新设计伤害系统。

需要修正的文件：

```text
Agent/04_AgentHandoff/Claude_Reports/2026-07-13_CombatFoundation_PhaseB_MinimalTagsAndSimpleDamage_Report.md
Agent/02_SkillSystem/Combat_Foundation_Runtime_Notes.md
```

## 必须修复的问题

### 1. `SetByCaller.Damage` 当前并没有被 C++ 伤害路径使用

报告中目前写法类似：

```text
SetByCaller.Damage：现有 ExecCalc_Damage 和 AuraProjectileSpell 使用
```

这是错误的。

当前 C++ 实际情况：

- `AuraProjectileSpell.cpp` 遍历 `DamageTypes`，用 `Damage.Fire` / `Damage.Lightning` / `Damage.Arcane` / `Damage.Physical` 这类 Damage Type Tag 作为 SetByCaller key。
- `ExecCalc_Damage.cpp` 遍历 `FAuraGameplayTags::DamageTypesToResistances`，用上述 Damage Type Tag 调用 `Spec.GetSetByCallerMagnitude(DamageTypeTag, false, 0.f)`。
- `SetByCaller.Damage` 目前只在 `Config/DefaultGameplayTags.ini` 中存在，当前 C++ 未读取它。

请把报告和说明文档改成：

- `SetByCaller.Damage` 是未来通用基础伤害 key 的候选/预留。
- 当前教程伤害路径仍使用 `Damage.Fire` / `Damage.Lightning` / `Damage.Arcane` / `Damage.Physical` 作为 SetByCaller key。
- 下一阶段如果要让新技能使用 `SetByCaller.Damage`，必须先实现映射或更新 GE / ExecCalc，否则当前 `ExecCalc_Damage` 会读不到这个数值，导致伤害为 0。

### 2. 当前 `ExecCalc_Damage` 不是简单公式

报告和 `Combat_Foundation_Runtime_Notes.md` 中目前把当前公式描述得过于简单，例如：

```text
Damage = Sum of (SetByCaller damage values × (1 - Resistance%))
```

这不完整，容易误导后续规划。

当前 `ExecCalc_Damage.cpp` 实际仍保留教程复杂计算：

- 遍历 `DamageTypesToResistances` 并扣抗性百分比。
- 计算 `BlockChance`，格挡后伤害减半。
- 读取 `Armor` 和 `ArmorPenetration`，通过曲线计算有效护甲。
- 读取 `CriticalHitChance`、`CriticalHitResistance`、`CriticalHitDamage`，计算暴击。
- 最终写入 `IncomingDamage`。

请把报告和说明文档改成：

- Phase B 没有新增复杂公式。
- 但当前项目仍然保留教程时代的复杂 `ExecCalc_Damage`。
- “简单伤害表现”是我们的下一阶段目标口径，不是当前代码已经完全做到。
- 下一阶段技能如果想保持最小验证，可以选择继续复用旧 ExecCalc，或另建/配置一个真正简单的临时 GE 路径，这需要单独决策。

### 3. Lightning Resistance Tag 大小写报告错误

报告中目前写：

```text
Attributes_Resistance_Lightning -> Attributes.Resistance.Lightning
```

实际代码是：

```cpp
FName("Attributes.Resistance.lightning")
```

请把报告改成实际值：

```text
Attributes_Resistance_Lightning -> Attributes.Resistance.lightning
```

并标注：这是旧教程遗留大小写问题，本轮未修复，后续 Tag 规范化阶段处理。

## 不要做

- 不要修改 `Source/` 下任何 C++ 文件。
- 不要修改 `Config/DefaultGameplayTags.ini`。
- 不要新增或删除 Tag。
- 不要重新运行 UE 编辑器或 MCP。
- 不要创建或修改 `.uasset`。
- 不要把旧 ExecCalc 改成简单公式。

## 验证要求

本轮只改 Markdown，不需要编译。

但报告中要写明：

```text
本轮为文档准确性修复，未改 C++，未重新编译。上一轮构建结果仍为 Result: Succeeded。
```

## 完成标准

- 报告不再声称 `SetByCaller.Damage` 当前被 `ExecCalc_Damage` / `AuraProjectileSpell` 使用。
- 运行时说明不再把当前 `ExecCalc_Damage` 描述成单纯简单公式。
- Lightning tag 的大小写记录与代码一致。
- 没有代码改动。
