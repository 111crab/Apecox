# Claude Code 执行 Prompt：Phase 1C 二次审查小修

项目：`D:\UnrealProject\Apecox`

先完整阅读：

- `Agent/Reviews/2026-08-10_Phase1C_Death_Respawn_Lifecycle_Second_Review.md`
- 当前相关源码。

本轮只修二次审查列出的剩余问题，不新增功能、不操作 UE 资产、不执行 Git 命令。

## 必须修改

1. 重构 `UApecoxVitalAttributeSet::PostGameplayEffectExecute()` 的收口逻辑：
   - 先完成当前属性所需钳制；
   - 再统一使用 GE 前快照与当前最终值比较，按实际变化广播 MaxHealthChanged/HealthChanged；
   - MaxHealth 连带压低 Health 时必须广播 HealthChanged，归零时必须能够广播 OutOfHealth；
   - OnHealthChanged 回调后重新读取实时 Health，再决定是否 OutOfHealth；
   - 所有回调完成后执行 `bOutOfHealth = (GetHealth() <= 0.0f)`；
   - 不使用已经必然失效的 `CurrentHealth > NewMaxHealth` 二次判断；
   - 保持“只有从正数跨入 <=0 才产生死亡”的规则，避免初始 0 属性误触发死亡。
2. `AApecoxPlayerCharacter::UninitializeAbilitySystem()` 的旧 Pawn 晚解绑分支也调用
   `HealthComponent->UninitializeFromAbilitySystem()`；依赖其 Avatar 守卫只解除旧组件委托，
   不清除新 Pawn Tag。
3. `UApecoxDeathAbility::FinishDeathAndEndAbility()` 找不到 HealthComponent 时使用 `ensureMsgf`，
   然后安全结束 Ability；不要静默当作已成功 FinishDeath。
4. 修正 `CancelAbilities` 第二参数注释为 `WithoutTags`；删除未使用局部变量和相关文件末尾多余空行。

## 验证与停止条件

1. 运行 `git diff --check`。
2. 构建 `ApecoxEditor Win64 Development`。
3. 新建中文报告：
   `Agent/Reports/2026-08-10_Phase1C_Death_Respawn_Lifecycle_Second_Fix_Report.md`
4. 报告列出修改的函数、最终属性事件顺序、构建结果及未执行 UE/Git 操作声明。
5. 构建成功并写完报告后立即停止，等待 Codex 最终审查。
