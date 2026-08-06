# Apex ProjectileCast 第三轮代码审查

审查日期：2026-07-29

结论：**第二轮修复已完成大部分运行链路，但仍漏做第二轮 Prompt 中的明确要求，暂不进入 UE 资产配置。**

## 已确认修复

- 已移除 UE 5.8 弃用的 `StructUtils` 模块和插件依赖，同时保留 `TInstancedStruct`。
- Montage 已在 `CommitAbility()` 前同步加载并检查返回值。
- Aim TargetData 已增加 Data/Cancelled 双路径、服务器 delegate 配对解绑、安全副本和预测客户端 RPC 分支。
- Projectile 已改为 `bool InitializeProjectile()`，并检查 Authority、Definition、Impact Spec 和运行时数值。
- Projectile 已显式设置速度，使用 Blocking + ProjectileMovement Sweep，并在客户端关闭碰撞。
- `HandleImpact()` 已采用 Authority-first，只在服务器应用 GE、执行 Cue 和销毁。

## 仍需修复

### P1：SkillDefinition 没有真正调用模板校验

文件：`Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp:12`

当前只读取了模板 CDO 并检查 ExecutionConfig 类型，没有调用：

```cpp
CDO->ValidateSkillDefinition(*this, Context)
```

因此 ProjectileCast 的 Montage、EventTag、ProjectileDefinition 等模板校验仍是死代码，`ActivationGroup::MAX` 也没有在根资产校验中收口。

### P1：缺失 SkillDefinition 的 Ability 仍被允许激活

文件：`Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp:48`

当前代码仍为：

```cpp
if (!SkillDef) return true;
```

这会让缺失 `AbilitySpec::SourceObject` 的配置驱动技能绕过成本、冷却和 Policy。必须记录 Warning 并返回 `false`；`ActivationGroup::MAX` 也必须拒绝激活。

### P1：投射物仍未忽略 Owner / Instigator

文件：`Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp:56`

初始化时没有调用 `CollisionSphere->IgnoreActorWhenMoving()`。投射物从角色手部生成时仍可能首先阻挡在施法者身上；虽然 `HandleImpact()` 拒绝自伤，但 ProjectileMovement 已停止，投射物会滞留到寿命结束。

### P1：Aim Task 的本地失败保护不完整

文件：`Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp:63`

Controller 和 Deproject 已收口，但以下情况仍未统一走 Cancel：

- Avatar 为空；
- World 为空，当前会直接解引用；
- Viewport 宽或高不大于 0。

### P2：有限值与引用资产校验仍不完整

- `UApexProjectileCastAbility::ActivateAbility()` 没有对 `MaxAimDistance` 做 `FMath::IsFinite()` 运行时检查。
- `UApexProjectileDefinition::IsDataValid()` 仍只比较大小，没有拒绝 NaN/Infinity。
- ProjectileCast 模板校验只检查 `ProjectileDefinition` 非空，没有合并该引用资产的 `IsDataValid()` 结果。

### P2：实施报告仍是第一轮旧报告

`Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md` 没有覆盖，仍包含：

- 已不存在的 `AbilitySystem/Shared` 路径；
- 已移除的 StructUtils 模块/插件依赖；
- 已取消的 BlueprintNativeEvent 设计；
- 第一轮编译耗时和文件清单。

## 编译状态

Codex 尝试复编译时，`Build.bat` 持续提示已有构建脚本占用 Mutex，因此本轮没有取得新的可信编译结果。子代理完成定点修复后必须重新编译，并在报告中记录真实输出；不得沿用第一轮结果。

## 第三轮修复后的最终复查

Codex 于第三轮修复后再次复查并确认：

- SkillDefinition -> 模板 CDO Data Validation 已接通。
- 坏 Spec / SourceObject / ActivationGroup MAX 已拒绝激活。
- ProjectileDefinition 有限值校验和 Projectile Owner/Instigator 忽略已完成。
- Codex 在沙箱外实际编译成功：`Result: Succeeded`，耗时 `1.99 秒`，无 StructUtils 弃用警告。

仍有两个收口项：

1. `UApexAbilityTask_WaitAimTargetData::SendAimTargetData()` 仍未拒绝空 Avatar 和零尺寸 Viewport，并在 ASC 空指针检查前构造 `FScopedPredictionWindow`。
2. 实施报告仍完全是第一轮旧内容，没有覆盖第三轮 Prompt 明确要求的最终报告。

因此代码主体已稳定，但在上述微型收口和报告覆盖前，仍不进入 UE 资产配置。

## 最终确认

Final Closeout 后：

- Avatar、World、PlayerController、Viewport 和 ASC 的失败路径已在 PredictionWindow 前收口。
- SourceLocation 使用已验证的 Avatar，Trace 使用已验证的 World。
- 最终实施报告已覆盖，旧 Shared、BlueprintNativeEvent 和 StructUtils 依赖描述已清除。
- Codex 最终复编译：`Result: Succeeded`，耗时 `1.62 秒`，0 error，无弃用警告。

**最终代码审查通过。** 下一步是用户审核公开类型和关键命名，确认后进入 UE 资产配置与单人/多人 PIE 验证。
