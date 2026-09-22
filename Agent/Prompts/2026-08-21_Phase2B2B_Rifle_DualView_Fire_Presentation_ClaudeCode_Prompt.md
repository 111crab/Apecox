# Claude Code 实施 Prompt：Phase 2B-2B 步枪双视角腰射表现

你是 Apecox 项目的具体实施子代理。本窗口可能没有此前上下文，必须以本文和列出的项目文档为准。Codex 负责高层设计与代码审查，用户负责公开命名审阅和 UE 编辑器人工配置；你的职责是严格实施已经批准的 C++，完成构建并提交中文报告。

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

然后阅读下列现有代码及必要依赖：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Weapons/ApecoxWeaponDefinition.h
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp
Source/Apecox/Apecox.Build.cs
```

先执行 `git status --short`，理解当前工作区可能包含用户和此前阶段的未提交改动。不得恢复、覆盖或清理不属于本任务的改动。

## 二、本轮目标

为现有 `GameplayCue.Weapon.Fire` 增加可由 GameplayCue Blueprint 调用的双视角表现入口：

```text
Owning Player：FP Arms Fire Montage + FP Weapon Fire Montage + FP Muzzle VFX/SFX
Simulated Proxy：TP Character Fire Montage + 可选 TP Weapon Fire Montage + TP Muzzle VFX/SFX
Dedicated Server：无视觉操作
```

同时把 `AApecoxPlayerCharacter` 的第一人称层级从完整 Manny Copy Pose 结构调整为专用 Arms 结构：Camera 附着 Root/Capsule，FirstPersonMesh 附着 Camera。

现有 Hitscan GA、TargetData、PredictionKey、服务器 Trace、伤害、弹匣与 Shot Confirmation 已通过验证，不得重构或改写。

## 三、批准的公开配置

在 `UApecoxWeaponPresentationDefinition` 中保留已有字段并新增：

```cpp
TObjectPtr<UAnimMontage> FirstPersonArmsFireMontage;
TObjectPtr<UAnimMontage> FirstPersonWeaponFireMontage;

TObjectPtr<UAnimMontage> ThirdPersonCharacterFireMontage;
TObjectPtr<UAnimMontage> ThirdPersonWeaponFireMontage;

FName MuzzleSocketName;
TObjectPtr<UNiagaraSystem> MuzzleFlashSystem;
TObjectPtr<USoundBase> FireSound;
```

要求：

- 使用合适的 `EditDefaultsOnly, BlueprintReadOnly` 与清晰分类。
- 给出简洁中文注释，既说明字段做什么，也说明玩法 Trace 不依赖视觉 Socket。
- `ThirdPersonWeaponFireMontage`、Niagara、Sound 允许为空。
- `MuzzleSocketName` 给出合理默认值，但不得假设资产一定存在该 Socket。
- 不添加 Reload、ADS、Recoil、Spread、Impact、Shell 或 CameraShake 字段。
- 不改成万能数组、Step 或 `FInstancedStruct`。

## 四、EquipmentComponent 实施

### 4.1 新增公开入口

在 `UApecoxEquipmentComponent` 新增：

```cpp
UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment|Presentation")
void PlayFirePresentation();
```

它由后续用户手工创建的 `GCN_Weapon_Fire` 调用。

### 4.2 新增内部辅助

按当前代码风格实现：

```cpp
bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent,
    UAnimMontage* Montage) const;

void PlayMuzzlePresentation(USkeletalMeshComponent* WeaponMesh,
    const UApecoxWeaponPresentationDefinition* Presentation) const;
```

可以为 const-correctness、返回值或 UE API 需要做最小调整，但不得改名或扩大职责；若调整，报告中必须解释。

### 4.3 视角分流

`PlayFirePresentation()`：

1. Dedicated Server 立即返回。
2. 验证 Owner Character、当前 WeaponDefinition 和 PresentationDefinition。
3. 通过 Pawn/Character 的 `IsLocallyControlled()` 判断本机表现通道。
4. 本地控制 Pawn：在 Character `GetFirstPersonMesh()` 播放 `FirstPersonArmsFireMontage`；在 `FirstPersonWeaponMeshComponent` 播放 `FirstPersonWeaponFireMontage`；在 FP Weapon Mesh 枪口播放 VFX/SFX。
5. 非本地控制 Pawn：在 Character `GetMesh()` 播放 `ThirdPersonCharacterFireMontage`；若配置则在 `ThirdPersonWeaponMeshComponent` 播放 `ThirdPersonWeaponFireMontage`；在 TP Weapon Mesh 枪口播放 VFX/SFX。

禁止同时播放 FP 和 TP 两条路径。禁止新增 RPC、Timer、全局去重布尔量或每 Tick 逻辑。

### 4.4 Mesh 创建

扩展 `RefreshWeaponPresentation()`：

- 不新增 Weapon AnimClass 字段。独立枪械 Mesh 当前没有 Locomotion AnimGraph，机械动作允许使用 Single Node 播放。
- 沿用现有 `OnlyOwnerSee`、`OwnerNoSee`、Collision 和 FirstPersonPrimitiveType。
- 不改变 `EquippedWeaponState`、CurrentWeaponInstance 或 AbilitySet 生命周期。

### 4.5 Montage、VFX、SFX 安全性

- Character 与 Arms Montage 通过其现有 `UAnimInstance::Montage_Play()` 播放，绝不能把 Character/Arms 从 Animation Blueprint 模式切成 Single Node。
- 动态创建的 FP/TP Weapon Mesh 若没有 AnimInstance，允许使用 `USkeletalMeshComponent::PlayAnimation(Montage, false)`。UE 5.8 的 `UAnimSingleNodeInstance` 支持 Montage；该回退只允许用于两个 Weapon Mesh Component。
- 空 Mesh、空 Montage或不满足上述两种路径时返回 false，不崩溃。
- 不使用 `LoadSynchronous()`，不硬编码资产路径。
- Niagara 使用附着到当前视角 Weapon Mesh 的一次性生成 API。
- Sound 使用引擎现有附着或位置播放 API，不增加持久音频组件。
- Socket 为空或不存在时不得崩溃；输出有帮助但不过量的日志，并选择明确的安全行为。
- VFX/SFX 只是表现，绝不影响一发射击的合法性、扣弹和伤害。

## 五、PlayerCharacter 组件层级

修改构造函数：

```text
Root/Capsule
|- ThirdPersonMesh
`- FirstPersonCamera
   `- FirstPersonMesh
```

要求：

- `FirstPersonCamera` 不再附着 `FirstPersonMesh` 的 `head` Socket。
- `FirstPersonMesh` 不再附着 ThirdPersonMesh，改为附着 FirstPersonCamera。
- 保留 `OnlyOwnerSee`、NoCollision 和 FirstPerson primitive 设置。
- 保留 Camera 的 Pawn Control Rotation、FP FOV 与 FP Scale 设置。
- Camera 使用 Capsule/Root 下的稳定眼高初始位置；不要硬编码 RAR 资产路径或 RAR 专属 Arms Transform。
- 删除已经失效的 `ABP_FP_Copy` / Copy Pose 注释和只为隐藏源 Mesh 刷新骨骼的强制设置。
- 不改变 CMC、输入、ASC、死亡、重生、拾取或网络移动逻辑。

精确 Arms 相对 Transform 将由用户在 `BP_ApecoxPlayerCharacter` 中配置。

## 六、Build.cs

若使用 Niagara C++ API，在 `Apecox.Build.cs` 增加最小 `Niagara` 模块依赖。不要顺手增加无关模块。

## 七、明确禁止

- 不创建、修改、迁移或保存任何 `.uasset`、`.umap`。
- 不调用 UE MCP；本轮自定义动画和 GameplayCue 配置用 MCP 收益低。
- 不修改 `UApecoxHitscanFireAbility`、TargetData、WeaponStateComponent 或射击 Tag。
- 不新增 GameplayTag、GA、GE、Attribute、AbilityTask、RPC 或 ActorComponent。
- 不使用厂商 `AM_RAR_FP_PCH_AssaultRifle_Fire`、`Update Tags` Notify 或 GameplayCore Blueprint。
- 不生成解决方案；用户自行处理 Rider 工程索引。
- 不执行 Git add、commit、push。
- 不做无关重构或格式化。

## 八、代码注释要求

中文注释必须帮助用户理解设计原因，重点解释：

- 为什么 Fire Cue 只触发表现，射击真相仍在 GA/服务器；
- 为什么通过本地控制关系选择 FP/TP，而不是复制两个动画状态；
- 为什么玩法发射源不使用视觉 Muzzle Socket；
- 为什么 Dedicated Server 跳过所有视觉资源；
- 为什么当前先留在 EquipmentComponent，以及未来何时才值得拆分表现组件。

避免逐行复述代码。

## 九、构建与自检

使用项目实际 UE 5.8 路径执行 Development Editor 构建。若编辑器正在运行导致 DLL 锁定，明确报告，不要强行终止用户进程。

至少检查：

- 头文件 IWYU 和前向声明；
- UObject、AnimMontage、Niagara、Sound 的 GC 引用；
- Dynamic Weapon Mesh 的 Single Node Montage 播放不得改变 Character/Arms 的 Animation Blueprint 模式；
- `IsLocallyControlled()` 在 Standalone、Listen Host、Owning Client、Simulated Proxy 上的分流；
- 空配置和未装备状态；
- Dedicated Server 路径；
- 现有装备、卸下幂等性未被破坏。

## 十、中文实施报告

新增：

```text
Agent/Reports/2026-08-21_Phase2B2B_Rifle_DualView_Fire_Presentation_Report.md
```

报告必须包含：

1. 实际修改文件，是否超出 Prompt；
2. 修改类名、父类与职责；
3. 每个新增成员变量的名称、类型、UPROPERTY 和作用；
4. 每个新增函数的完整签名、可见性、UFUNCTION 和作用；
5. PlayerCharacter 组件层级修改前后；
6. Standalone、Listen Host、Owning Client、Simulated Proxy、Dedicated Server 的表现路径；
7. Build.cs 变化；
8. 构建命令、结果与警告；
9. 未修改的 GA、网络、伤害和资产范围；
10. 后续必须由用户完成的 UE 配置，只列类别，不自行猜测最终步骤；
11. 所有与批准命名或设计不一致之处及理由。

完成后停止，等待 Codex 审查。不要继续创建资产或进入下一阶段。
