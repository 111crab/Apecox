# Git 工作流建议

*最后更新：2026-07-07*

## 当前观察

当前仓库在 `main` 分支，跟踪 `origin/main`，并且工作区已有未提交改动。所以第一步不是马上清理，而是先把改动分组，避免不同来源的变化混成一个大提交。

初次扫描看到的改动包括：
- `.vsconfig`
- `Aura.uproject`
- `Config/DefaultGame.ini`
- `Source/Aura.Target.cs`
- `Source/AuraEditor.Target.cs`
- 若干 `Source/Aura/...` 文件
- 新增 `.claude/`、`Agent/`、`.slnx` 文件、`Plugins/`

## 原则

- 尽量保持 `main` 稳定。
- 对明确任务使用命名分支。
- 文档、代码、资产/配置尽量分开提交。
- 不覆盖另一个 agent 或用户的改动。
- 不在未确认的情况下 revert dirty 文件。
- 每次实施前先记录当前分支和 dirty 状态。

## 建议分支命名

示例：
- `codex/phase-0-coordination`
- `codex/skill-architecture`
- `claude/runtime-prototype-targeting`
- `claude/runtime-prototype-effects`
- `claude/editor-tooling-v1`

后续如果你想要更简单的命名规则，可以再改。

## 提交信息风格

建议前缀：
- `docs:` 只改文档
- `audit:` 项目审计或报告
- `gas:` GAS 运行时代码
- `editor:` 编辑器工具
- `ui:` UI/HUD
- `assets:` 资产组织或资产引用
- `fix:` bug 修复
- `test:` 测试或验证代码
- `chore:` 配置、升级、工具类改动

示例：
- `docs: establish agent coordination workspace`
- `audit: summarize current Aura GAS baseline`
- `gas: add skill definition data asset`

## 版本与 Tag 规则

`commit` 用来保存每一批有意义的修改；`tag` 用来给稳定基线或阶段成果起一个可长期引用的名字。

建议使用带说明的 annotated tag：

```bash
git tag -a tutorial-baseline-before-v1-skill-runtime -m "Tutorial baseline before V1 skill runtime implementation"
git push origin tutorial-baseline-before-v1-skill-runtime
```

当前已创建并推送：
- `tutorial-baseline-before-v1-skill-runtime`：V1 技能运行时代码改写前的教程基线。

后续建议：
- V1-A 全部 6 个技能完成并通过验证后，再创建类似 `v1-a-skill-runtime` 的阶段 tag。
- 如果以后要发布 GitHub Releases，可以基于 tag 创建 Release，并补充中文/英文 release notes。

## 单个任务建议流程

1. 查看状态：`git status --short --branch`。
2. 确认分支，必要时创建任务分支。
3. 做范围明确的改动。
4. 运行合适的验证。
5. 写或更新 agent 报告。
6. 审查 diff。
7. 在用户批准或流程允许时提交。
8. 稳定节点再 push。

## UE 项目特殊注意

- `.uasset` 等二进制资产很难合并，避免多人同时改同一个资产。
- 大规模资产操作前，先明确会影响哪些路径。
- 如果 UE 编辑器/MCP 操作改动了许多资产或配置，报告里必须列出路径。
- 如果项目后续有大量大文件资产，应该考虑 Git LFS；当前尚未验证。

## 当前第一步建议

当前已完成一次工作区分类，报告见：

`Agent/01_ProjectAudit/Working_Tree_Classification_2026-06-26.md`

后续提交应按报告分组进行。
