# ClaudeCode 修复 Prompt：Phase 1B 首次运行问题

更新日期：2026-08-06

这是一次严格限界的小修复。请先阅读：

1. `D:/UnrealProject/Apecox/Agent/Reviews/2026-08-06_Phase1B_Runtime_Input_ActorInfo_Review.md`
2. `D:/UnrealProject/Apecox/Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
3. `D:/UnrealProject/Apecox/Source/Apecox/Public/Input/ApecoxInputComponent.h`
4. `D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Report.md`

不要重构，不新增公开 API、类、枚举或 GameplayTag，不创建/修改 UE 资产，不使用 MCP，不执行 Git 操作。

## 1. 修复 PlayerState 默认 Avatar 的误判

修改 `AApecoxPlayerCharacter::InitializeAbilitySystem()`。

UE 5.8 的 `UAbilitySystemComponent::InitializeComponent()` 会默认执行 `InitAbilityActorInfo(Owner, Owner)`。本项目 ASC 由 PlayerState 持有，因此首次正式绑定 Character 前，`ExistingAvatar == ApecoxPS` 是合法的 GAS 引导状态。

要求：

- `ExistingAvatar == nullptr`：正常继续。
- `ExistingAvatar == this`：仍由已有幂等分支处理。
- `ExistingAvatar == ApecoxPS`：不 ensure、不调用 `SetAvatarActor(nullptr)`，直接继续到 `ASC->InitAbilityActorInfo(ApecoxPS, this)`。
- `ExistingAvatar` 是其他 `AApecoxPlayerCharacter`：沿用现有逻辑，让旧 Character 执行 `UninitializeAbilitySystem()`。
- 其他未知 Actor：保留 ensure 和防御性清理，不能静默接受。
- 不改变 OwnerActor 必须是 `ApecoxPS`、AvatarActor 必须是当前 Character 的最终状态。

请在代码旁加入简短中文注释，说明“PlayerState/PlayerState 是 ASC InitializeComponent 的合法默认过渡状态”，避免以后再次误判。

## 2. 将 Ability 输入改为中性的物理边沿

修改 `UApecoxInputComponent::BindAbilityActions()`：

- Pressed 回调由 `ETriggerEvent::Triggered` 改为 `ETriggerEvent::Started`。
- Released 回调继续使用 `ETriggerEvent::Completed + ETriggerEvent::Canceled`。
- 同步修正头文件注释，不再写 `Pressed = Triggered（one-shot）`。
- 注释明确：Ability IA 与对应 IMC Mapping 的 Trigger 都应为空；IA 只采集物理按下/松开，不定义瞬发、按住、蓄力或松发等玩法。

设计理由：本项目允许同一个技能在不同角色配置下采用不同触发方式，因此技能玩法不能固化在 IA。Trigger 为空时，`Started` 精确表示 Digital Action 从未激活到开始激活的物理边沿；`AbilityInputTagPressed()` 只调用一次，但它会把 Spec 同时放入 Pressed 与 Held，直到 Completed/Canceled 才移出 Held。

不要在本轮新增 `Hold`、`ReleaseToActivate` 等枚举或配置字段。当前只建立中性的输入底座；同一个 GA 类如何读取每次授予/Spec 级触发策略，要在后续技能配置设计中单独讨论。

## 3. 更新原实施报告

覆盖更新 `D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Report.md`，追加“首次运行修复”小节，记录：

- PlayerState 默认 Avatar 为什么合法、代码如何处理。
- 瞬时 `Pressed` Trigger 为什么会导致立即 `Completed`。
- 正确资产要求是 `IA_Ability_Tactical` 与对应 IMC Mapping 的 Triggers 均为空，C++ 使用 `Started + Completed/Canceled` 采集物理边沿。
- AbilitySet 资产实例按父类规则命名为 `DA_Phase1B_AbilitySet`。

## 4. 静态检查与编译

运行：

```powershell
git diff --check
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE -NoLink
```

若 `Build.bat` 因外层全局锁等待超过合理时间，停止等待并如实写入报告；不要启动 UE 编辑器，不要自行使用其他引擎版本。

完成后停止，向用户汇报修改文件、关键条件分支、检查结果，等待 Codex 复审。
