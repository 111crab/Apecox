# Claude Code 实施 Prompt：M1-A 第一人称完整步枪表现 Prefab

你是 Apecox 项目的具体实施子代理。本窗口可能没有此前上下文。Codex 负责高层规划与代码审查，用户负责公开命名审阅、UE 编辑器资产配置和最终 PIE 验证；你只负责严格实施本文批准的 C++、执行构建并提交中文报告。

## 一、项目与必读文件

项目根目录：

```text
D:/UnrealProject/Apecox
```

开始前按顺序阅读：

```text
Agent/README.md
Agent/00_Coordination/Current_Phase.md
Agent/00_Coordination/Current_Code_Design.md
Agent/00_Coordination/Subagent_Review_Checklist.md
.agents/ue-project-context.md
```

然后阅读：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Apecox.Build.cs
```

工作区可能包含用户或前序任务改动。不得恢复、覆盖、清理或格式化无关文件；不要把时间花在 Git 分类上，不执行 `git add`、`commit` 或 `push`。

## 二、背景与唯一目标

现有运行链路已经成立：

```text
InputTag.Weapon.Fire
-> UApecoxHitscanFireAbility
-> 客户端预测 / TargetData / 服务器逐发校验
-> 权威扣弹、命中与伤害
-> GameplayCue.Weapon.Fire
-> UApecoxEquipmentComponent::PlayFirePresentation()
```

目前第一人称已经能显示 RAR Arms、播放开火动画、Niagara 和声音。问题只在枪械资产：`SK_RAR_AssaultRifle` 是枪体，默认弹匣、护木和机械瞄具是独立 Mesh；现有 `FirstPersonWeaponMesh` 单字段无法组成一把完整枪械，也无法在 Blueprint Viewport 中统一预览和调节。

本轮只建立一个**固定完整武器表现 Prefab**：

- 不是运行时模块化武器系统；
- 不允许玩家拆装配件；
- 不改变枪械功能或数值；
- 不处理远端 TP；
- 不触碰现有射击玩法链路。

## 三、新增 AApecoxWeaponPresentationActor

新增镜像文件：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp
```

类定义：

```cpp
AApecoxWeaponPresentationActor : AActor
```

要求：

- 使用 `APECOX_API`。
- `UCLASS(Blueprintable, Abstract)`，实际实例由项目 Blueprint 子类提供。
- 默认不复制、不开 Tick、Actor Collision 关闭。
- 构造函数只创建默认组件，不访问 World。
- 关键注释使用中文，同时解释“为什么表现与玩法真相分离”。

固定组件和层级：

```text
PresentationRoot : USceneComponent
`- WeaponMesh : USkeletalMeshComponent
   `- MuzzlePoint : USceneComponent
```

批准成员名：

```cpp
TObjectPtr<USceneComponent> PresentationRoot;
TObjectPtr<USkeletalMeshComponent> WeaponMesh;
TObjectPtr<USceneComponent> MuzzlePoint;
```

要求：

- 使用 `CreateDefaultSubobject`。
- `WeaponMesh` 默认关闭碰撞和 Overlap。
- 组件通过 `VisibleAnywhere, BlueprintReadOnly` 暴露，Blueprint 子类可以在 `WeaponMesh` 下添加固定默认部件。
- 不保存 WeaponInstance、弹药、射速、伤害、命中、配件或复制状态。
- 不主动播放 Montage、生成 Cue 或执行 Trace。

批准公开访问函数：

```cpp
USkeletalMeshComponent* GetWeaponMesh() const;
USceneComponent* GetMuzzlePoint() const;
```

可以标记为 `BlueprintPure`，不得增加其他无必要的公开 API。

## 四、调整 UApecoxWeaponPresentationDefinition

在 `ApecoxWeaponPresentationDefinition.h` 前向声明 `AApecoxWeaponPresentationActor`。

使用已批准字段：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson")
TSubclassOf<AApecoxWeaponPresentationActor> FirstPersonWeaponPresentationClass;
```

替换并删除旧字段：

```cpp
TObjectPtr<USkeletalMesh> FirstPersonWeaponMesh;
```

本轮必须保留且不得改名：

```text
FirstPersonAttachSocket
FirstPersonAttachTransform
ThirdPersonWeaponMesh
ThirdPersonAttachSocket
ThirdPersonAttachTransform
WorldPickupMesh
FirstPersonArmsFireMontage
FirstPersonWeaponFireMontage
ThirdPersonCharacterFireMontage
ThirdPersonWeaponFireMontage
MuzzleSocketName
MuzzleFlashSystem
FireSound
```

注意：

- `MuzzleSocketName` 暂时只继续服务 TP 单 Mesh 路径。
- 第一人称改用 Presentation Actor 的显式 `MuzzlePoint`。
- 保持直接类引用，不引入 Soft Class、AssetManager 或异步加载状态机。
- 更新类注释，删除“Mesh 字段足以表达完整枪械”的旧语义。

## 五、调整 UApecoxEquipmentComponent

### 5.1 运行时引用

前向声明 `AApecoxWeaponPresentationActor` 和必要组件类型。

使用：

```cpp
UPROPERTY(Transient)
TObjectPtr<AApecoxWeaponPresentationActor> FirstPersonWeaponPresentationActor;
```

替换：

```cpp
TObjectPtr<USkeletalMeshComponent> FirstPersonWeaponMeshComponent;
```

`ThirdPersonWeaponMeshComponent` 保持原类型、原字段和原逻辑，不得借机重构 TP。

### 5.2 RefreshWeaponPresentation()

保留现有前置检查、装备摘要来源和 Dedicated Server 早退。

FP 路径要求：

1. 只有 `Character->IsLocallyControlled()` 且 `FirstPersonWeaponPresentationClass` 有效时才生成 FP Actor。
2. 使用当前 `World` 生成 Actor，Owner 设置为 Character；Actor 本身不得复制。
3. 生成失败输出包含类名和 Character 名称的 Warning，但不影响装备玩法状态。
4. Actor 附着到 `Character->GetFirstPersonMesh()` 的 `FirstPersonAttachSocket`，使用 `SnapToTargetIncludingScale`，随后应用 `FirstPersonAttachTransform`。
5. 遍历 Presentation Actor 内全部 `UPrimitiveComponent`，统一设置：

```text
OnlyOwnerSee = true
OwnerNoSee = false
CollisionEnabled = NoCollision
GenerateOverlapEvents = false
FirstPersonPrimitiveType = FirstPerson
```

6. 不能只配置 `WeaponMesh`，因为 Blueprint 后续添加的固定弹匣、护木和机械瞄具同样必须遵守 FP 可见性和碰撞规则。
7. 重复刷新不得生成多个 FP Actor；空 Class 时安全跳过。

TP 路径继续使用现有：

```cpp
ThirdPersonWeaponMeshComponent
Pres->ThirdPersonWeaponMesh
```

不要新增 `ThirdPersonPresentationClass`，不要修改 TP AnimBP 或 TP Montage 选择。

### 5.3 DestroyWeaponPresentation()

- 使用 `AActor::Destroy()` 幂等销毁有效的 `FirstPersonWeaponPresentationActor`，随后清空引用。
- 不使用 `delete`、`ConditionalBeginDestroy()` 或手工 GC。
- 继续按原逻辑销毁 `ThirdPersonWeaponMeshComponent`。
- 重复调用、OnRep 刷新、卸下和 EndPlay 均不得崩溃。

### 5.4 PlayMontageOnMesh()

Owning Player 的枪械 Montage 目标改为：

```cpp
FirstPersonWeaponPresentationActor->GetWeaponMesh()
```

更新 Single Node 回退的受限判定：只允许 FP Presentation Actor 的 `WeaponMesh` 与原 `ThirdPersonWeaponMeshComponent` 使用 `PlayAnimation()` 回退。Character 和 Arms 缺少 AnimInstance 时仍必须直接失败，不能切换 Animation Mode。

### 5.5 PlayMuzzlePresentation()

将内部函数调整为能同时支持：

```text
FP：显式 MuzzlePoint 组件本身，AttachPointName = None
TP：ThirdPersonWeaponMeshComponent + MuzzleSocketName
```

推荐签名：

```cpp
void PlayMuzzlePresentation(
    USceneComponent* AttachComponent,
    FName AttachPointName,
    const UApecoxWeaponPresentationDefinition* Presentation) const;
```

行为要求：

- `AttachComponent` 或 `Presentation` 无效时安全返回。
- `AttachPointName != NAME_None` 时验证 Socket 存在；缺失只跳过并输出 Verbose。
- `AttachPointName == NAME_None` 时使用组件自身 Transform，服务 FP `MuzzlePoint`。
- Niagara 和 Sound 都附着到同一 Component/AttachPoint。
- 空 VFX 或空 SFX 分别跳过，不影响另一项，也不影响射击结算。
- TP 的 `MuzzleSocketName == NAME_None` 时保持旧语义：跳过 TP 枪口表现，不允许回退到 TP 武器原点。

### 5.6 PlayFirePresentation()

Owning Player：

```text
Character FirstPersonMesh -> FirstPersonArmsFireMontage
FP Presentation WeaponMesh -> FirstPersonWeaponFireMontage
FP Presentation MuzzlePoint -> MuzzleFlashSystem + FireSound
```

非本地观察者：保持原 TP Character、TP Weapon Mesh 和 TP Muzzle Socket 路径。

不得同时播放两条视角路径，不新增 RPC、Multicast、Timer、Tick 或去重状态。

## 六、网络与职责硬边界

- Presentation Actor 是当前机器本地生成的纯表现对象，不复制、不承载玩法状态。
- Dedicated Server 不生成 FP Actor，也不访问 Niagara、Sound 或渲染组件。
- Standalone、Listen Host 和 Owning Client 的本地 Pawn 使用 FP Actor。
- 远端角色不生成 FP Actor，继续使用现有 TP Mesh 路径。
- 不修改 `EquippedWeaponState`、`CurrentWeaponInstance`、AbilitySet 授予/撤销或 Inventory。
- 不修改 `UApecoxHitscanFireAbility`、TargetData、PredictionKey、服务器 Trace、RPM、弹匣、伤害和 Shot Confirmation。

## 七、允许修改范围

允许新增/修改：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Agent/Reports/2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_Report.md
```

只有出现真实新增模块依赖时才允许修改 `Source/Apecox/Apecox.Build.cs`；预计本轮不需要。

禁止：

- 创建运行时配件槽、配件 Definition、Modifier 或外观切换系统。
- 新增 GameplayTag、GA、GE、Attribute、AbilityTask、RPC 或复制变量。
- 修改 Character、PlayerController、ASC、WeaponInstance、WeaponStateComponent、Hitscan GA 或输入。
- 创建、修改、迁移或删除任何 `.uasset`、`.umap`、Config 或项目设置。
- 使用 MCP 操作 UE 编辑器。
- 硬编码 RAR 资产路径、部件名或最终 Transform。
- 依赖 RAR Demo Gameplay Blueprint。
- 生成 Rider/Visual Studio 解决方案。
- 执行 Git 暂存、提交、推送或清理。
- 无关重构、全仓格式化或修复未在范围内的问题。

发现范围外问题时，只在报告中记录，不自行修复。

## 八、构建与自检

使用：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

编辑器运行导致 DLL 锁定时，只报告，不强制结束用户进程。

至少检查：

- Public/Private 目录镜像、API 宏、反射、前向声明、Include 和 GC 引用。
- Actor 默认组件、Blueprintable/Abstract、无 Tick、无复制、无碰撞。
- Spawn、Attach、重复 Refresh、Destroy、OnRep 和 EndPlay 生命周期。
- 远端角色不创建 FP Actor，TP 现有路径未被破坏。
- Blueprint 新增 PrimitiveComponent 能被统一配置为 FP 表现。
- Arms/Character 不会被误切为 Single Node。
- MuzzlePoint 与 TP Socket 两种枪口路径的空值和缺失 Socket 安全。
- 原射击、弹匣、伤害、PredictionKey 和网络文件未被修改。

构建成功不代表 UE 表现验证成功。不要猜测最终 Transform，也不要声称资产已经完成。

## 九、中文实施报告

新增：

```text
Agent/Reports/2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_Report.md
```

报告必须包含：

1. 实际修改文件以及是否超出 Prompt。
2. 新增类名、父类、文件路径、UCLASS 标记和职责。
3. 新增/替换成员变量的名称、类型、UPROPERTY 和作用。
4. 新增/修改函数的完整签名、可见性、UFUNCTION 和作用。
5. FP Actor 的 Spawn、Attach、Primitive 配置和 Destroy 生命周期。
6. FP MuzzlePoint 与 TP Socket 的分流实现。
7. Standalone、Listen Host、Owning Client、远端观察者、Dedicated Server 的代码路径。
8. Build.cs 是否变化。
9. 构建命令、结果、警告和未执行项。
10. 明确未修改的 GAS、射击、网络、TP、资产和配件系统范围。
11. 用户后续只需创建哪类 Blueprint 和迁移哪类 Mesh；不要猜最终 Transform。
12. 所有与批准设计、命名或本文要求不一致之处及理由。

完成构建和报告后立即停止，等待 Codex 审查。不要继续 UE 配置、迁移资产、创建修复 Prompt 或进入远端 TP 阶段。
