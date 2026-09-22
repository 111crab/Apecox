# M1-A 第一人称完整步枪表现 Prefab 实施报告

实施日期：2026-08-24  
构建结果：**通过**（0 错误，0 新增警告）  
状态：等待 Codex 审查。

## 1. 实际修改文件（是否超出 Prompt）

严格限定在 Prompt 第七节「允许修改范围」内，未超出：

| 文件 | 改动 |
| --- | --- |
| `Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h` | 新增 |
| `Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp` | 新增 |
| `Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h` | 替换 FP 字段 + 更新注释 |
| `Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h` | 替换运行时成员 + 调整 `PlayMuzzlePresentation` 签名 |
| `Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp` | FP Spawn/Attach/Configure/Destroy/Fire 改造 |
| `Agent/Reports/2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_Report.md` | 新增本报告 |

未修改 `Apecox.Build.cs`、Character、PlayerController、ASC、WeaponInstance、WeaponStateComponent、Hitscan GA、TargetData、GameplayCue、复制逻辑或任何 `.uasset/.umap`；未调用 MCP、未生成解决方案、未执行 Git。

## 2. 新增类

| 项 | 值 |
| --- | --- |
| 类名 | `AApecoxWeaponPresentationActor` |
| 父类 | `AActor` |
| 文件 | `Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h` / `Private/...cpp` |
| UCLASS | `UCLASS(Blueprintable, Abstract)` |
| 职责 | 纯表现武器 Prefab：把 RAR 枪体和默认部件在 Blueprint Viewport 固定组装成完整武器，只向 EquipmentComponent 提供表现组件，不复制、不 Tick、无碰撞、不承载玩法状态。 |

组件层级：

```text
PresentationRoot : USceneComponent
`- WeaponMesh : USkeletalMeshComponent
   `- MuzzlePoint : USceneComponent
```

构造函数只创建默认组件、不访问 World；`WeaponMesh` 默认关闭碰撞与 Overlap。

## 3. 新增/替换成员变量

### 新增类 `AApecoxWeaponPresentationActor`

| 成员名 | 类型 | UPROPERTY | 作用 |
| --- | --- | --- | --- |
| `PresentationRoot` | `TObjectPtr<USceneComponent>` | `VisibleAnywhere, BlueprintReadOnly` | Prefab 根节点，完整枪械相对 Arms 的统一调整基准 |
| `WeaponMesh` | `TObjectPtr<USkeletalMeshComponent>` | `VisibleAnywhere, BlueprintReadOnly` | RAR 枪体，枪械 Montage 播放目标 |
| `MuzzlePoint` | `TObjectPtr<USceneComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 显式枪口锚点，FP 枪口 VFX/SFX 附着位置 |

### `UApecoxWeaponPresentationDefinition`

| 成员名 | 类型 | UPROPERTY | 作用 |
| --- | --- | --- | --- |
| `FirstPersonWeaponPresentationClass`（新增） | `TSubclassOf<AApecoxWeaponPresentationActor>` | `EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson"` | FP 完整武器表现 Prefab 的 Blueprint 子类 |
| `FirstPersonWeaponMesh`（删除） | `TObjectPtr<USkeletalMesh>` | — | 被上者替换 |

其余字段（`FirstPersonAttachSocket/Transform`、全部 TP 字段、`WorldPickupMesh`、7 个开火字段）保持不变。

### `UApecoxEquipmentComponent`

| 成员名 | 类型 | UPROPERTY | 作用 |
| --- | --- | --- | --- |
| `FirstPersonWeaponPresentationActor`（新增） | `TObjectPtr<AApecoxWeaponPresentationActor>` | `Transient` | 运行时生成的 FP 完整武器表现 Actor |
| `FirstPersonWeaponMeshComponent`（删除） | `TObjectPtr<USkeletalMeshComponent>` | — | 被上者替换 |
| `ThirdPersonWeaponMeshComponent` | `TObjectPtr<USkeletalMeshComponent>` | `Transient` | 保持不变，仍走旧 TP 单 Mesh 路径 |

## 4. 新增/修改函数

### 新增公开访问函数（`AApecoxWeaponPresentationActor`）

```cpp
UFUNCTION(BlueprintPure, Category = "Apecox|Presentation")
USkeletalMeshComponent* GetWeaponMesh() const;

UFUNCTION(BlueprintPure, Category = "Apecox|Presentation")
USceneComponent* GetMuzzlePoint() const;
```

### 修改函数签名（`UApecoxEquipmentComponent`）

`PlayMuzzlePresentation` 由「传入武器 Mesh」改为「传入附着组件 + 附着点名」，以同时服务 FP MuzzlePoint 与 TP Socket：

```cpp
// 旧
void PlayMuzzlePresentation(USkeletalMeshComponent* WeaponMesh,
    const UApecoxWeaponPresentationDefinition* Presentation) const;

// 新
void PlayMuzzlePresentation(USceneComponent* AttachComponent, FName AttachPointName,
    const UApecoxWeaponPresentationDefinition* Presentation) const;
```

其余函数（`PlayFirePresentation()`、`RefreshWeaponPresentation()`、`DestroyWeaponPresentation()`、`PlayMontageOnMesh()`）签名不变，仅内部实现调整。

## 5. FP Actor 生命周期

`RefreshWeaponPresentation()` 保留前置检查与 Dedicated Server 早退，FP 路径改为：

1. **Spawn**：仅当 `Character->IsLocallyControlled()` 且 `Pres->FirstPersonWeaponPresentationClass` 有效时才生成；用 `GetWorld()->SpawnActor` 生成，Owner 设为 Character，`bNoFail = true` 关闭引擎默认失败日志，改由自定义 Warning 输出类名与角色名；已有 Actor 时跳过（防重复）。
2. **Configure**：`GetComponents<UPrimitiveComponent>` 遍历 Actor 内全部 Primitive（含 Blueprint 后续添加的弹匣/护木/瞄具），统一设置 `OnlyOwnerSee=true`、`OwnerNoSee=false`、`CollisionEnabled=NoCollision`、`GenerateOverlapEvents=false`、`FirstPersonPrimitiveType=FirstPerson`。
3. **Attach**：`AttachToComponent(GetFirstPersonMesh(), SnapToTargetIncludingScale, FirstPersonAttachSocket)` 后 `SetActorRelativeTransform(FirstPersonAttachTransform)`。
4. **Destroy**：`DestroyWeaponPresentation()` 用 `AActor::Destroy()` 幂等销毁有效 Actor 后置空；不用 `delete`/`ConditionalBeginDestroy`/手工 GC。
5. 空 Class 安全跳过；重复 Refresh、OnRep、卸下、EndPlay 均安全（`EndPlay`、`OnRep_EquippedWeaponState`、`UnequipCurrentWeapon` 都先 Destroy 再 Refresh）。

## 6. FP MuzzlePoint 与 TP Socket 分流

- **FP**：`PlayMuzzlePresentation(FPWeaponActor->GetMuzzlePoint(), NAME_None, Pres)` —— 附着点名 None，使用组件自身 Transform；Niagara 与 Sound 都附着到同一 `MuzzlePoint` 组件。
- **TP**：`PlayMuzzlePresentation(ThirdPersonWeaponMeshComponent, Pres->MuzzleSocketName, Pres)` —— 附着点名非 None 时先 `DoesSocketExist` 验证，缺失仅 Verbose 跳过；`MuzzleSocketName == NAME_None` 时保持旧语义，跳过 TP 枪口表现、不回退到武器原点。
- 判别依据：`AttachPointName == NAME_None` 且 `AttachComponent->IsA<USkeletalMeshComponent>()` 时判定为「TP 武器无 Socket」并跳过；普通 `USceneComponent`（FP MuzzlePoint）继续使用组件 Transform。空 VFX/SFX 分别独立跳过，不影响另一项与射击结算。

## 7. 各网络角色代码路径

| 角色 | 行为 |
| --- | --- |
| Dedicated Server | `RefreshWeaponPresentation()` 首行早退，不生成 FP Actor、不访问 Niagara/Sound/渲染；`PlayFirePresentation()` 同样早退 |
| Standalone | 本地 Pawn 被本地控制，走 FP 路径生成 Presentation Actor 并播放 FP 表现 |
| Listen Host | 自己的 Pawn 走 FP；远程客户端 Pawn 是 Simulated Proxy，走 TP 路径 |
| Owning Client | 本地 Pawn `IsLocallyControlled()==true`，走 FP；不生成 TP 武器（TP 仍由 `ThirdPersonWeaponMeshComponent` 生成并 `OwnerNoSee`） |
| 远端观察者 | `IsLocallyControlled()==false`，不生成 FP Actor，继续用 `ThirdPersonWeaponMeshComponent` + TP Montage + TP Socket 表现 |

`PlayFirePresentation()` 通过 `IsLocallyControlled()` 二选一，绝不同时播放两条视角路径，不新增 RPC/Multicast/Timer/Tick/去重状态。

## 8. Build.cs 是否变化

无变化。本轮未引入新模块依赖（`USceneComponent`、`UPrimitiveComponent`、`SkeletalMeshComponent`、`SpawnActor`、Niagara、Sound 均在现有 `Engine`/`Niagara` 依赖内）。

## 9. 构建结果

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 28.66 seconds
错误：0
新增警告：0
```

非代码提示：`UbaServer - bind 0.0.0.0:1345 failed (...)` —— 本地端口占用导致 UBA 服务绑定失败，回退到本地执行器完成构建，与本次代码无关。

未执行项：未进入 UE 编辑器、未做 PIE 验证、未迁移资产、未创建 Blueprint、未执行 Git。

## 10. 明确未修改范围

- GAS：未改 `UApecoxHitscanFireAbility`、Hitscan TargetData、PredictionKey、服务器 Trace、RPM、弹匣、伤害、Shot Confirmation。
- 网络：未改 `EquippedWeaponState`、`CurrentWeaponInstance`、AbilitySet 授予/撤销、Inventory、任何 RPC/复制变量。
- 表现：未改 TP Manny、TP AnimBP、TP 单 Mesh 路径、`ThirdPersonWeaponMeshComponent`。
- 资产：未创建/修改/迁移/删除任何 `.uasset/.umap`；未新增 GameplayTag、GA、GE、Attribute、GameplayCue、DataAsset 或 AbilityTask。
- 配件系统：未实现配件槽、配件拾取、外观动态替换或 Modifier。

## 11. 用户后续 UE 操作（不猜最终 Transform）

代码审查通过后，用户只需：

1. 创建 Blueprint 子类（父类 `AApecoxWeaponPresentationActor`），建议路径 `/Game/Blueprints/Weapons/Rifle/BP_Apecox_RiflePresentation_FP`。
2. 在该 Blueprint Viewport 中把 RAR 枪体 + 默认弹匣/护木/机械瞄具固定组装到 `WeaponMesh` 下，并摆放 `MuzzlePoint`；迁移 RAR 5.8 工程相关 Mesh 及依赖，不迁移供应商 Demo Gameplay Blueprint。
3. 在武器 Presentation DataAsset 上把 `FirstPersonWeaponPresentationClass` 指向该 Blueprint，并按实际资产微调 `FirstPersonAttachSocket` 与 `FirstPersonAttachTransform`。

最终 Socket 名与 Transform 以实际资产为准，本报告不猜测。

## 12. 与批准设计/命名不一致之处

无。类名 `AApecoxWeaponPresentationActor`、成员名 `PresentationRoot/WeaponMesh/MuzzlePoint`、DataAsset 字段 `FirstPersonWeaponPresentationClass`、访问函数 `GetWeaponMesh/GetMuzzlePoint` 均与 `Current_Code_Design.md` 与 Prompt 完全一致。

---

*构建通过后停止，等待 Codex 审查。*
