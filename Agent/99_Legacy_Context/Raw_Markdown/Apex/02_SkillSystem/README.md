# Apex 技能系统设计区

生成日期：2026-07-14

这里存放 Apex 技能系统本体设计文档。Aura 原始文档已完整归档在 `Agent/Aura_Legacy_Archive/02_SkillSystem/`，本目录只放 Apex 当前仍应参考或继承的上下文。

## 当前建议读取顺序

1. `Apex_Inherited_Skill_Context.md`：从 Aura 继承到 Apex 的核心技能系统口径。
2. `Combat_Foundation_RFC.md`：Tag、属性、伤害、命中链路的 Aura 阶段 RFC，阅读时需把 `UAura*` 转译为 Apex 未来命名。
3. `Skill_Runtime_Risk_Checklist.md`：GAS、Montage、TargetData、投射物、网络同步风险清单。
4. `Montage_GameplayEvent_Timing_Decision.md`：Montage Notify 与 GameplayEvent 的职责边界。

## 注意

- 本目录不表示当前已经开始实现 GAS。
- Aura 文件中的类名、路径、资产名不是 Apex 最终命名。
- 新增 GameplayTag、属性、核心 C++ 类名必须先给用户审阅。
