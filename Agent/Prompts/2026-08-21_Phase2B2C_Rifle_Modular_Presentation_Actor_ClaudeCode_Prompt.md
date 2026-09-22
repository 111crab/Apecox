# Claude Code 实施 Prompt：Phase 2B-2C 模块化武器表现 Actor

> **状态：已被取代，不得执行。** 请改用 `2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_ClaudeCode_Prompt.md`。旧方案同时重构 FP/TP 且使用“模块化武器”口径，已不符合当前固定完整枪械、先独立收口 FP 的范围。

你是 Apecox 项目的具体实施子代理。本窗口可能没有此前上下文，必须以本文和列出的项目文档为准。Codex 负责高层设计与代码审查，用户负责公开命名审阅和 UE 编辑器人工配置；你的职责是严格实施已经批准的 C++、完成构建并提交中文报告。

## 一、工作区与必读文件

项目根目录：

```text
D:/UnrealProject/Apecox
```

开始前按顺序阅读：

```text
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

参考 Lyra 的“装备实例生成纯表现 Actor”思想，但不得复制其完整装备框架：

```text
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Equipment/LyraEquipmentDefinition.h
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Equipment/LyraEquipmentInstance.h
D:/UnrealProject/LyraStarterGame/Source/LyraGame/Equipment/LyraEquipmentInstance.cpp
```

先执行 `git status --short`，只用于理解工作区；不得恢复、覆盖或清理用户及此前阶段的改动，不执行 `git add/commit/push`。

## 二、背景与目标

现有 Phase 2B-2B 已经完成并验证：

```text
GameplayCue.Weapon.Fire
-> UApecoxEquipmentComponent::PlayFirePresentation()
-> FP/TP Arms/Character Montage
-> Weapon Montage
-> Muzzle Niagara + Fire Sound
```

射击输入、自动连射、弹匣、客户端预测、服务器逐发验证、命中、伤害和 Shot Confirmation 均已成立。

当前问题只在表现装配：RAR 的 `SK_RAR_AssaultRifle` 是模块化枪械主体，默认弹匣、护木和机械瞄具是独立部件。现有“运行时生成一个 USkeletalMeshComponent”的设计无法表达完整枪械，也无法在 Blueprint Viewport 中预览装配。

本轮将 FP/TP 动态 Weapon Mesh Component 替换为**本地生成、非复制、纯表现的 Actor**。不得触碰射击玩法链路。

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

要求 `Blueprintable`，可以设为 `Abstract`，因为实际实例由项目自建 Blueprint 子类提供。

构造函数创建：

```cpp
TObjectPtr<USceneComponent> PresentationRoot;
TObjectPtr<USkeletalMeshComponent> WeaponMesh;
TObjectPtr<USceneComponent> MuzzlePoint;
```

层级：

```text
PresentationRoot
`- WeaponMesh
   `- MuzzlePoint
```

公开只读访问：

```cpp
USkeletalMeshComponent* GetWeaponMesh() const;
USceneComponent* GetMuzzlePoint() const;
```

约束：

- Actor 默认不复制、不开 Tick。
- 自身组件无碰撞；Blueprint 后续新增的 PrimitiveComponent 也会由 EquipmentComponent 在生成后统一关闭碰撞。
- 不保存 WeaponInstance、弹药、伤害或装备槽位，不执行 Trace，不发送 RPC。
- `MuzzlePoint` 是显式表现锚点，允许用户在 Blueprint Viewport 直接移动到枪口，不依赖第三方 Socket 名。
- C++ 注释使用中文，说明“表现载体”和“玩法真相”的边界，避免逐行复述。

如果为视角配置增加内部非 UFUNCTION 辅助函数是必要的，可以实现，但不得增加新的公开配置字段、枚举或玩法状态；报告中必须列出并解释。

## 四、调整 UApecoxWeaponPresentationDefinition

前向声明 `AApecoxWeaponPresentationActor`，使用以下字段替换原有两个武器 Mesh 字段：

```cpp
TSubclassOf<AApecoxWeaponPresentationActor> FirstPersonPresentationActorClass;
TSubclassOf<AApecoxWeaponPresentationActor> ThirdPersonPresentationActorClass;
```

删除：

```text
FirstPersonWeaponMesh
ThirdPersonWeaponMesh
MuzzleSocketName
```

保留且不得改名：

```text
FirstPersonAttachSocket
FirstPersonAttachTransform
ThirdPersonAttachSocket
ThirdPersonAttachTransform
WorldPickupMesh
FirstPersonArmsFireMontage
FirstPersonWeaponFireMontage
ThirdPersonCharacterFireMontage
ThirdPersonWeaponFireMontage
MuzzleFlashSystem
FireSound
```

要求：

- `FirstPersonPresentationActorClass` 与 `ThirdPersonPresentationActorClass` 使用 `EditDefaultsOnly, BlueprintReadOnly`，分类清晰。
- 保持直接类引用；已装备武器的表现属于常驻工作集，本轮不引入异步流送状态机。
- 更新中文类注释，明确 DataAsset 选择表现载体，但不保存运行时真相。
- `WorldPickupMesh` 继续服务地面拾取物，不改世界拾取流程。

## 五、调整 UApecoxEquipmentComponent

### 5.1 运行时引用

使用以下 UPROPERTY 引用替换旧的 FP/TP Weapon Mesh Component 引用：

```cpp
TObjectPtr<AApecoxWeaponPresentationActor> FirstPersonPresentationActor;
TObjectPtr<AApecoxWeaponPresentationActor> ThirdPersonPresentationActor;
```

必须为 GC 安全的 `UPROPERTY(Transient)`；不得复制这些引用。

### 5.2 RefreshWeaponPresentation

保持现有前置检查和 Dedicated Server 早退。对两个已配置 Class：

1. 使用 Character 作为 Owner/Instigator，在当前 World 本地生成表现 Actor。
2. 不开启 Actor 复制。
3. FP Actor 附着到 `Character->GetFirstPersonMesh()` 的 `FirstPersonAttachSocket`，应用 `FirstPersonAttachTransform`。
4. TP Actor 附着到 `Character->GetMesh()` 的 `ThirdPersonAttachSocket`，应用 `ThirdPersonAttachTransform`。
5. 遍历每个表现 Actor 的 `UPrimitiveComponent`：关闭碰撞；FP 设置 `OnlyOwnerSee=true`、`OwnerNoSee=false`、`FirstPersonPrimitiveType=FirstPerson`；TP 设置 `OwnerNoSee=true`、`OnlyOwnerSee=false`、`FirstPersonPrimitiveType=WorldSpaceRepresentation`。
6. 不假设 Blueprint 只有 WeaponMesh；默认弹匣、护木、机械瞄具等新增 PrimitiveComponent 必须继承相同视角规则。

Spawn 与 Attach 的顺序必须符合 UE Actor 生命周期；若使用 Deferred Spawn，确保 Blueprint Construction Script 能正常建立部件后再统一配置组件。空 Class或 Spawn 失败只跳过对应视角并输出有帮助的 Warning，不影响装备玩法状态。

### 5.3 DestroyWeaponPresentation

幂等销毁两个表现 Actor 并清空引用。重复调用、RepNotify 刷新、卸下武器和 Owner 销毁时不得崩溃。不得用 `ConditionalBeginDestroy()` 或手工 delete。

### 5.4 PlayFirePresentation

保留现有 `IsLocallyControlled()` 唯一视角分流：

- Owning Player：在 Character FirstPersonMesh 播放 `FirstPersonArmsFireMontage`；在 `FirstPersonPresentationActor->GetWeaponMesh()` 播放 `FirstPersonWeaponFireMontage`；使用 FP Actor 的 `MuzzlePoint` 播放 Niagara/SFX。
- Simulated Proxy：在 Character ThirdPersonMesh 播放 `ThirdPersonCharacterFireMontage`；在 TP Actor WeaponMesh 播放可选的 `ThirdPersonWeaponFireMontage`；使用 TP Actor 的 `MuzzlePoint` 播放 Niagara/SFX。

调整 `PlayMuzzlePresentation` 的参数，使其接收表现 Actor 或明确的 `USceneComponent* MuzzlePoint`；选择更清晰、最小的签名并在报告中说明。它必须：

- 在有效 `MuzzlePoint` 上附着生成一次性 Niagara 与 Sound；
- 不再读取 `MuzzleSocketName`；
- 空 Actor/Point/VFX/SFX 安全跳过；
- 不影响射击是否成立。

`PlayMontageOnMesh()` 可以保留现有实现，但只能在 Character/Arms 的 AnimInstance 路径和 WeaponMesh 的受限 Single Node 回退中工作，不得把 Character/Arms 切换为 Single Node。

## 六、网络与生命周期边界

- FP/TP Presentation Actor 都是当前机器根据已复制装备摘要本地重建的纯表现对象，不复制、不 Multicast。
- Standalone / Listen Host / Owning Client 的本地 Pawn 只播放 FP。
- 其他客户端的 Simulated Proxy 只播放 TP。
- Dedicated Server 不生成表现 Actor，也不访问 Niagara、Sound 或渲染组件。
- 不改变 `EquippedWeaponState`、`CurrentWeaponInstance`、AbilitySet 授予/移除和 RepNotify 职责。
- 不新增 Timer、Tick、全局布尔去重或网络状态。

## 七、允许修改范围

允许新增/修改：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Source/Apecox/Apecox.Build.cs（仅确有新增模块依赖时）
Agent/Reports/2026-08-21_Phase2B2C_Rifle_Modular_Presentation_Actor_Report.md
```

禁止：

- 修改 `UApecoxHitscanFireAbility`、TargetData、WeaponInstance、WeaponStateComponent、输入、伤害或 GameplayTag。
- 新增 GA、GE、Attribute、AbilityTask、RPC、复制变量或网络 Actor。
- 创建、修改、迁移、删除任何 `.uasset` 或 `.umap`。
- 调用 UE MCP；本轮自定义可视装配由用户在代码审查后手工完成。
- 硬编码 RAR/Manny 资产路径、部件名称或 Socket 名。
- 引入 RAR Demo Gameplay Blueprint 依赖。
- 生成 Rider/Visual Studio 解决方案。
- 执行 `git add`、`commit`、`push`。
- 无关重构、格式化或清理用户改动。

## 八、构建与自检

使用项目实际 UE 5.8 路径执行 `ApecoxEditor Win64 Development` 构建。编辑器正在运行且导致 DLL 锁定时，只报告，不强行结束用户进程。

至少检查：

- Public/Private 目录镜像、IWYU、API 宏、前向声明和 UObject GC；
- Actor/组件构造、Spawn、Attach、Destroy 生命周期；
- Blueprint 子类新增组件能够被统一设置 FP/TP 可见性与碰撞；
- Owner 设置能让 `OnlyOwnerSee/OwnerNoSee` 按预期工作；
- Dedicated Server 早退；
- 空 Class、Spawn 失败、未装备、重复刷新和重复销毁；
- Montage、Niagara、Sound 的空配置；
- WorldPickup 仍能取得 Mesh；
- 原有 Fire Cue、弹匣、伤害和网络代码没有被修改。

## 九、中文实施报告

新增：

```text
Agent/Reports/2026-08-21_Phase2B2C_Rifle_Modular_Presentation_Actor_Report.md
```

报告必须包含：

1. 实际修改文件及是否超出 Prompt；
2. 新增类名、父类、文件路径、UCLASS 标记和职责；
3. 每个新增/删除/替换成员变量的名称、类型、UPROPERTY 与作用；
4. 每个新增/修改函数的完整签名、可见性、UFUNCTION 与作用；
5. FP/TP Actor 的 Spawn、Attach、视角设置与 Destroy 生命周期；
6. Standalone、Listen Host、Owning Client、Simulated Proxy、Dedicated Server 路径；
7. Presentation Definition 字段迁移情况；
8. Build.cs 是否变化；
9. 构建命令、结果和警告；
10. 明确未修改的 GA、网络、弹药、伤害和资产范围；
11. 用户后续需要创建的 Blueprint 类别与配置字段，只列类别，不猜最终 Transform；
12. 所有与批准设计或命名不一致之处及理由。

完成后停止，等待 Codex 审查。不要继续创建资产、修改手工清单或进入下一阶段。
