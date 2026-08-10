# Phase 1C 二次审查小修报告

**日期**：2026-08-10  
**审查来源**：Codex — `Agent/Reviews/2026-08-10_Phase1C_Death_Respawn_Lifecycle_Second_Review.md`  
**状态**：等待 Codex 最终审查

---

## 一、修改文件列表

| 文件 | 修改内容 | 对应审查问题 |
|------|----------|-------------|
| `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | 重构 `PostGameplayEffectExecute` 统一收口 | P1: MaxHealth 连带压低 Health 事件丢失 / P2: bOutOfHealth 实时终值 |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 晚解绑分支添加 `UninitializeFromAbilitySystem` | P2: 旧 Pawn 晚解绑未反初始化 |
| `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp` | `ensureMsgf` + 注释修正 | P2: 静默失败 / 注释 |

---

## 二、逐问题修复详情

### P1：MaxHealth 连带降低 Health 的事件仍然丢失 + P2：bOutOfHealth 应使用实时终值

**文件**：`ApecoxVitalAttributeSet.cpp`

**问题**：`PostAttributeChange(MaxHealth)` 已通过 `SetNumericAttributeBase` 把 Health 压低到新上限，进入 `PostGameplayEffectExecute` 的 MaxHealth 分支时 `CurrentHealth > NewMaxHealth` 已为 false，连带 HealthChanged/OutOfHealth 分支永远不执行。

**修复**：将 `PostGameplayEffectExecute` 重构为统一收口：

1. **先钳制**：仅 Health 属性在 Post 阶段完成最终钳制。MaxHealth 已在 PreAttributeChange 中完成钳制，不重复操作。

2. **统一比较**：不再分两个属性分支各做各的广播，而是用 GE 前快照与当前最终值统一比较：
   - `MaxHealthBeforeAttributeChange != GetMaxHealth()` → 广播 `OnMaxHealthChanged`
   - `HealthBeforeAttributeChange != GetHealth()` → 广播 `OnHealthChanged`
   - 此快照比较能**正确检测** PostAttributeChange 已连带的 Health 变化，不依赖 `CurrentHealth > NewMaxHealth` 二次推断

3. **OutOfHealth 实时终值判断**：`OnHealthChanged` 广播后重新读取 `GetHealth()`（回调可能已治疗），再决定是否需要广播 `OnOutOfHealth`。只在新 Pawn 初始 0 属性保护条件（`OldHealth > 0.0f`）下触发。

4. **门控终值更新**：所有回调完成后执行 `bOutOfHealth = (GetHealth() <= 0.0f)`，而非硬编码 `true`。若回调中实现了免死/治疗/最后机会效果，后续死亡仍可正常触发。

5. **删除未使用局部变量** `ASC`。

**修改的函数**：`UApecoxVitalAttributeSet::PostGameplayEffectExecute`

**最终属性事件顺序**：

```
PostGameplayEffectExecute:
  1. 钳制 Health（如果需要）
  2. 若 GetMaxHealth() != MaxHealthBeforeAttributeChange → OnMaxHealthChanged.Broadcast
  3. 若 GetHealth() != HealthBeforeAttributeChange → OnHealthChanged.Broadcast
  4. 重新读取 GetHealth()
  5. 若 GetHealth() <= 0 && OldHealth > 0 && !bOutOfHealth → OnOutOfHealth.Broadcast
  6. bOutOfHealth = (GetHealth() <= 0.0f)
```

---

### P2：旧 Pawn 晚解绑分支没有反初始化 HealthComponent

**文件**：`ApecoxPlayerCharacter.cpp`

**问题**：当 ASC Avatar 已切到新 Pawn 时，旧 Pawn 的 `UninitializeAbilitySystem` 只移除了 Character 自己的死亡委托，没有调用 `HealthComponent->UninitializeFromAbilitySystem()`。旧组件继续绑定持久化 VitalAttributeSet，可能收到属于新 Pawn 的属性事件。

**修复**：else 分支中，移除死亡委托后追加 `HealthComponent->UninitializeFromAbilitySystem()` 调用。组件内部已有 `ASC Avatar == Owner` 守卫，只会解除旧委托，不会清除新 Pawn 的死亡 Tag。

**修改的函数**：`AApecoxPlayerCharacter::UninitializeAbilitySystem` (else 分支)

---

### P2：DeathAbility 找不到 HealthComponent 时静默失败 + 注释修正

**文件**：`ApecoxDeathAbility.cpp`

**问题**：
- `FinishDeathAndEndAbility()` 找不到 HealthComponent 时静默设置 `bDeathFinished = true` 并结束，无诊断信息
- `CancelAbilities` 第二个参数注释误写为"不过滤拥有者"，实际是 `WithoutTags`

**修复**：
1. `FinishDeathAndEndAbility`：找不到 HealthComponent 时使用 `ensureMsgf` 报告错误，然后安全结束 Ability
2. 修正注释：第二个 nullptr 标注为 `WithoutTags`（不排除特定 Tag 的 GA）

---

## 三、构建结果

**git diff --check**：✅ 通过（仅有 LF→CRLF 工作树警告，无空白错误）

**构建命令**：
```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

**结果**：✅ **Succeeded**（0 错误，0 新增警告）

---

## 四、未做操作声明

- ❌ 未创建/修改/删除任何 `.uasset` 或 `.umap`
- ❌ 未操作 UE 编辑器
- ❌ 未执行任何 Git 命令
- ❌ 未新增功能、未扩展护盾/UI/动画/伤害类型

---

**修复完成，等待 Codex 最终审查。**
