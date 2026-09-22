# Phase 2B-2B 步枪双视角腰射表现代码审查

审查日期：2026-08-21  
结论：**暂不通过，需完成 1 项局部修复。**

## 审查发现

### P2：Single Node 回退没有真正限制为两个 Weapon Mesh

位置：

```text
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp:361
```

`PlayMontageOnMesh()` 当前只根据 `AnimInstance` / `GetSingleNodeInstance()` 判断播放路径。只要任意传入 Mesh 没有 AnimInstance，就会执行：

```cpp
MeshComponent->PlayAnimation(Montage, false);
```

这意味着 FP Arms 或 TP Character 在 AnimBP 尚未配置、加载异常或被误设时，也会被切换为 Single Node，违反本轮已经批准的关键边界：Character/Arms 只能通过现有 AnimInstance 播放，Single Node 只允许用于 `FirstPersonWeaponMeshComponent` 和 `ThirdPersonWeaponMeshComponent`。

此外，AnimInstance 路径没有检查 `Montage_Play()` 的返回值；Slot 不匹配、Skeleton 不兼容等导致播放失败时，函数仍会返回 `true`，不利于后续 UE 配置排错。

## 修复要求

- 保持 `PlayMontageOnMesh()` 的公开签名、调用位置和职责不变。
- 有普通 AnimInstance 时使用 `Montage_Play()`，并根据返回值判断成功。
- 只有传入 Mesh 明确等于两个动态 Weapon Mesh Component 之一时，才允许回退到 `PlayAnimation()`。
- Character/Arms 没有可用 AnimInstance 时返回 `false`，不得改变 Animation Mode。
- 不修改 Hitscan GA、GameplayCue 路径、复制、资产或其他功能。

## 已通过部分

- 修改范围符合 Prompt，没有改动 `.uasset`、`.umap`、Hitscan GA 或网络真相层。
- `UApecoxWeaponPresentationDefinition` 的 7 个新增字段与批准设计一致。
- `PlayFirePresentation()` 的 Dedicated Server 早退及 `IsLocallyControlled()` FP/TP 分流正确。
- Niagara/SFX 只作为表现，Socket 缺失时安全跳过。
- PlayerCharacter 已按批准层级调整为 Camera 挂 Capsule、FP Arms 挂 Camera。
- `Niagara` 模块依赖最小且现有 Development Editor 构建通过。
- `git diff --check` 未发现空白错误。

## 公开 API 审阅

本轮唯一新增 Blueprint 入口：

```cpp
UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment|Presentation")
void PlayFirePresentation();
```

命名和职责符合已批准方案，无需用户重新确认。

## 下一步

执行局部修复 Prompt，Codex 复审通过后，再更新 UE 人工配置清单并开始资产与 GameplayCue 配置。
