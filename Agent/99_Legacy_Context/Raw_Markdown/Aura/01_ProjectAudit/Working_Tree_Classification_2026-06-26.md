# 当前工作区分类 - 2026-06-26

## 总结

当前 dirty 工作区主要包含四类内容：

- 新增的 Agent 协作/上下文文档。
- UE 5.4 升级到 UE 5.8 的项目配置改动。
- MCP Automation Bridge 插件和相关项目配置。
- 本机生成的 IDE/agent 工具文件，不建议提交。

建议不要做一个巨大的 `update everything` 提交，而是拆成两到四个小提交。这样未来如果某一类改动出问题，可以更容易回滚。

## 提交组 A - Agent 协作文档

**建议：提交。**

路径：
- `.agents/ue-project-context.md`
- `Agent/README.md`
- `Agent/00_Coordination/WorkingAgreement.md`
- `Agent/00_Coordination/Task_Backlog.md`
- `Agent/00_Coordination/Decision_Log.md`
- `Agent/00_Coordination/Git_Workflow.md`
- `Agent/00_Coordination/Skill_MCP_Catalog.md`
- `Agent/01_ProjectAudit/Aura_Current_State.md`
- `Agent/01_ProjectAudit/Working_Tree_Classification_2026-06-26.md`
- `Agent/04_AgentHandoff/**/README.md`

建议提交信息：

```text
docs: establish agent coordination workspace
```

说明：
- 这些文件是我们之后协作的基础设施。
- 不影响 UE 编译或运行时行为。

## 提交组 B - 已有 SkillGraph 参考文档

**建议：提交，但标记为未审查参考。**

路径：
- `Agent/UE_GAS_SkillGraph_Design_Document.md`

建议提交信息：

```text
docs: add existing GAS skill graph reference
```

说明：
- 这个文件在新目录结构建立前已经存在。
- 目前只作为灵感来源，不是最终方案。
- 后续可以移动或复制到 `Agent/90_References/External_Design_Reviews/`，并单独写评审。

## 提交组 C - UE 5.8 项目升级

**建议：提交。用户已说明插件变化不重要，后续需要可自行补充。**

路径：
- `Aura.uproject`
- `Source/Aura.Target.cs`
- `Source/AuraEditor.Target.cs`
- `.vsconfig`

观察到的改动：
- `EngineAssociation` 从 `5.4` 改为 `5.8`。
- Target 构建设置从 `BuildSettingsVersion.V5` 改为 `BuildSettingsVersion.V7`。
- Include order 从 `Unreal5_4` 改为 `Unreal5_8`。
- `.vsconfig` 更新了 Visual Studio C++ 组件、Unreal IDE/Debugger、LLVM/Clang、Windows 11 SDK。
- `FlatNodes` 插件从 `Aura.uproject` 中移除。
- `PCG` 插件加入。
- `MotionWarping` 保持启用。

建议提交信息：

```text
chore: upgrade project settings to UE 5.8
```

说明：
- 这是一个清晰的升级提交。
- `FlatNodes` 被移除不影响当前任务；用户已确认后续有需要可自行补充。

## 提交组 D - MCP Automation Bridge

**建议：提交。**

路径：
- `Plugins/McpAutomationBridge/`
- `Config/DefaultGame.ini`

观察到的改动：
- `Config/DefaultGame.ini` 增加了 MCP 插件设置：
  - `bEnableNativeMCP=True`
  - `bAlwaysListen=False`
  - `bMultiListen=False`
  - `NativeMCPPort=3000`
- 插件源码位于 `Plugins/McpAutomationBridge`。
- 插件的 `Binaries/` 和 `Intermediate/` 已被当前 `.gitignore` 忽略。
- 未被忽略的插件源码/配置/文档约 145 个文件。
- 插件描述中标记为 beta/experimental，许可证为 MIT。

建议提交信息：

```text
chore: add MCP automation bridge plugin
```

说明：
- 提交插件源码可以让 MCP 自动化环境可复现。
- 不提交插件的 `Binaries/` 或 `Intermediate/`。
- 因为这是第三方工具，建议独立提交。

## 低价值格式改动

**建议：可以单独做一个很小的格式提交，也可以暂时保留到后续整理。**

路径：
- `Source/Aura/Private/AbilitySystem/Abilities/AuraProjectileSpell.cpp`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemLibrary.cpp`

观察到的改动：
- 每个文件只删除了一行空行。

建议提交信息：

```text
style: remove stray blank lines
```

说明：
- 改动无害，但没有业务意义。
- 不建议混到功能提交里。

## 不建议提交 / 建议忽略

**建议：不要提交。后续如果用户同意，再加 `.gitignore`。**

路径：
- `.claude/settings.local.json`
- `Aura.slnx`
- `Automation_Aura.slnx`

原因：
- `.claude/settings.local.json` 是本地 Claude 权限配置，属于用户/机器环境。
- `Aura.slnx` 和 `Automation_Aura.slnx` 包含 `E:/UE_5.8/...` 这类本机绝对路径。
- 它们更像生成文件或本机 IDE 状态。

建议后续 `.gitignore` 增加：

```text
.claude/
*.slnx
```

## 当前 Git 警告

`git status` 会出现：

```text
warning: unable to access 'C:\Users\Lenovo/.config/git/ignore': Permission denied
```

这不是项目文件导致的。它更像是 Git 在读取用户级全局 ignore 文件时没有权限。它不阻止本地 status/diff，但以后可以修一下，让 Git 输出更干净。

## 建议提交顺序

1. `docs: establish agent coordination workspace`
2. `docs: add existing GAS skill graph reference`
3. `chore: upgrade project settings to UE 5.8`
4. `chore: add MCP automation bridge plugin`
5. 可选：`style: remove stray blank lines`

## 已确认事项

- 用户确认 `FlatNodes` 插件不重要；后续有需要可自行补充。
- 用户授权 Codex 按建议进行提交。

## 仍待确认

- `.slnx` 是否长期作为本机生成文件处理。
- `.claude/` 是否保持纯本地，还是未来需要一份共享 Claude 配置模板。
- MCP 插件长期是直接 vendored 到仓库，还是后续改成其他管理方式。
