# V1-A 火球术配置化运行时阶段收口报告

日期：2026-07-07

## 当前结论

V1-A 的第一个代表技能“火球术”已经完成最小配置化运行时闭环。当前版本不是最终技能编辑器，也不是完整技能框架终态；它的价值是验证方案 C 的基础路线可行：

1. 通过少量通用 C++ Ability 承载运行时流程。
2. 通过 `UAuraSkillDefinition` 数据资产决定技能配置。
3. 通过原生 C++ 片段处理高风险逻辑，例如投射物、伤害 GE、TargetData、Montage 事件和网络权威。

## 已完成内容

1. 新增 `UAuraSkillDefinition`，用于保存技能的输入 Tag、Ability 类、Montage、投射物、伤害 GE、伤害类型等配置。
2. 新增 `UAuraConfiguredActiveAbility`，用于读取 SkillDefinition 并执行配置化主动技能流程。
3. 扩展角色启动技能授予逻辑，使角色可以通过 SkillDefinition 授予技能。
4. 创建 `DA_Fireball`，让火球术从配置资产进入运行时。
5. 保留旧教程火球链路，当前阶段没有强行删除旧 `GA_FireBolt` 和旧 StartupAbilities。
6. 修复 Montage / GameplayEvent / TargetData / 投射物生成链路中的多处生命周期问题。
7. 修复 `ExecCalc_Damage` 在遍历未配置伤害类型时输出 SetByCaller Error 的问题。
8. 补充 `Skill_Runtime_Risk_Checklist.md`，沉淀这次发现的 GAS 运行时风险。

## 已验证内容

用户已在编辑器中完成当前最小验证：

1. 单人情况下火球术可以正常触发。
2. Listen Server 下客户端释放火球，Host 能看到表现。
3. Listen Server 下 Host 释放火球，客户端能看到表现。
4. 伤害没有明显重复应用。
5. 修复后 `AuraEditor Win64 Development` 编译通过。

编译命令：

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' AuraEditor Win64 Development -Project='D:\UnrealProject\Aura\Aura.uproject' -WaitMutex -MaxParallelActions=4
```

## 仍需留意

1. 当前验证主要覆盖单人和 Listen Server，不等于 Dedicated Server 完整验收。
2. 当前火球术仍是 V1-A 原型，后续要继续抽象技能片段，而不是把所有机制都塞进一个 Ability 类。
3. Montage 被死亡、眩晕、切换状态等战斗中断打断时的通用规则还未完整设计。
4. `AuraPlayerState` 仍有 UE 5.8 的 `NetUpdateFrequency` 弃用警告，属于后续清理项。
5. `StructUtils` 插件有弃用提示，当前不是火球术功能阻塞项。

## 下一阶段建议

下一步建议不要立刻做技能编辑器。更稳的顺序是继续做 V1-A 的第二个代表技能，用不同机制验证“配置资产 + 原生片段”的扩展能力。

候选方向：

1. 瞬发自我 Buff / 治疗：验证无投射物、无 TargetData 或弱 TargetData 的技能。
2. 范围落点技能：验证位置 TargetData、范围判定、延迟生效。
3. 召唤 / 陷阱：验证生命周期、Actor 所有权、持续存在、销毁规则。

在进入第二个技能前，应先确认它要覆盖哪一类机制，以及复用哪些现有资产。
