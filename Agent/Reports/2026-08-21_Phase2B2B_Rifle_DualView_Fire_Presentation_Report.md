# Phase 2B-2B 步枪双视角腰射表现 实施报告

实施日期：2026-08-21  
构建结果：**通过**（0 错误，0 新增警告）  
状态：等待 Codex 审查。

## 1. 实际修改文件（是否超出 Prompt）

全部修改均落在 Prompt「六、修改边界」批准的文件清单内，未超出。

| 文件 | 改动类型 | 是否超出 |
| --- | --- | --- |
| `Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h` | 新增 7 个表现字段 + 前向声明 | 否 |
| `Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h` | 新增 1 个公开入口 + 2 个内部辅助 + 前向声明 | 否 |
| `Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp` | 实现 3 个函数 + 3 个 include | 否 |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 重构第一人称组件层级（仅构造函数） | 否 |
| `Source/Apecox/Apecox.Build.cs` | 新增 `Niagara` 私有依赖 | 否 |
| `Agent/Reports/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Report.md` | 新增本报告 | 否 |

未修改 `UApecoxHitscanFireAbility`、TargetData、WeaponStateComponent、射击 Tag、GA/GE/Attribute/AbilityTask、任何 `.uasset`/`.umap`，未执行 Git、未调用 UE MCP。

## 2. 修改的类名、父类与职责

| 类 | 父类 | 职责（本轮变更） |
| --- | --- | --- |
| `UApecoxWeaponPresentationDefinition` | `UDataAsset` | 从"只保存 Mesh/附着"扩展为"同时保存开火动画、枪口 Socket、Niagara、音效的只读表现配置" |
| `UApecoxEquipmentComponent` | `UActorComponent` | 新增开火表现分发入口，按本地控制关系把一次已成立射击翻译为 FP/TP 动画 + 枪口 VFX/SFX |
| `AApecoxPlayerCharacter` | `ACharacter` | 第一人称层级从"完整 Manny Copy Pose"改为"Camera 附着 Root、FirstPersonMesh 附着 Camera"的专用 Arms 结构 |

## 3. 新增成员变量（名称 / 类型 / UPROPERTY / 作用）

均位于 `UApecoxWeaponPresentationDefinition`：

| 名称 | 类型 | UPROPERTY | 作用 |
| --- | --- | --- | --- |
| `FirstPersonArmsFireMontage` | `TObjectPtr<UAnimMontage>` | `EditDefaultsOnly, BlueprintReadOnly, Category="FirstPerson\|Fire"` | Owning Player 的 RAR Arms 腰射 Montage |
| `FirstPersonWeaponFireMontage` | `TObjectPtr<UAnimMontage>` | 同上 | Owning Player 的 RAR 枪械机械动作 Montage |
| `ThirdPersonCharacterFireMontage` | `TObjectPtr<UAnimMontage>` | `Category="ThirdPerson\|Fire"` | Simulated Proxy 的 Manny 全身 Fire Montage |
| `ThirdPersonWeaponFireMontage` | `TObjectPtr<UAnimMontage>` | 同上 | 可选 TP 枪械机械动作 Montage；V1 允许为空 |
| `MuzzleSocketName` | `FName` | `EditDefaultsOnly, BlueprintReadOnly, Category="FirePresentation"` | FP/TP 武器 Mesh 上生成枪口表现的 Socket 名，默认 `"Muzzle"` |
| `MuzzleFlashSystem` | `TObjectPtr<UNiagaraSystem>` | 同上 | 一次性枪口 Niagara，允许为空 |
| `FireSound` | `TObjectPtr<USoundBase>` | 同上 | 一次性开火声音，允许为空 |

所有引用均为 `TObjectPtr` UPROPERTY，受 GC 追踪。原有 Mesh/附着/WorldPickup 字段全部保留。

## 4. 新增函数（完整签名 / 可见性 / UFUNCTION / 作用）

### 4.1 公开入口

```cpp
// UApecoxEquipmentComponent，public
UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment|Presentation")
void PlayFirePresentation();
```

作用：播放一次开火表现，由用户后续手工创建的 `GCN_Weapon_Fire`（GameplayCueNotify_Burst）调用。是唯一的视角分发入口。

### 4.2 内部辅助（protected）

```cpp
bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage) const;
```

作用：空指针安全地播放 Montage。不选择视角。Character/Arms 有 AnimBP 时走 `UAnimInstance::Montage_Play`；独立 Weapon Mesh 无 AnimInstance 时回退到 `USkeletalMeshComponent::PlayAnimation`（Single Node）。返回 `false` 当 Mesh 或 Montage 为空。

```cpp
void PlayMuzzlePresentation(USkeletalMeshComponent* WeaponMesh,
    const UApecoxWeaponPresentationDefinition* Presentation) const;
```

作用：在给定武器 Mesh 的 `MuzzleSocketName` 上生成一次性 Niagara（`SpawnSystemAttached`）与音效（`SpawnSoundAttached`）。空指针、空 Socket、Socket 不存在均安全返回，不崩溃。

## 5. PlayerCharacter 组件层级修改前后

**修改前**（完整 Manny Copy Pose 结构）：

```text
Root / Capsule
|- ThirdPersonMesh (GetMesh, OwnerNoSee)
|  `- FirstPersonMesh (附着 ThirdPersonMesh)
|     `- FirstPersonCamera (附着 FirstPersonMesh 的 head Socket)
```

**修改后**（专用 Arms 结构）：

```text
Root / Capsule
|- ThirdPersonMesh (GetMesh, OwnerNoSee)
`- FirstPersonCamera (附着 Capsule，使用 Pawn Control Rotation)
   `- FirstPersonMesh (附着 FirstPersonCamera，OnlyOwnerSee)
```

具体变更：
- `FirstPersonCamera` 不再附着 `FirstPersonMesh` 的 `head` Socket，改附着 `GetCapsuleComponent()`。
- `FirstPersonMesh` 不再附着 ThirdPersonMesh，改附着 FirstPersonCamera。
- 保留 `OnlyOwnerSee`、`NoCollision`、`FirstPersonPrimitiveType`、Pawn Control Rotation、FP FOV（70.0）与 FP Scale（0.6）。
- 摄像机初始位置改为 `FVector(0, 0, BaseEyeHeight)`——与服务端 `GetPawnViewLocation()` 一致，作为稳定眼高基线。
- 删除 `ABP_FP_Copy` / Copy Pose 注释与 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones` 强制设置。
- 未改动 CMC、输入、ASC、死亡、重生、拾取或网络移动逻辑。

## 6. 各端表现路径

`PlayFirePresentation()` 内部流程：Dedicated Server 早退 → 验证 Owner Character / WeaponDefinition / PresentationDefinition → `Character->IsLocallyControlled()` 分流。

| 场景 | 本机角色 | `IsLocallyControlled()` | 表现路径 |
| --- | --- | --- | --- |
| Standalone | 本地 Pawn | true | FP Arms + FP Weapon + FP 枪口 VFX/SFX |
| Listen Host（自己） | 本地 Pawn | true | FP（Authority 路径 Fire Cue 触发） |
| Listen Client（自己） | 本地 Pawn | true | FP（预测 Fire Cue 触发） |
| 其他玩家（任意客户端） | Simulated Proxy | false | TP Character + 可选 TP Weapon + TP 枪口 VFX/SFX |
| Dedicated Server | 无本地 Pawn | — | 早退，不创建/播放任何视觉资源 |

禁止同时播放 FP 与 TP 两条路径；未新增 RPC、Timer、全局去重布尔量或每 Tick 逻辑。

## 7. Build.cs 变化

`PrivateDependencyModuleNames` 新增 `"Niagara"`：

```csharp
PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "Niagara" });
```

`Niagara` 只在 `.cpp` 中使用（`UNiagaraFunctionLibrary::SpawnSystemAttached`），故作为私有依赖，未加入 `PublicDependencyModuleNames`。未增加任何无关模块。

## 8. 构建命令、结果与警告

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 17.54 seconds（增量构建）
错误：0
新增警告：0
```

既有警告 1 个（与本次修改无关）：`ApecoxAbilitySystemComponent.cpp:228` 的 `EGameplayAbilityInstancingPolicy::NonInstanced` 弃用警告，属 Phase 2B-1 已有代码，本轮未触碰。

首轮构建曾因 `ApecoxEquipmentComponent.h` 缺少 `UApecoxWeaponPresentationDefinition` 前向声明而失败，补上前向声明后通过（见第 11 节）。

## 9. 未修改范围

- `UApecoxHitscanFireAbility`：TargetData、两阶段 Trace、PredictionKey、扣弹、伤害、Shot Confirmation 全部未改。
- `UApecoxWeaponStateComponent`、射击 Tag、GA/GE/Attribute/AbilityTask：未改，未新增。
- 网络：未新增 RPC、未改复制属性；`CurrentWeaponInstance` 与 `EquippedWeaponState` 复制语义不变。
- 装备/卸下：`EquipWeapon`、`UnequipCurrentWeapon` 幂等性未破坏。
- 资产：未创建、修改、迁移或保存任何 `.uasset`/`.umap`。

## 10. 后续由用户完成的 UE 配置（仅列类别）

以下仅列类别，具体资产名、Montage Slot、Socket 命名与 DataAsset 字段值由 Codex 在审查通过后写入 `Current_UE_Manual_Steps.md`，由用户在 UE 编辑器中手工完成：

1. **第一人称 Arms 表现资产**：从 RAR 原始 Animation Sequence 创建 Apecox 自有 FP Arms Fire Montage（去除厂商 `Update Tags` Notify），并配置 FirstPersonMesh 的 Arms AnimClass / Mesh / 相对 Transform。
2. **第一人称武器表现资产**：创建 Apecox 自有 FP Weapon Fire Montage（武器机械动作）。
3. **第三人称 Fire Montage**：从 Rifle Pro 重定向到 Manny 后创建 Apecox 自有 TP Character Fire Montage（及可选 TP Weapon Montage）。
4. **GameplayCue Blueprint**：手工创建 `GCN_Weapon_Fire`（父类 `GameplayCueNotify_Burst`，Tag `GameplayCue.Weapon.Fire`），Cue 内只调用 `Target Character -> GetEquipmentComponent -> PlayFirePresentation`。
5. **武器 Presentation DataAsset 配置**：在 `DA_WeaponPresentation_Rifle` 中填充本轮新增的 7 个表现字段（含 `MuzzleSocketName`、`MuzzleFlashSystem`、`FireSound`）。
6. **枪口 Socket 与特效**：确认武器 Mesh 存在 `Muzzle` Socket；准备一次性枪口 Niagara 与开火音效资产。

## 11. 与批准命名/设计不一致之处及理由

本轮未出现与批准命名或公开配置不一致之处。以下为需向 Codex 说明的设计取舍（均落在 Prompt 允许范围内）：

1. **摄像机初始位置采用 `BaseEyeHeight`**：Prompt 要求"Capsule/Root 下的稳定眼高初始位置，不硬编码 RAR 专属 Transform"。我选用 `APawn::BaseEyeHeight`（默认 64.0），它与服务端 `GetPawnViewLocation()`（`GetActorLocation() + (0,0,BaseEyeHeight)`）精确对齐，使客户端 ViewOrigin 与服务端权威视点保持一致，利于 Hitscan 验证通过。原 `head` Socket 相对偏移 `(-2.8, 5.89, 0)` 及 `(0,90,-90)` 旋转不再适用，一并移除。

2. **摄像机相对旋转归零**：原 `(0,90,-90)` 是为对齐 `head` Socket 方向。改为附着 Capsule 后，`bUsePawnControlRotation=true` 使视图旋转完全由 Controller 控制旋转驱动，相对旋转不影响视角，故不再设置。

3. **`Niagara` 为私有依赖**：`UNiagaraSystem` 仅在 `ApecoxEquipmentComponent.cpp` 中被完整使用；公共头文件只前向声明，故按"最小依赖"放入 `PrivateDependencyModuleNames` 而非 Public。

4. **`PlayMontageOnMesh` 的 Single Node 回退判定**：用 `AnimInstance && !GetSingleNodeInstance()` 区分"有 AnimBP 的 Character/Arms"与"无 AnimInstance 或仅 SingleNode 的 Weapon Mesh"。该判定确保 Character/Arms 绝不会被切成 Single Node 模式，Single Node 回退只可能命中两个动态 Weapon Mesh Component。

---

*本轮未执行购买、下载、项目资产修改、MCP 或 Git 操作。构建通过后停止，等待 Codex 审查。*
