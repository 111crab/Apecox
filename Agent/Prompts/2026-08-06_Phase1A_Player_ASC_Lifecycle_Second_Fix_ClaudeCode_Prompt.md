# ClaudeCode 二次精简修复 Prompt：Phase 1A

Phase 1A 的两个 P1 已正确修复。现在只处理一个剩余 P2 和编译检查，不进行其他重构。

## 必读

- `D:/UnrealProject/Apecox/Agent/Reviews/2026-08-06_Phase1A_Player_ASC_Lifecycle_Review.md` 最后的“修复后复审追加”。
- 当前 `ApecoxVitalAttributeSet.cpp`。

## 允许修改

- `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`
- 可选：把 `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` 第 16 行旧注释“清理自己的 ActorInfo”修正为“解除自己的 Avatar 关系”。只改注释，不改 API。
- 在现有 `Agent/Reports/2026-08-06_Phase1A_Player_ASC_Lifecycle_Fix_Report.md` 末尾追加“二次修复”小节。

## 唯一逻辑修复

当前 `PostGameplayEffectExecute` 的 MaxHealth 分支调用：

```cpp
SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f));
```

删除这次 Base Value 回写。原因：`GetMaxHealth()` 是包含持续 Modifier 的当前聚合值，把它再写回 Base Value 会在持续 MaxHealth Buff 存在时产生重复计算风险。

当前实现已经具备：

- `PreAttributeBaseChange` 钳制 MaxHealth Base；
- `PreAttributeChange` 钳制最终值；
- `PostAttributeChange` 在 MaxHealth 下降后压低 Health。

因此 `PostGameplayEffectExecute` 的 MaxHealth 分支不得再调用 `SetMaxHealth`。可以保留“若 Health 大于当前 MaxHealth，则压低 Health”的防御检查，但不要直接写 `FGameplayAttributeData`，也不要加入死亡、事件或新属性。

不要改动 Character 生命周期、PlayerState、Build.cs 或其他文件。

## 编译检查

Unreal Editor 当前仍在运行。不要结束进程。使用 UBT 的无链接模式检查 UHT 和 C++：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE -NoLink
```

如 `-NoLink` 未被 Build.bat 接受，原样报告，不要尝试 Hot Reload 或带后缀 DLL。完整链接将在用户关闭 UE 后完成。

更新现有修复报告，说明代码改动和 `-NoLink` 编译结果。完成后停止，不提交 Git，不操作 UE 资产，不继续 Phase 1B。
