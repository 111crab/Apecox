# UE_GAS_SkillGraph 参考方案评审

*评审日期：2026-06-26*  
*评审对象：`Agent/UE_GAS_SkillGraph_Design_Document.md`*  
*评审目的：判断该方案哪些思想可吸收，哪些内容需要后置，避免直接照搬。*

## 总体结论

这份参考文档很有价值，但它描述的是一个“成熟商业项目后期平台化”的终局方案，而不是 Aura 当前阶段可以直接实施的 V1 方案。

可以吸收的核心方向：
- 少量通用 `UGameplayAbility`，不为每个技能创建一个 GA 类。
- 技能以数据资产作为主入口，而不是靠一堆技能蓝图继承。
- GAS 仍负责 GE、Attribute、Tag、Cue、AbilityTask、复制和预测，不重写 GAS。
- 技能编辑器最终应该保存配置资产，不生成静态 C++ 技能类。
- 客户端结果是提议，服务器做最终裁决。
- 目标选择、Commit、动画、Cue、GE、取消、冷却都应该有统一语义。

必须后置的内容：
- 完整 SkillGraph 语言。
- 编译器、IR、Fiber、Scheduler、VM/Executor 指令系统。
- 静态网络污点分析。
- 完整 Runtime Debugger。
- Graph + Stage Timeline 双编辑器。
- 热更新、Program Hash、版本握手、Patch Manifest。
- 大型 Adapter SDK 和跨系统扩展平台。

推荐路线：先做“轻量数据驱动技能运行时”，验证机制分类和 GAS 边界；等技能语义稳定后，再决定是否升级为 SkillGraph/IR。

## 主要优点

## 1. 方向正确：通用 GA + 技能数据资产

文档主张“不为每个技能生成独立 GameplayAbility Blueprint”，这是正确的。

Aura 当前已有：
- `UAuraGameplayAbility`
- `UAuraDamageGameplayAbility`
- `UAuraProjectileSpell`
- `StartupAbilities`
- `StartupInputTag`

这些目前仍偏教程式“技能类驱动”。未来可以演进为：
- 少量通用 GA。
- `USkillDefinition` 或类似技能配置资产。
- GA 读取配置并执行目标、消耗、冷却、效果、表现。

这个方向值得吸收。

## 2. 方向正确：GAS 原生职责不要重复实现

参考文档明确提出：
- 持续时间、周期、叠层、属性修改交给 GameplayEffect。
- 状态、分类、阻断交给 GameplayTag。
- 网络化表现交给 GameplayCue。
- 异步等待优先封装 AbilityTask。

这非常重要。我们不应该做一个“绕开 GAS 的技能系统”，而应该做一个“组织 GAS 能力的技能系统”。

对 Aura 的含义：
- 现有 `UAuraAttributeSet`、`ExecCalc_Damage`、`MMC_MaxHealth`、`MMC_MaxMana` 可以继续保留。
- 后续技能系统应复用已有 GE/Tag/EffectContext，而不是另起一套 Buff/状态/数值系统。

## 3. 方向正确：技能编辑器不应该开放任意函数调用

文档禁止“调用任意 Blueprint/C++ 函数”作为正式节点，这个原则值得保留。

原因：
- 任意函数无法稳定分析网络权限。
- 取消和资源清理不可控。
- 预测可逆性无法保证。
- 项目很容易退化成“隐藏在编辑器里的临时代码”。

后续我们可以设计“受约束的扩展点”：
- Native Action。
- Targeting Provider。
- Effect Recipe。
- Presentation Action。
- Adapter。

但不应该给技能编辑器一个万能 `CallFunction` 逃生口。

## 4. 技能样例覆盖面很好

文档列出的样例很适合作为架构压力测试：
- 普通火球。
- 蓄力射击。
- 三段连招。
- 持续光束。
- 链式闪电。
- 开关光环。
- 受击反弹被动。
- 陷阱。
- 召唤物。
- Hitscan。
- 时间回溯。
- 卡牌强化下一技能。

这些样例不是都要马上做，但可以作为“机制覆盖地图”。

## 主要问题

## 1. 对当前 Aura 来说过重

完整 SkillGraph + Compiler + IR + Executor + Fiber + Scheduler 是一个大型系统。当前 Aura 还处于教程 GAS 基础阶段，如果直接冲这个目标，会先被工具链复杂度吞掉，反而很久看不到技能原型。

风险：
- 很多 C++ 基础设施先写很久，实际技能体验还没出现。
- Runtime 和 Editor 同时开工，范围失控。
- 新增复杂抽象后，现有教程代码迁移成本变高。

建议：
- V1 不做完整图语言。
- V1 先做结构化 `SkillDefinition`。
- 用枚举/结构体/数组表达技能阶段，而不是立刻做通用 IR。
- 先实现 5 到 8 个代表技能，靠真实需求倒逼抽象。

## 2. SkillGraph/IR 应该是后期升级，不是第一步

文档提出“保存或 Cook 时编译为 `FCompiledSkillProgram`，运行时执行 IR”。这是好方向，但不是第一阶段必须项。

第一阶段更适合：
- `USkillDefinition` 保存技能基础信息。
- `FSkillActivationConfig` 保存激活、输入、网络策略。
- `FSkillCostConfig` / `FSkillCooldownConfig` 保存成本和冷却。
- `FSkillTargetingConfig` 保存目标选择策略。
- `FSkillEffectConfig` 保存 GE/SetByCaller/Tag。
- `FSkillPresentationConfig` 保存动画、Cue、音效。

这种结构足够支持很多技能，也更容易在 UE Details 面板或简单 Editor Utility 里先跑起来。

等我们发现“阶段、分支、并发、子图”真的成为瓶颈，再引入 Graph。

## 3. 网络与预测目标太早拉满

文档从一开始就把预测拒绝、版本握手、网络四轨 Trace、Server Rewind 等内容纳入设计。这是高质量商业系统的要求，但对 Aura V1 太重。

建议 V1 网络目标：
- 保留服务器权威原则。
- 玩家 ASC 继续放 `PlayerState`，保持 Mixed 复制。
- 敌人 ASC 继续 Minimal。
- LocalPredicted 先只覆盖输入响应、动画、简单表现。
- 伤害、资源、Spawn 先服务器权威。
- 不立刻做 Server Rewind。
- 不立刻做版本握手/热更新。

## 4. 编辑器设计过于终局

Graph + Timeline + Preview World + Network Preview + Runtime Debugger + Diff + 批量编辑，都是正确方向，但不能作为第一版编辑器目标。

Aura 适合的编辑器路线：

第一阶段：
- 先没有专用编辑器，只用 `DataAsset + Details Panel`。
- 或者做很小的 Editor Utility，用于创建模板技能资产。

第二阶段：
- 自定义资产类型。
- 基础 Asset Editor。
- 预览目标、范围、Cue。

第三阶段：
- Timeline。
- Graph。
- Debugger。
- Diff。

也就是说：编辑器应跟随运行时成熟度，而不是先做大编辑器。

## 5. 模块拆分不适合当前项目立即采用

参考文档建议很多模块：
- `SkillCoreRuntime`
- `SkillGAS`
- `SkillNodesCore`
- `SkillCompiler`
- `SkillEditor`
- `SkillAdapters.*`
- `SkillTests`

长期看合理，但当前 Aura 只有一个 `Aura` Runtime 模块，还没有 Aura 自己的 Editor 模块。

建议：
- V1 先在 `Aura` Runtime 下增加清晰子目录，例如 `AbilitySystem/SkillSystem/`。
- 等第一个运行时原型稳定，再考虑抽成插件或模块。
- 技能编辑器开始前，再新增 `AuraEditor` 或独立 editor plugin。

## 可吸收为我们 V1 原则的内容

建议立刻吸收：
- 技能不等于 GA 类；技能应该有稳定 `SkillId`。
- 技能配置应能描述输入、成本、冷却、目标、效果、表现。
- Commit 不一定在 Ability 开头，应由技能阶段决定。
- 成本和冷却应通过 GAS GE/Tag 实现。
- 目标数据来自客户端时必须由服务器验证。
- 动画 Notify 只能作为调度信号，不应单独决定权威伤害。
- GameplayCue 只做表现，不改变玩法状态。
- 目标选择、效果应用、表现播放应分层。
- 被动监听应避免无限递归，并需要内部冷却/触发上限。

## 建议后置的内容

建议后置到 V2/V3：
- 完整 Graph 节点系统。
- IR 指令集。
- Fiber/Scheduler。
- 编译器 CFG/类型/污点分析。
- Program Hash 和版本握手。
- 热更新体系。
- Runtime Debugger。
- Diff 和批量编辑。
- Adapter SDK。
- Server Rewind。
- 时间回溯。
- 卡牌修改下一技能这类运行时 Program 变体。

## 建议立即讨论的技能机制分类

为了“支持市面常见技能机制”，我们应先分类，而不是直接开写。

建议第一批机制族：

| 机制族 | 代表技能 | 主要验证点 |
| --- | --- | --- |
| 瞬发效果 | 自我治疗、直接伤害 | 成本、冷却、GE 应用、UI 查询 |
| 投射物 | 火球 | 目标点、Spawn、EffectContext、命中结算、Cue |
| 地面范围 | 暴风雪/火焰地面 | 位置确认、范围查询、持续 GE、范围提示 |
| Buff/Debuff | 加速、减速、护盾、灼烧 | Duration、Stack、Tag 阻断、GameplayCue 持续表现 |
| 蓄力/吟唱 | 蓄力射击 | Reserve/Confirm/Refund、取消、释放时机 |
| 持续引导 | 光束、持续治疗 | 周期成本、目标刷新、取消、Tick 预算 |
| 近战/连招 | 三段攻击 | Montage、Notify、输入窗口、命中窗口 |
| 被动触发 | 受击反弹、击杀回蓝 | 事件监听、内部冷却、防递归 |
| 光环/开关 | 持续光环 | Infinite GE、开关状态、移除清理 |
| 召唤/陷阱 | 召唤物、地雷 | 持久 Actor、所有权转移、生命周期清理 |

## 推荐第一批原型

不要一上来做 12 个。建议第一轮做 6 个，覆盖最大机制面积：

1. **瞬发治疗/伤害**
   - 最简单，用来验证 `SkillDefinition`、成本、冷却、GE 应用。

2. **火球投射物**
   - 复用现有 `UAuraProjectileSpell` 思路，验证数据驱动投射物技能。

3. **范围持续地面技能**
   - 验证地面目标、AoE、持续效果、GameplayCue。

4. **蓄力射击**
   - 验证输入按住/松开、取消、资源预留、释放强度。

5. **三段连招**
   - 验证 Montage、输入窗口、命中窗口、近战 Trace。

6. **受击反弹被动**
   - 验证被动事件监听、内部冷却、防递归。

第二轮再加入：
- 持续光束。
- 链式闪电。
- 开关光环。
- 陷阱。
- 召唤物。

## 建议的下一步

下一步不应该马上写代码，而是写一份我们自己的：

`Agent/02_SkillSystem/Skill_Mechanic_Taxonomy.md`

它应该回答：
- 市面技能机制怎么分类。
- 哪些机制本质相同。
- 哪些机制应该由 GE/Tag/Cue/GAS 原生能力承担。
- 哪些机制需要我们新增 Runtime 能力。
- 第一批技能原型是哪几个。

之后再写：

`Agent/02_SkillSystem/GAS_Architecture_Options.md`

对比：
- 方案 A：每个技能一个 GA。
- 方案 B：通用 GA + 结构化 DataAsset。
- 方案 C：通用 GA + SkillGraph/IR。
- 方案 D：先 B 后 C 的渐进路线。

当前我的倾向是 **方案 D：先做通用 GA + 结构化 DataAsset，等需求稳定后再升级 SkillGraph/IR**。
