# ClaudeCode Prompt - Apex Ability 输入路由第二次修复

项目：`D:\UnrealProject\Apex`

## 目标

修复第一次复审剩余的一项 WhileInputActive 输入状态错误，并清理两项文档问题。不得扩大范围。

## 开始前读取

```text
Agent/Reviews/2026-07-23_Apex_Ability_Grant_Input_Routing_Review.md
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

先运行 `git status --short`，保留全部现有改动。

## 允许修改

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

## 修复 1：Pressed 必须处理 WhileInputActive Handle

在 `ProcessAbilityInput()` 的 Pressed 循环中，删除循环开头这段：

```cpp
if (AbilitiesToActivate.Contains(Handle))
{
	continue;
}
```

原因：

- WhileInputActive Handle 已在 Held 阶段加入 `AbilitiesToActivate`。
- Pressed 阶段仍必须设置 `Spec.InputPressed = true`。
- 如果 Spec 已激活，仍必须发送 `AbilitySpecInputPressed()`。
- 是否已在待激活数组中，不能成为跳过输入状态更新的理由。

后续 OnInputTriggered 分支继续使用 `AbilitiesToActivate.AddUnique(Handle)` 去重。除此之外不要改变当前处理顺序。

## 修复 2：注释拼写

在 `ApexAbilitySet.h` 把：

```text
TOjectPtr
```

改为：

```text
TObjectPtr
```

只改注释。

## 修复 3：同步实施报告现状

直接修正报告前半部分的旧描述，使其与最终代码一致：

- `AbilityInputTagPressed` 只入 Pressed/Held 队列，不立即激活。
- `ProcessAbilityInput` 完整处理 Held、Pressed、统一激活和 Released。
- 运行链路中 OnInputTriggered 在 `ProcessAbilityInput` 激活。
- RemoveCoreAbilitySet 包含 `ClearAbilityInput()`。
- AbilitySet/InputConfig Data Validation 包含 Tag 根域检查。
- 最终编译结论为修复版本的 0 error、0 warning。

保留“首次审查修复”章节作为历史，但报告中不得再存在互相矛盾的现状描述。

## 禁止事项

- 不修改其他源码、UE 资产、配置或 `.uproject`。
- 不新增类、函数、字段、Tag 或系统。
- 不使用 MCP。
- 不生成 Rider/Visual Studio 项目文件。
- 不执行 Git add、commit、push。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

要求 0 error、0 warning。把第二次修复和最终编译结果写入原实施报告，完成后停止等待 Codex 复审。
