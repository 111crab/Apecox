# Aura Agent 上下文迁移报告

生成日期：2026-07-14

## 目标

把 Aura 项目 Agent/ 中对 Apex 仍有价值的协作流程、上下文、技能系统经验和历史材料迁移到 Apex，减少后续反复提醒和上下文丢失。

## 执行结果

- 已完整复制 Aura Agent/ 原始文件到：D:\UnrealProject\Apex\Agent\Aura_Legacy_Archive\
- 归档文件数量：73
- 归档总大小：37.04 MB
- 包含 Markdown、旧 Prompt、旧报告、旧审查、外部文章 mhtml、旧迁移包等原文。

## 新增 / 更新的 Apex active 文档

| 文件 | 用途 |
| --- | --- |
| Agent/README.md | Apex Agent 目录说明和新对话读取顺序。 |
| Agent/00_Coordination/Git_Workflow.md | 从 Aura 继承并适配 Apex 的 Git 工作流。 |
| Agent/00_Coordination/Pending_Questions.md | 当前待确认问题集中记录。 |
| Agent/00_Coordination/Session_Summary.md | 当前阶段摘要，用于上下文恢复。 |
| Agent/00_Coordination/Skill_MCP_Catalog.md | Skill/MCP 目录入口，指向 Tooling_Guide.md。 |
| Agent/01_ProjectAudit/README.md | 项目审计区占位说明，保证目录可被 Git 记录。 |
| Agent/04_AgentHandoff/README.md | 交接区说明，兼容 Aura 旧目录习惯。 |
| Agent/02_SkillSystem/README.md | Apex 技能系统设计区说明。 |
| Agent/02_SkillSystem/Apex_Inherited_Skill_Context.md | 从 Aura 继承到 Apex 的核心技能系统口径。 |
| Agent/02_SkillSystem/Combat_Foundation_RFC.md | Aura 阶段 Tag/属性/伤害/命中链路 RFC 原文拷贝。 |
| Agent/02_SkillSystem/Skill_Runtime_Risk_Checklist.md | Aura 阶段 GAS 异步/网络风险清单原文拷贝。 |
| Agent/02_SkillSystem/Montage_GameplayEvent_Timing_Decision.md | Montage Notify / GameplayEvent 决策原文拷贝。 |
| Agent/03_SkillEditor/README.md | 技能编辑器设计区说明。 |
| Agent/03_SkillEditor/Skill_Editor_Vision.md | Aura 阶段技能编辑器愿景原文拷贝。 |
| Agent/90_References/External_Reference_Index.md | 外部参考索引和使用规则。 |
| Agent/90_References/UE_GAS_SkillGraph_Design_Document.md | 旧参考方案原文拷贝。 |
| Agent/90_References/UE_GAS_SkillGraph_Design_Review.md | 旧参考方案评审拷贝。 |
| Agent/Reports/2026-07-14_Aura_Agent_Context_Migration_Report.md | 本报告。 |

## 合并到现有 active 文档的内容

| 文件 | 合并内容 |
| --- | --- |
| Agent/Working_Agreement.md | 增加“从 Aura 继承的硬规则”：先计划再实施、Prompt 范围、中文报告、学习型注释、直接新增 C++ 文件规则、命名审阅规则。 |
| Agent/Decision_Log.md | 增加“Aura Agent 上下文迁移为 Apex 协作基线”决策。 |
| Agent/Task_Backlog.md | 在 Phase 0 中标记 Aura Agent 上下文迁移已完成。 |

## 迁移判断

### 直接归档

旧 Prompt、旧 Claude 报告、旧 Codex 审查、旧 Aura 项目状态、旧火球/治疗运行链路等内容全部保留在 Aura_Legacy_Archive/，用于追溯经验，不作为 Apex 当前实现要求。

### 主动合并

协作规则、Prompt 规则、学习型注释、MCP 使用边界、命名审阅、技能系统分层口径、Tag 粒度原则、GameplayCue 边界、Montage/GameEvent 边界，被提炼进 Apex active 文档。

### 不直接套用

Aura 旧类名、旧目录、旧资产、旧 UAura* 代码路径、旧配置资产字段不直接迁移到 Apex。Apex 后续会重新命名并重新写 RFC。

## 当前仍需注意

- Agent/Aura_Legacy_Archive/ 中包含大体积 mhtml 外部文章。它们已迁移，但默认不主动读取。
- 当前 git 工作区中还存在 Content/ParagonPhase/、Content/Blueprints/、Source/Apex/Character/ 等未跟踪改动，可能来自用户或 ClaudeCode；本次迁移未修改这些内容。
- 后续提交前建议先做一次工作区分类，避免 Agent 文档、C++、UE 资产混在同一个 commit。

## 后续建议

1. 等 ClaudeCode 完成 Character 冷启动后，Codex 先审查 Source/Apex/Character/。
2. 用户在 UE 编辑器中人工验证 Phase 资产、角色蓝图和输入配置。
3. Character 基线稳定后，再继续 Paragon Montage 盘点。
4. GAS / 技能系统 RFC 开始前，优先读取 Agent/02_SkillSystem/Apex_Inherited_Skill_Context.md 和 Agent/02_SkillSystem/Skill_Runtime_Risk_Checklist.md。


