# Apecox Git 工作流

更新日期：2026-08-05

## 项目原则

- 以完成的功能或小阶段为提交单位，不为无关细节消耗主要开发时间。
- 提交前看一次状态、检查暂存差异；不逐文件进行过度审计。
- 不覆盖或回退用户、UE 编辑器、其他代理产生的改动。
- 稳定里程碑使用 annotated tag；普通进展使用 commit。

## 推荐跟踪范围

- 跟踪：`Source`、`Config`、`Agent`、`.agents`、`Apecox.uproject`、项目自建插件源码。
- 跟踪：项目自建的 Blueprint、Map、动画派生资产、材质实例等必要 `.uasset/.umap`。
- 排除：`Binaries`、`DerivedDataCache`、`Intermediate`、`Saved`、`.vs`、Rider 缓存和 Automation 解决方案。
- 第三方 Marketplace/Fab 内容默认不提交到公开 GitHub；保存来源、版本、导入步骤和依赖清单。

## 二进制资产

建议对项目自建 `.uasset/.umap` 使用 Git LFS，因为它们不可文本合并。启用前先确认 Git LFS 已安装，并确认 GitHub 存储额度。

如果暂不使用 LFS，也必须避免提交整包第三方资产和单文件超过 GitHub 限制的内容。

## 提交流程

1. `git status --short --branch`
2. 按当前功能暂存明确路径。
3. `git diff --cached --stat`
4. 编译/PIE 已完成时提交。
5. 小阶段结束或需要跨机器备份时 push。

## 建议提交前缀

- `docs:` 文档和架构。
- `chore:` 工程、插件和配置。
- `framework:` Gameplay Framework。
- `gas:` ASC、Attribute、GA、GE、Tag。
- `weapon:` 武器、弹药、命中和装备。
- `ability:` 英雄技能。
- `net:` 复制、RPC 和预测。
- `ui:` HUD 与 CommonUI/UMG。
- `fix:` 缺陷修复。

## 里程碑 Tag 候选

- `apecox-initial-baseline`
- `apecox-gas-foundation-v1`
- `apecox-rifle-vertical-slice-v1`
- `apecox-multiplayer-combat-v1`
