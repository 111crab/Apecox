# Apex Git 工作流建议

生成日期：2026-07-14
来源：继承 Aura 阶段经验，并按 Apex 新项目调整。

## 原则

- 每次实施前先查看 `git status --short --branch`。
- 不覆盖用户、UE 编辑器或其他 agent 产生的改动。
- 文档、C++、配置、UE 资产尽量分开提交。
- `.uasset` 等二进制资产很难合并，避免多人同时改同一个资产。
- 大规模资产迁移前，先列清单，再执行。
- 稳定基线用 Git tag 标记；普通阶段工作用 commit 保存。

## 分支建议

当前 Apex 观察到的分支是 `master`。如后续需要任务分支，建议使用：

| 场景 | 示例 |
| --- | --- |
| Codex 文档/规划 | `codex/apex-agent-docs` |
| Claude 代码实施 | `claude/character-cold-start` |
| GAS 基础 | `codex/gas-foundation-rfc` |
| 资产验证 | `codex/paragon-asset-audit` |

## 提交信息建议

| 前缀 | 用途 |
| --- | --- |
| `docs:` | Agent 文档、报告、Prompt |
| `audit:` | 项目审计、资产盘点、状态分类 |
| `character:` | 角色、相机、移动、动画基础 |
| `gas:` | ASC、GA、GE、AttributeSet、Tag |
| `skill:` | 技能运行时、技能模板、战斗衍生物 |
| `editor:` | 技能编辑器、Editor 工具 |
| `assets:` | 资产迁移、资产引用、UE 内容调整 |
| `fix:` | bug 修复 |
| `chore:` | 工程配置、依赖、杂项 |

## Tag 建议

- `apex-initial-baseline`：干净项目初始基线。
- `apex-character-baseline`：玩家角色基础移动和第三人称视角可验证后。
- `apex-gas-foundation-v1`：ASC、Attribute、输入激活最小闭环后。
- `apex-skill-runtime-v1`：第一批技能运行时核心稳定后。

使用 annotated tag：

```bash
git tag -a apex-character-baseline -m "Apex character movement and camera baseline"
git push origin apex-character-baseline
```

## 单任务流程

1. 查看状态。
2. 确认任务范围。
3. 写 Prompt 或计划。
4. 执行改动。
5. 编译或 PIE 验证。
6. 写报告。
7. Codex 审查。
8. 用户确认后再提交或推送。
