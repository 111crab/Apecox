# GAS 技能运行时架构方案对比

*创建日期：2026-06-26*  
*状态：Phase 2 已确认方向。V1 采用“克制版方案 C”：少量通用 GA + DataAsset 配置 + 少量原生机制片段。*

## 目的

现在第一批技能机制已经确认，下一步不是立刻写代码，而是选定 V1 运行时架构。

这个文档回答三件事：
- “少量 C++ 运行时能力 + 数据配置资产”到底是什么意思。
- 为什么不建议第一版就做完整 SkillGraph / IR / VM。
- 第一批 6 个技能原型分别会要求运行时具备哪些能力。

## 当前 Aura 起点

现有 Aura 代码已经有教程阶段 GAS 基础：
- `UAuraGameplayAbility`：当前主要承载 `StartupInputTag`。
- `UAuraDamageGameplayAbility`：承载 `DamageEffectClass` 和 `DamageTypes`。
- `UAuraProjectileSpell`：具体投射物技能类，服务器生成 `AAuraProjectile`，命中后应用 `DamageEffectSpecHandle`。
- `UAuraAbilitySystemComponent`：负责初始技能授予和输入 Tag 到 AbilitySpec 的映射。
- `UAuraAttributeSet`：已经有生命、法力、主属性、副属性、抗性、`IncomingDamage` 等属性。

这说明项目不是白纸。我们要做的是把“教程里的具体技能类”逐步升级为“可配置、可扩展、可编辑器化”的技能运行时。

## 方案 A：每个技能一个独立 GA 类

每个技能都写一个 `UGameplayAbility` 子类，例如 `UGA_Fireball`、`UGA_Heal`、`UGA_ComboAttack`。

优点：
- 最直观，适合学习 GAS 基础。
- 每个技能可以自由写逻辑，短期实现快。
- Debug 时容易定位到具体类。

缺点：
- 技能数量一多，会变成“一个机制一套代码”。
- 很难做通用技能编辑器，因为编辑器只能改资产参数，不能安全地生成新的 C++ 逻辑。
- 重复逻辑会快速增加，例如成本、冷却、目标过滤、表现、命中结算。
- 多人验证容易分散，每个技能都可能用不同方式处理权威端、表现端和状态同步。

适合：
- 教程学习。
- 少量特殊 Boss 技能。
- V1 里保留少数“无法通用化”的 escape hatch。

不适合：
- 作为 Aura 长期主路线。

## 方案 B：少量通用 GA + DataAsset 配置

少量 C++ 类负责通用执行流程，具体技能由配置资产描述。

这里的“少量 C++ 运行时能力”指的是：
- 通用激活流程：检查 Tag、成本、冷却、输入时机、取消。
- 通用目标选择：自身、单体、地面点、范围、投射物命中、近战 Trace。
- 通用效果应用：Instant GE、Duration GE、Periodic GE、SetByCaller 数值。
- 通用表现入口：GameplayCue、Montage、Niagara、音效、UI 提示。
- 通用生命周期对象：投射物 Actor、范围 Area Actor、持续效果句柄。

这里的“数据配置资产”指的是：
- 技能 ID / 技能 Tag / 输入 Tag。
- 成本、冷却、施法时间、取消规则。
- 目标选择模式和过滤规则。
- 伤害/治疗/Buff/Debuff 参数。
- 投射物类、速度、生命周期、命中规则。
- 范围半径、持续时间、周期。
- Montage、GameplayCue、音效、图标等表现配置。

注意：这不等于运行时生成新的 C++ 类。更合理的做法是：
- 使用少量通用 GA 类。
- 使用 DataAsset 保存技能配置。
- 运行时根据配置创建 `GameplayEffectSpec`、填充 SetByCaller、触发 GameplayCue、生成通用 Actor。
- 后续技能编辑器保存配置资产，而不是生成静态 C++ 文件。

优点：
- 最符合“技能编辑器最终只保存配置”的长期目标。
- 第一批 6 个原型大多可以通过配置覆盖。
- 通用规则集中，方便做多人验证和调试。
- 未来可以在编辑器 UI 上暴露配置，而不必暴露 C++ 类。

缺点：
- V1 需要先设计好配置结构，否则会变成“大而全但难用”的 DataAsset。
- 某些机制仍需要原生 C++ 扩展点，例如复杂蓄力、连招窗口、投射物命中、被动监听。
- 如果所有行为都硬塞进一个通用 GA，代码会变成巨大分支。

适合：
- Aura V1 主路线。

## 方案 C：通用 GA + 原生机制片段

这是方案 B 的增强版：DataAsset 不只配置数值，还引用一组 C++ 原生机制片段。

可以把技能拆成几个阶段：
- Activation：如何激活，是否需要按住、松开、确认。
- Targeting：如何获得目标。
- Delivery：如何把效果送到目标，例如瞬发、投射物、范围、近战 Trace。
- Effect：应用哪些 GameplayEffect 或 SetByCaller 数值。
- Presentation：播放哪些 Montage / GameplayCue / 音效 / UI。
- Cleanup：持续技能、Area Actor、被动监听如何结束。

每类阶段由少量 C++ 策略类或结构体实现，DataAsset 选择和配置它们。

优点：
- 比单个巨大通用 GA 更清晰。
- 机制可复用，技能编辑器也更容易按模块展示。
- 对第一批技能覆盖更自然，例如投射物和范围持续技能只是在 Delivery 上不同。

缺点：
- 设计比方案 B 更复杂。
- 需要非常克制，避免第一版就做成小型脚本语言。

适合：
- Aura V1 推荐路线，但要控制第一版规模。

## 方案 D：完整 SkillGraph / IR / VM

技能编辑器直接使用图节点，保存图，编译成 IR，再由运行时解释或执行。

优点：
- 长期上限高。
- 可以表达复杂技能逻辑。
- 和“技能编辑器”愿景最接近。

缺点：
- 对 V1 太重。
- 需要节点系统、编译器、校验器、调试器、网络安全模型、版本管理。
- 如果 GAS 基础运行时还没稳定，图编辑器会把问题放大。
- 很容易先做出“看起来像编辑器”的东西，但底层技能语义不稳定。

适合：
- 中后期，当我们已经明确哪些机制真正需要图表达时再升级。

不适合：
- 第一批技能原型之前。

## 已确认路线：分阶段混合方案

V1 已确认采用“方案 C 的克制版本”：

1. **保留现有教程 GAS 基础，不大拆。**  
   先把现有 `UAuraProjectileSpell` 视为投射物能力样本，而不是马上重写所有教程代码。

2. **新增技能配置资产。**  
   例如 `UAuraSkillDefinition`，记录技能 Tag、输入、成本、冷却、目标、效果、表现等配置。

3. **新增少量通用运行时能力。**  
   不为每个技能写一个全新 GA，而是让通用 GA 根据配置运行。

4. **机制片段先少后多。**  
   第一版只实现能支撑 6 个原型的片段，例如 Instant、Projectile、Area、Charge、Combo、PassiveTrigger。

5. **特殊复杂逻辑保留 C++ 扩展点。**  
   例如蓄力、连招、被动反弹可能先用原生 AbilityTask / Component / Strategy 实现，而不是强行数据化所有行为。

6. **GameplayEffect 尽量使用模板 + Spec 参数。**  
   通过 SetByCaller、Tag、EffectContext 注入运行时数值，避免为每个技能创建大量静态 GE 类。

7. **GameplayCue 负责表现，不负责玩法结算。**  
   Niagara、音效、命中特效、持续范围表现可以走 Cue 或表现配置，但伤害、治疗、Buff、Debuff 仍由权威端 GAS 结算。

## 第一批原型对架构的压力测试

| 原型 | 最低架构能力 | 特别注意 |
| --- | --- | --- |
| 瞬发治疗/伤害 | 配置目标、创建 GE Spec、应用到 Self/Target | 成本和冷却必须统一处理 |
| 火球投射物 | 配置投射物 Delivery、服务器生成 Actor、命中应用 Spec | 投射物表现可复制，但命中结算必须服务器权威 |
| 范围持续地面技能 | 配置地面 Targeting、Area 生命周期、周期 GE | 范围 Actor 或查询服务要有清理规则 |
| 蓄力射击 | 输入按住/松开、蓄力时间映射数值、取消 | 预测和 Commit 时机需要单独设计 |
| 三段连招 | 阶段状态、输入窗口、Montage 事件、近战 Trace | Notify 只能调度，不能单独决定权威伤害 |
| 受击反弹被动 | 被动注册、事件监听、内部冷却、防递归 | 避免反弹触发反弹造成循环 |

## 多人设计底线

第一批每个技能都要能回答：
- 谁发起激活请求？
- 谁拥有权威结果？
- 哪些内容需要复制？
- 哪些内容只是本地表现？
- Listen Server 下 Host 既是服务器又是本地玩家时，逻辑是否会重复执行？
- 其他客户端是否能看到合理表现？
- 其他角色受到的伤害、治疗、Buff/Debuff 是否由服务器正确应用？

## 暂不做的事情

V1 暂不做：
- 完整图编辑器。
- 自定义 IR / VM / 调度器。
- 运行时生成 C++ 类。
- 完整 Dedicated Server 压测。
- 任意函数调用节点。
- 面向所有游戏类型的最终形态编辑器。

这些不是否定，而是后置。我们先把可解释、可验证、可学习的 GAS 运行时打牢。

## 下一步

下一步写 `Skill_Runtime_Model.md`，把推荐路线拆成具体对象：
- `UAuraSkillDefinition` 应该包含什么。
- 通用 GA 应该如何读取配置。
- Targeting / Delivery / Effect / Presentation 各自边界是什么。
- 第一批 6 个技能如何映射到这个模型。
- 哪些代码由 Claude 第一批实现，哪些只是预留接口。

顺序已确认：
1. 先写 `Skill_Runtime_Model.md`。
2. 再做轻量资产盘点。
3. 再写第一份 Claude 实施 Prompt。
4. 在单技能 Prompt 被用户确认前，不进入 C++ 实施。
