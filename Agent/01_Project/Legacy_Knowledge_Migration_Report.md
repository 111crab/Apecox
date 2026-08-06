# Aura/Apex 技术上下文迁移报告

生成日期：2026-08-05
目标项目：`D:/UnrealProject/Apecox`

## 1. 迁移结果

已建立三层知识结构：

### 活跃层

`Agent/00_Coordination`、`01_Project`、`02_CombatFramework`、`03_EditorVision`。

用途：当前工作、顶层路线、已确认决策、代码设计、UE 手工操作和 Apecox 综合架构。

### 精选参考层

`Agent/90_References/Inherited_Framework`。

保留 14 份高价值旧文档原文：

- `Lyra_GAS_Architecture_Study.md`
- `Skill_Framework_Extensibility_Synthesis.md`
- `AnimBP_Architecture_Study.md`
- `AttributeSet_Best_Practices_Summary.md`
- `Combat_Foundation_RFC.md`
- `Skill_Runtime_Risk_Checklist.md`
- `Montage_GameplayEvent_Timing_Decision.md`
- `SkillSystem_Architecture_Refactor_Summary.md`
- `GAS_Architecture_Options.md`
- `Skill_Runtime_Model.md`
- `Skill_Mechanic_Catalog.md`
- `Representative_Skill_Portfolio.md`
- `UE_GAS_SkillGraph_Design_Review.md`
- `New_FPS_Project_Direction_And_Asset_Baseline.md`

### 全量历史层

`Agent/99_Legacy_Context`。

- Aura/Apex 共扫描 208 个 Markdown 来源。
- 按 SHA256 去重后复制 131 个唯一原文。
- `Legacy_Markdown_Manifest.csv` 保留每个来源、哈希、文件大小、重复数和规范归档路径。
- 旧 Prompt、Report、Review、技能运行链路和 Bug 修复经验仍可追溯，但不会污染当前入口。

## 2. 已综合保存的技术思想

### 项目与协作

- 顶层路线和局部阶段分开。
- 当前状态、下一步和待确认事项只有一个活跃入口。
- 代码前审阅类名/成员/函数/Tag，代码后由 Codex 审查，用户再做 UE 配置和 PIE。
- 文档、编译、编辑器资产和多人运行分别验证。
- MCP 只处理适合批量化的工作。

### GAS 和技能

- PlayerState Owner / Pawn Avatar。
- 可撤销 AbilitySet。
- InputAction -> InputTag -> AbilitySpec。
- GA 公共基类、流程模板、AbilityTask、虚钩子、专用 GA 扩展阶梯。
- 一个根 SkillDefinition + 一个受约束的类型化 ExecutionConfig。
- 独立 Skill Policy/Relationship 层。
- Native Tag 与 Config Tag 的使用边界。
- Shared GA 与 GAS AssetTags/Spec-aware Policy 的冲突经验。
- TargetData、PredictionKey、Authority 和远端表现验证。

### 战斗与数值

- Cast Target、Spawn Rule、Detection Rule 分层。
- Projectile/Field/Trap/Summon 等 CombatEntity 独立生命周期。
- AttributeSet、GE、Execution、Meta Attribute、Health Component/死亡流程分层。
- State/Effect/Buff 的长期概念及“不提前重复 GAS”的限制。
- SetByCaller 仅是动态数值键，不是角色属性或技能身份。

### 动画和表现

- AnimInstance 数据、StateMachine、BlendSpace、Montage、Notify 各自职责。
- GameplayEvent Notify 驱动关键 GA 时机，普通 Notify 驱动纯表现。
- GameplayCue 只负责表现。
- 第一/第三人称和武器动作族复用、重定向、Socket、IK、AimOffset 的边界。

### 技能编辑器

- 类 Montage 的 Details + Viewport + Timeline。
- 保存配置资产，不生成每技能 C++。
- 尽量复用运行时定义，不建立独立预览逻辑。
- 不能替代真正网络、权威和 PIE。

### 工程经验

- Soft Reference 加载判断。
- AbilitySpec/EffectSpec/TargetData/HitResult 有效性。
- Ability Level 与 SourceObject。
- Authority/Prediction 成对。
- AbilityTask、Delegate、Timer、Montage、GameplayEvent 的清理。
- 生成、可见、移动、命中、伤害、Cue、复制必须分层验证。

## 3. 明确没有迁移的内容

- Aura/Apex 的业务 C++ 代码。
- 旧蓝图、地图和 Gameplay 资产。
- 旧项目当前阶段状态。
- 旧类名作为 Apecox 的既定命名。
- 大体积 MHTML、图片和附件。

这不是技术思想丢失：

- 业务代码中可复用的经验已由风险清单、运行链路和综合基线覆盖。
- 旧 Markdown 原文已全量去重归档。
- MHTML 的本机路径、大小、SHA256 和对应摘要已进入外部资料索引。

## 4. 旧结论的状态分类

### 继续采用

- 职责分层、受控扩展、GAS 原生生命周期、多人验证和学习型文档。

### 作为候选重新审阅

- 具体类名。
- SkillDefinition/WeaponDefinition 字段。
- Spec-aware Policy 的实现方式。
- Skill Timeline。
- State/Effect/Buff 的项目级资产。
- 第一/第三人称双 Mesh 架构。

### 明确不直接迁移

- 万能 GA。
- 万能配置表。
- 任意 Step 解释器。
- 完整 SkillGraph/IR/VM。
- Tag 单例。
- Aura/Apex 旧业务代码。
- Lyra 完整 Experience/GameFeature/ModularGameplay。

## 5. 完整性保障

以后发生上下文压缩时，按以下顺序恢复：

1. `.agents/ue-project-context.md`
2. `Agent/README.md`
3. `Agent/00_Coordination/Current_Phase.md`
4. 当前 RFC 或 `Current_Code_Design.md`
5. `02_CombatFramework/Framework_Inheritance_Baseline.md`
6. 需要论证时再查精选参考和历史 Manifest

这样既能快速恢复当前状态，也保留所有旧技术原文的追溯能力。
