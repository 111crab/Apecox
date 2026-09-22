# Claude Code 修复 Prompt：Phase 2B-2B Montage 播放边界

你是 Apecox 项目的具体实施子代理。Codex 已完成首轮代码审查；本轮只修复一个明确问题，不扩展功能。

项目根目录：

```text
D:/UnrealProject/Apecox
```

先阅读：

```text
Agent/Reviews/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Review.md
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
```

## 唯一修复目标

修复 `UApecoxEquipmentComponent::PlayMontageOnMesh()`：当前实现会让任何没有普通 AnimInstance 的 Mesh 回退到 `USkeletalMeshComponent::PlayAnimation()`，没有把 Single Node 严格限制在两个动态 Weapon Mesh Component。

要求：

1. 保持函数签名与四个现有调用点不变：

```cpp
bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent,
    UAnimMontage* Montage) const;
```

2. `MeshComponent` 或 `Montage` 为空时返回 `false`。
3. Mesh 拥有普通 Animation Blueprint `UAnimInstance` 时，调用 `Montage_Play()`；只有返回值大于 `0.0f` 才返回 `true`，播放失败返回 `false`。
4. Single Node 回退必须显式验证：

```text
MeshComponent == FirstPersonWeaponMeshComponent
或
MeshComponent == ThirdPersonWeaponMeshComponent
```

只有以上两个 Weapon Mesh 才允许调用：

```cpp
MeshComponent->PlayAnimation(Montage, false);
```

5. FP Arms 或 TP Character 没有普通 AnimInstance 时直接返回 `false`，不得调用 `PlayAnimation()`，不得改变它们的 Animation Mode。
6. 可加入一条有帮助但不过量的 `Verbose` 日志说明 Character/Arms 因缺少 AnimInstance 而跳过；不要每 Tick 输出，本函数只在 Fire Cue 时调用。
7. 保留中文设计注释，并让注释与真实代码一致。

## 修改边界

只允许修改：

```text
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Agent/Reports/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Fix_Report.md
```

不得修改头文件、Character、Build.cs、Hitscan GA、TargetData、GameplayCue、复制逻辑或任何 `.uasset/.umap`；不得调用 MCP、生成解决方案或执行 Git。

## 验证与报告

执行：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

新增中文报告：

```text
Agent/Reports/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Fix_Report.md
```

报告说明实际修改、四种输入情形的行为（Character/Arms 有 AnimBP、Character/Arms 无 AnimBP、Weapon 无 AnimBP、空输入）、构建结果及是否超出范围。完成后停止，等待 Codex 复审。
