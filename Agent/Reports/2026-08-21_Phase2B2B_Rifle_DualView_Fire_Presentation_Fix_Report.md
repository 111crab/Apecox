# Phase 2B-2B 修复报告：Montage 播放边界

修复日期：2026-08-21  
构建结果：**通过**（0 错误，0 新增警告）  
状态：等待 Codex 复审。

## 1. 实际修改（是否超出范围）

只修改一个文件，未超出 Prompt「修改边界」：

| 文件 | 改动 |
| --- | --- |
| `Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp` | 重写 `PlayMontageOnMesh()` 函数体 |
| `Agent/Reports/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Fix_Report.md` | 新增本报告 |

未修改头文件、`AApecoxPlayerCharacter`、`Apecox.Build.cs`、Hitscan GA、TargetData、GameplayCue、复制逻辑或任何 `.uasset`/`.umap`；未调用 MCP、未生成解决方案、未执行 Git。

## 2. 修复内容

`PlayMontageOnMesh()` 签名与 4 个调用点保持不变：

```cpp
bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage) const;
```

修复后的判定顺序：

1. `MeshComponent` 或 `Montage` 为空 → 返回 `false`。
2. `GetAnimInstance()` 非空且 `GetSingleNodeInstance()` 为空（即普通 AnimBP 实例）→ 调用 `Montage_Play()`，仅当返回值 `> 0.0f` 返回 `true`，否则 `false`。
3. 无普通 AnimInstance 时，显式比较 `MeshComponent == FirstPersonWeaponMeshComponent` 或 `== ThirdPersonWeaponMeshComponent`，只有命中才允许 `PlayAnimation(Montage, false)`（Single Node 回退）。
4. 否则（FP Arms / TP Character 缺 AnimInstance）→ 输出一条 `Verbose` 日志并返回 `false`，不调用 `PlayAnimation()`，不改变其 Animation Mode。

修复前的缺陷：只依据 `AnimInstance` / `GetSingleNodeInstance()` 判断，任何无 AnimInstance 的 Mesh 都会回退 `PlayAnimation()`，可能把未配置 AnimBP 的 Character/Arms 误切成 Single Node；且 AnimInstance 路径未检查 `Montage_Play()` 返回值。

## 3. 四种输入情形的行为

| 输入情形 | 判定路径 | 行为 | 返回值 |
| --- | --- | --- | --- |
| Character/Arms 有 AnimBP 实例 | `bHasAnimBPInstance == true` | `Montage_Play()` | 时长 `> 0` 时 `true`，播放失败 `false` |
| Character/Arms 无 AnimBP 实例 | 非 Weapon Mesh，无普通 AnimInstance | 输出 Verbose 日志，不改变 Animation Mode | `false` |
| Weapon Mesh 无 AnimInstance | `bIsWeaponMesh == true` | `PlayAnimation(Montage, false)`（Single Node） | `true` |
| 空输入（Mesh 或 Montage 为 null） | 首行空检查 | 直接返回 | `false` |

补充说明：Weapon Mesh 若已有 Single Node 实例（此前已回退过），`GetSingleNodeInstance()` 非空使 `bHasAnimBPInstance` 为 false，仍命中 Weapon Mesh 路径继续 `PlayAnimation()`。

## 4. 构建结果

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 7.46 seconds（增量构建，仅 ApecoxEquipmentComponent.cpp 重编）
错误：0
新增警告：0
```

既有警告 1 个（与本次无关）：`ApecoxAbilitySystemComponent.cpp:228` 的 `NonInstanced` 弃用警告，本轮未触碰该文件。

## 5. 未修改范围

- 头文件、Character、Build.cs：未改。
- `UApecoxHitscanFireAbility`、TargetData、PredictionKey、Trace、扣弹、伤害、Shot Confirmation：未改。
- GameplayCue 路径与复制逻辑：未改。
- `PlayFirePresentation()`、`PlayMuzzlePresentation()`：未改。
- 任何 `.uasset`/`.umap`：未创建、未修改。

---

*构建通过后停止，等待 Codex 复审。*
