# ClaudeCode 修复 Prompt：Phase 1A 玩家与 ASC 生命周期

你已经完成 Phase 1A 首轮实现。现在只修复 Codex 审查列出的具体问题，不开始 Phase 1B。

## 必读

1. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md`
2. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Phase.md`
3. `D:/UnrealProject/Apecox/Agent/Reviews/2026-08-06_Phase1A_Player_ASC_Lifecycle_Review.md`
4. 首轮源码和首轮实施报告。

保留 Codex 当前 Markdown 改动，不得还原。先执行 `git status`，确认只在下列允许范围内修改。

## 允许修改

- `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
- `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
- `Source/Apecox/Private/Player/ApecoxPlayerState.cpp`
- `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h`
- `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`
- 新建修复报告：`Agent/Reports/2026-08-06_Phase1A_Player_ASC_Lifecycle_Fix_Report.md`

若编译本身证明必须调整 Include，可在同一已有 Phase 1A 文件内做最小修正并在报告说明。不得创建新类、Tag、资产或配置。

## 修复 1：新 Pawn 绑定前清退已有旧 Avatar

在 `AApecoxPlayerCharacter::InitializeAbilitySystem()` 中，取得目标 ASC 后：

1. 保留“同一 ASC 且 Avatar 已是 this”的幂等返回。
2. 处理当前 Character 自己缓存了另一个 ASC 的情况，复用 `UninitializeAbilitySystem()`，不要复制一遍清理代码。
3. 在绑定目标 ASC 之前读取 `ASC->GetAvatarActor()`。
4. 如果 ExistingAvatar 非空且不是 this：
   - 若它是 `AApecoxPlayerCharacter`，调用旧 Character 的 `UninitializeAbilitySystem()`，让旧 Character 清除自己的缓存并取消旧 Avatar Ability。
   - 若不是 Apecox PlayerCharacter，使用 `ensureMsgf` 暴露违反当前 GameMode/Pawn 约束的异常；做最小安全清理，保留有效 OwnerActor，只解除 Avatar。
5. 确认旧 Avatar 已处理后，再调用 `InitAbilityActorInfo(ApecoxPS, this)`。

这处理客户端“新 Pawn 先到、旧 Pawn 后清理”的复制乱序，不能只依赖旧 Pawn 之后执行 `UnPossessed/EndPlay`。

## 修复 2：正常解绑保留 PlayerState OwnerActor

修改 `UninitializeAbilitySystem()`：

```text
if CachedASC 的 AvatarActor == this:
    CancelAllAbilities()
    if OwnerActor 有效:
        SetAvatarActor(nullptr)
    else:
        ClearActorInfo()
最后无条件清空本 Character 的缓存
```

- 不调用 `RemoveAllGameplayCues()`。
- 不在 OwnerActor 有效的正常换 Pawn路径调用 `ClearActorInfo()`。
- 旧 Character 延迟清理时，若 ASC 已绑定新 Avatar，只清本 Character 缓存，不能影响新 Avatar。

## 修复 3：收紧生命周期 API

- `GetAbilitySystemComponent()` 和 `GetApecoxAbilitySystemComponent()` 保持 public。
- `InitializeAbilitySystem()`、`UninitializeAbilitySystem()` 移入 protected。
- `PossessedBy`、`OnRep_PlayerState`、`UnPossessed`、`EndPlay` 继续位于 protected。
- 不增加 BlueprintCallable 或其他反射暴露。

同一个 `AApecoxPlayerCharacter` 类的成员函数可以调用另一个该类实例的 protected 生命周期函数，用于旧 Avatar 清退。

## 修复 4：PlayerState 网络更新频率

在 `AApecoxPlayerState` 构造函数中加入：

```cpp
SetNetUpdateFrequency(100.0f);
```

添加简短中文注释：`APlayerState` 默认 1 Hz 不适合承载玩家 ASC/Attribute；100 Hz 是当前小规模多人原型基线，不是玩法属性。

不要增加自定义复制、RPC 或新成员。

## 修复 5：Vital Attribute 不变量与初始化职责

1. 移除 `InitHealth(100.0f)` 和 `InitMaxHealth(100.0f)`。
2. 如果构造函数移除后不再需要，删除其声明和定义；不要用另一个硬编码值替代。
3. 增加：

```cpp
virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
```

4. `PreAttributeBaseChange` 和 `PreAttributeChange` 都调用 `ClampAttribute`：
   - MaxHealth 不小于 0。
   - Health 限制在 `[0, MaxHealth]`。
5. `PostAttributeChange` 在 MaxHealth 变化且 Health 大于 NewValue 时，通过 Owning ASC 的标准属性修改入口把 Health 压回 NewValue；不要直接写 `FGameplayAttributeData` 内部字段。
6. `PostGameplayEffectExecute` 的 MaxHealth 分支除了钳制 MaxHealth，还必须再次保证 Health 不超过 MaxHealth。
7. 不加入死亡、事件、委托、Shield、Damage Meta Attribute 或出生数值。

## 报告准确性

不要修改首轮报告。新建：

`D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1A_Player_ASC_Lifecycle_Fix_Report.md`

报告必须说明：

- 五项修复逐项如何完成。
- 实际修改文件、成员和函数可见性变化。
- 本批确实新增的 Attribute 是 `Health/MaxHealth`；没有新增 Tag、GA、GE、Cue、DataAsset、AbilityTask 或资产。
- 原 Prompt 的 `ClearActorInfo()` 正常路径要求已被审查纠正为保留 PlayerState OwnerActor。
- 完整编译结果和未验证项。

## 编译

当前检测到 Unreal Editor 仍在运行。不要强杀进程，也不要反复执行注定因 DLL 文件锁失败的链接。

- 如果执行时编辑器已经关闭，运行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

- 如果编辑器仍在运行，完成静态检查并在报告中明确写“未执行完整编译：编辑器持有 DLL 文件锁”，等待用户关闭后由 Codex/用户复编。

完成后停止。不要操作 UE 资产，不要生成 IDE 文件，不要提交 Git，不要继续 Phase 1B。

