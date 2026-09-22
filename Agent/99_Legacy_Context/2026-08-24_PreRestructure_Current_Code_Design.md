# 当前代码设计（2026-08-24 重构前快照）

更新日期：2026-08-21

## 当前执行设计：Phase 2B-2C 模块化武器表现 Actor

### 1. 调整原因

Phase 2B-2B 已验证 Fire Cue、FP Arms、枪械机械 Montage、枪口 Niagara、声音、连射和命中逻辑能够成立，但 RAR 的 `SK_RAR_AssaultRifle` 只是模块化步枪主体。弹匣、护木、机械瞄具等需要独立组件装配；只在 `UApecoxWeaponPresentationDefinition` 中配置一个 `USkeletalMesh`，既不能表达完整枪械，也无法在编辑器中直观看到装配结果。

本轮把运行时生成的单一 Weapon Mesh Component 替换为可预览的纯表现 Actor。玩法真相、装备实例、弹药、GA、预测和服务器验证全部保持不变。

### 2. 新增表现 Actor

新增：

```text
AApecoxWeaponPresentationActor : AActor
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationActor.h
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp
```

固定 C++ 组件：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `PresentationRoot` | `TObjectPtr<USceneComponent>` | 表现 Actor 根节点，承载 Blueprint 自定义部件 |
| `WeaponMesh` | `TObjectPtr<USkeletalMeshComponent>` | 枪械主体与枪械 Montage 播放目标 |
| `MuzzlePoint` | `TObjectPtr<USceneComponent>` | 显式枪口表现锚点，避免依赖第三方 Socket 命名 |

公开只读访问函数：

```cpp
USkeletalMeshComponent* GetWeaponMesh() const;
USceneComponent* GetMuzzlePoint() const;
```

Actor 只负责可视组件和表现锚点：不复制、不 Tick、无碰撞、不保存弹药、不判断命中、不应用伤害。Blueprint 子类可以在 `WeaponMesh` 下增加 Magazine、Forestock、Iron Sight 等 StaticMeshComponent，并直接在 Viewport 中调节。

### 3. Presentation Definition 调整

`UApecoxWeaponPresentationDefinition` 使用以下类配置替换原有两个武器 Mesh 字段：

```cpp
TSubclassOf<AApecoxWeaponPresentationActor> FirstPersonPresentationActorClass;
TSubclassOf<AApecoxWeaponPresentationActor> ThirdPersonPresentationActorClass;
```

保留：

```text
FirstPersonAttachSocket / FirstPersonAttachTransform
ThirdPersonAttachSocket / ThirdPersonAttachTransform
WorldPickupMesh
FP/TP Character 与 Weapon Fire Montage
MuzzleFlashSystem / FireSound
```

删除不再适用的 `FirstPersonWeaponMesh`、`ThirdPersonWeaponMesh` 和 `MuzzleSocketName`。枪口位置改由表现 Actor 的 `MuzzlePoint` 明确给出；玩法 Trace 起点仍由 Hitscan Fire Config 决定。

### 4. Equipment Component 调整

`UApecoxEquipmentComponent` 的运行时表现引用改为：

```cpp
TObjectPtr<AApecoxWeaponPresentationActor> FirstPersonPresentationActor;
TObjectPtr<AApecoxWeaponPresentationActor> ThirdPersonPresentationActor;
```

`RefreshWeaponPresentation()` 在非 Dedicated Server 上根据 Presentation Definition 本地生成 FP/TP 表现 Actor，设置 Owner 为 Character，分别附着到 FirstPersonMesh 与 ThirdPersonMesh。FP Actor 的所有 PrimitiveComponent 使用 `OnlyOwnerSee + FirstPerson`；TP Actor 使用 `OwnerNoSee + WorldSpaceRepresentation`。这些 Actor 不复制，客户端根据已复制的装备摘要自行重建。

`DestroyWeaponPresentation()` 幂等销毁两个 Actor。`PlayFirePresentation()` 继续使用 `IsLocallyControlled()` 选择唯一视角，通过表现 Actor 的 `WeaponMesh` 播放枪械 Montage，并在 `MuzzlePoint` 生成 Niagara/SFX。

### 5. 编辑器资产口径

代码审查通过后由用户手工创建：

```text
/Game/Blueprints/Weapons/Rifle/BP_Apecox_RiflePresentation_FP
/Game/Blueprints/Weapons/Rifle/BP_Apecox_RiflePresentation_TP
```

父类均为 `AApecoxWeaponPresentationActor`。FP 蓝图组装 RAR 主体、默认弹匣、默认护木、前后机械瞄具和 `MuzzlePoint`；TP 蓝图可以先复用同一套主体和部件，再单独调整世界视角 Transform。

### 6. 明确不改

- 不修改 `UApecoxHitscanFireAbility`、TargetData、PredictionKey、服务器 Trace、伤害、RPM、弹匣和 Shot Confirmation。
- 不新增 GameplayTag、GA、GE、Attribute、AbilityTask、RPC、复制 Actor 或 Tick。
- 不把 RAR Demo Gameplay Blueprint 作为项目运行依赖。
- 不在 C++ 中硬编码第三方资产路径或具体部件。
- 不由子代理创建、修改或迁移 `.uasset`。

### 7. 完成标准

- Development Editor 构建通过。
- 装备/卸下和 RepNotify 可以幂等创建、销毁两个本地表现 Actor。
- Dedicated Server 不生成表现 Actor。
- GameplayCue 仍只播放当前视角，射击逻辑无行为变化。
- 用户能在 Blueprint Viewport 中组装完整 RAR 步枪和显式枪口点。

## 当前执行设计：Phase 2B-2B 步枪双视角腰射表现

### 1. 目标与运行链路

本轮只把已经成立的一次射击事务翻译为可见、可听的双视角表现：

```text
UApecoxHitscanFireAbility
-> ASC ExecuteGameplayCue(GameplayCue.Weapon.Fire)
-> GCN_Weapon_Fire（用户后续创建的 GameplayCueNotify_Burst）
-> Target Character.GetEquipmentComponent()
-> UApecoxEquipmentComponent::PlayFirePresentation()
   |- Owning Player：FP Arms Montage + FP Weapon Montage + FP 枪口 VFX/SFX
   `- Simulated Proxy：TP Character Montage + 可选 TP Weapon Montage + TP 枪口 VFX/SFX
```

`GameplayCue.Weapon.Fire` 已处于现有客户端预测窗口和服务端权威 Cue 路径中。本轮不从动画 Notify 发起射击，也不增加第二套 RPC。

### 2. 职责边界

#### `UApecoxHitscanFireAbility`

保持不变，继续负责本地射击意图、Shot ID、GAS TargetData、服务器校验、权威 Trace、扣弹、伤害、Cue 和 Shot Confirmation。它不知道 RAR、Manny、Montage、Niagara、音效或 Mesh Socket。

#### `UApecoxWeaponPresentationDefinition`

继续作为每种武器的只读表现 DataAsset。新增字段：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `FirstPersonArmsFireMontage` | `TObjectPtr<UAnimMontage>` | Owning Player 的 RAR Arms 腰射 Montage |
| `FirstPersonWeaponFireMontage` | `TObjectPtr<UAnimMontage>` | Owning Player 的 RAR 枪械机械动作 Montage |
| `ThirdPersonCharacterFireMontage` | `TObjectPtr<UAnimMontage>` | Simulated Proxy 的 Manny Fire Montage |
| `ThirdPersonWeaponFireMontage` | `TObjectPtr<UAnimMontage>` | 可选 TP 枪械机械动作；V1 允许为空 |
| `MuzzleSocketName` | `FName` | FP/TP 当前武器 Mesh 上生成枪口表现的 Socket 名 |
| `MuzzleFlashSystem` | `TObjectPtr<UNiagaraSystem>` | 一次性枪口 Niagara |
| `FireSound` | `TObjectPtr<USoundBase>` | 一次性开火声音 |

保留现有 Mesh、Attach Socket/Transform 和 WorldPickup 字段。玩法数值、Trace 起点和命中不进入该 DataAsset。本轮使用直接引用，因为已装备武器的表现资源属于常驻工作集。

#### `UApecoxEquipmentComponent`

继续承担当前 Pawn 的装备生命周期和双视角表现，不新增组件。

新增公开函数：

```cpp
UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment|Presentation")
void PlayFirePresentation();
```

新增内部辅助函数：

```cpp
bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent,
    UAnimMontage* Montage) const;
void PlayMuzzlePresentation(USkeletalMeshComponent* WeaponMesh,
    const UApecoxWeaponPresentationDefinition* Presentation) const;
```

- `PlayMontageOnMesh()` 只处理空指针和 Montage 播放，不选择视角。Character/Arms 通过现有 AnimInstance 播放；独立 Weapon Mesh 没有 AnimBP 时允许使用 UE 的 Single Node `PlayAnimation()` 播放 Montage。
- `PlayMuzzlePresentation()` 只在给定武器 Mesh 的 `MuzzleSocketName` 上生成一次 Niagara 与声音，不决定射击是否成立。
- `PlayFirePresentation()` 是唯一视角分发入口。
- 本轮不为独立 Weapon Mesh 新增 AnimClass 配置。武器机械动画不需要 Locomotion 图，先使用 Single Node 播放；未来出现持续 Weapon AnimGraph 需求时再增加独立设计。

### 3. 第一人称组件层级

`AApecoxPlayerCharacter` 从完整 Manny Copy Pose 结构切换为专用 Arms：

```text
Capsule / RootComponent
|- GetMesh()                       TP Manny，OwnerNoSee
`- FirstPersonCamera              使用 Pawn Control Rotation
   `- FirstPersonMesh             RAR Arms，OnlyOwnerSee
      `- FirstPersonWeaponMesh    EquipmentComponent 运行时创建并附着
```

- Camera 附着 Capsule/Root，不再附着 `FirstPersonMesh.head`。
- FirstPersonMesh 附着 Camera；Mesh、AnimClass 和相对 Transform 仍由 `BP_ApecoxPlayerCharacter` 配置。
- 删除只为 `ABP_FP_Copy` 服务的 Copy Pose 注释和 `AlwaysTickPoseAndRefreshBones` 强制设置。
- 仍只有一个 Character、Capsule 和 CMC，移动与网络预测不变。

### 4. 网络与预测规则

- Standalone / Listen Host 本地 Pawn：只播放 FP。
- Owning Client 本地 Pawn：预测 Fire Cue 到达时只播放 FP。
- 其他客户端的 Simulated Proxy：服务端 Fire Cue 到达时只播放 TP。
- Dedicated Server：不创建或播放视觉资源。

禁止新增 Multicast RPC。现有 GAS PredictionKey/Cue 路径负责预测 Cue 与服务端 Cue 的协调。若实测出现同一 Shot 重复播放，先定位 Cue 预测去重，不能用延时或全局布尔锁掩盖。

### 5. GameplayCue 与动画资产边界

用户后续手工创建：

```text
/Game/Blueprints/GameplayCues/GCN_Weapon_Fire
父类：GameplayCueNotify_Burst
Tag：GameplayCue.Weapon.Fire
```

Cue 只执行 `MyTarget -> AApecoxPlayerCharacter -> GetEquipmentComponent -> PlayFirePresentation`，不在蓝图中重复选择武器资产。

- 不直接使用含厂商 `Update Tags` Notify 的 RAR Montage。
- 后续从 RAR 原始 Animation Sequence 创建 Apecox 自有 FP Arms/Weapon Montage。
- TP Fire 动画从 Rifle Pro 重定向到 Manny 后创建 Apecox 自有 Montage。
- AnimNotify 不决定子弹、扣弹或伤害。

### 6. 修改边界与完成标准

允许修改：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h（仅确有必要时）
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Apecox.Build.cs（仅 Niagara 依赖）
Agent/Reports/...
```

完成标准：Development Editor 构建通过；没有硬编码资产路径；空资源和无 Socket 不崩溃；Dedicated Server 不进入表现路径；现有 Hitscan、预测、伤害和弹匣行为不变。

## 上一阶段设计参考：Phase 2B-1 自动步枪腰射 Hitscan

### 1. 本轮目标

建立第一条真正可玩的射击事务：

```text
IA_WeaponFire
-> InputTag.Weapon.Fire
-> UApecoxHitscanFireAbility（一次激活 = 一发）
-> 客户端采集准星意图、生成 Shot ID、预测开火 Cue
-> GAS TargetData + PredictionKey 发送服务端
-> 服务端验证 WeaponInstance / Shot ID / RPM / Ammo / Aim
-> 服务端两阶段 Trace
-> 权威扣弹、应用 Damage GE、发送确认与 Impact Cue
```

本轮不通过“步枪专属类”实现，而是验证一套可供手枪、步枪、冲锋枪等 Hitscan 武器复用的稳定流程。

### 2. 配置层

新增：

```text
FApecoxWeaponFireConfig
FApecoxHitscanFireConfig : FApecoxWeaponFireConfig
```

`UApecoxWeaponDefinition` 新增：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `FireConfig` | `TInstancedStruct<FApecoxWeaponFireConfig>` | 只允许选择已知射击配置派生结构，不接受任意 Struct/Step |
| `GetFireConfig()` | `const FApecoxWeaponFireConfig*` | 读取公共射击配置 |
| `GetFireConfig<T>()` | 模板访问器 | 安全读取具体配置；类型不匹配返回空 |

`FApecoxWeaponFireConfig`：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `MagazineCapacity` | `int32` | 新建武器实例时的弹匣容量 |
| `RoundsPerMinute` | `float` | 客户端节奏与服务端权威射速门控的共同静态参数 |
| `GameplayFireOriginOffset` | `FVector` | 相对角色权威视角旋转的玩法发射源偏移；不依赖 Dedicated Server 上的视觉枪口 Mesh |

`FApecoxHitscanFireConfig`：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `MaxRange` | `float` | Hitscan 最大距离 |
| `TraceChannel` | `TEnumAsByte<ECollisionChannel>` | 权威世界检测通道 |
| `DamageEffectClass` | `TSubclassOf<UGameplayEffect>` | 命中拥有 ASC 的目标时由服务端应用的 Instant GE |

约束：

- `TInstancedStruct` 是“有基类约束的类型化变体”，不是万能配置字段。
- Lyra 把许多射击字段直接放在 `ULyraRangedWeaponInstance`；Apecox 只借鉴其生命周期和 SourceObject 数据流，配置形态采用自己的 Definition + Inline Typed Config。
- 不加入单次请求结果 Tag、SetByCaller Tag、Damage Type Tag 或武器实例身份 Tag。

### 3. 运行时武器实例

新增：

```text
UApecoxRangedWeaponInstance : UApecoxWeaponInstance
```

关键成员：

| 名称 | 复制 | 作用 |
| --- | --- | --- |
| `CurrentMagazineAmmo` | Owner 可见的运行时复制状态 | 当前弹匣；只由服务端正式扣除 |
| `LastLocalFireTimeSeconds` | 不复制 | Owning Client/Listen Host 的本地射速节奏 |
| `LastServerFireTimeSeconds` | 不复制 | Authority 对该武器的权威射速账本 |
| `NextLocalShotId` | 不复制 | 生成客户端唯一 Shot ID |
| `LastAcceptedShotId` | 不复制 | Authority 防止重复或倒序射击事务重复结算 |

关键函数：

| 名称 | 作用 |
| --- | --- |
| `CanStartLocalShot()` | 根据本地时间、RPM 和已复制弹匣判断是否可以发起预测射击 |
| `StartLocalShot()` | 更新本地射速时间并生成非零 Shot ID；不权威扣弹 |
| `CanCommitServerShot()` | 检查 Shot ID、RPM 和当前弹匣，不修改状态 |
| `CommitServerShot()` | 仅 Authority 调用；原子更新服务器射速账本、Shot ID 账本并扣一发弹药；返回 `bool`，只有 true 才继续伤害和 Cue |
| `GetCurrentMagazineAmmo()` | 提供只读弹匣访问 |

`UApecoxInventoryItemInstance::Initialize()` 改为 virtual；`UApecoxRangedWeaponInstance` override 后调用 `Super`，再根据有效 FireConfig 初始化 Authority 弹匣。客户端通过复制接收弹匣，而不是自行读取 Definition 重建权威状态。

### 4. 武器 GA 层

新增：

```text
UApecoxWeaponGameplayAbility : UApecoxGameplayAbility
UApecoxHitscanFireAbility : UApecoxWeaponGameplayAbility
```

`UApecoxWeaponGameplayAbility` 负责所有武器 Ability 的公共来源校验：

| 名称 | 作用 |
| --- | --- |
| `GetWeaponInstanceFromSpec()` | 通过传入/当前 Spec Handle 找到 AbilitySpec，再从 SourceObject 取得 WeaponInstance |
| `GetRangedWeaponInstanceFromSpec()` | 类型安全取得 RangedWeaponInstance |
| `IsSourceWeaponCurrentlyEquipped()` | 确认 SourceObject 正是当前 EquipmentComponent 装备的实例 |
| `CanActivateAbility()` | 先执行父类检查，再拒绝空 SourceObject、错误类型或已卸下武器 |

禁止在激活前依赖 `GetCurrentAbilitySpec()`；必须使用传入 Handle 通过 ASC 查找 Spec，避免 CDO/尚未建立 Current Spec 上下文时取错数据。

`UApecoxHitscanFireAbility`：

- `ActivationPolicy = WhileInputActive`。
- `NetExecutionPolicy = LocalPredicted`。
- 一次激活只处理一发，完成或拒绝后结束；Held 输入下一帧再尝试激活，WeaponInstance 负责 RPM 门控。
- 注册并在 EndAbility 对称移除 `AbilityTargetDataSetDelegate`，同时消费 Client Replicated TargetData。
- Owning Client 构造 `FApecoxHitscanShotTargetData`，在 Prediction Window 内调用 GAS 原生 `CallServerSetReplicatedTargetData()`。
- Server 不信任客户端 `FHitResult`；服务端只把它作为候选/诊断输入，重新构造权威 Trace。
- 只有 Authority 可以调用 `CommitServerShot()`、应用伤害 GE 和决定正式命中结果。

### 5. TargetData 与两阶段检测

新增：

```text
FApecoxHitscanShotTargetData : FGameplayAbilityTargetData_SingleTargetHit
```

网络字段：

| 名称 | 作用 |
| --- | --- |
| `ShotId` | 一次射击的幂等标识 |
| `ClientFireTimeSeconds` | 为诊断和未来 SSR 保留；V1 不据此回滚世界 |
| `ViewOrigin` | 客户端开枪时的视点，服务端只用于合理性检查 |
| `AimDirection` | 客户端准星方向，服务端与权威 BaseAimRotation 比较后使用 |
| 继承的 `HitResult` | 客户端候选命中；不直接应用伤害 |

实现 `GetScriptStruct()`、`NetSerialize()` 和 `TStructOpsTypeTraits<...>::WithNetSerializer`。读取时必须先检查 ScriptStruct 类型和 TargetData 有效性，再进行结构体转换。

服务端两阶段检测：

1. 使用 Authority Pawn 的 `GetPawnViewLocation()` / `GetBaseAimRotation()` 与客户端 ViewOrigin/AimDirection 做容差检查。
2. 从权威视点沿合法瞄准方向 Trace，得到玩家意图点。
3. 使用 `GameplayFireOriginOffset` 从角色权威 Transform/瞄准旋转构造 Gameplay Fire Origin。
4. 从 Gameplay Fire Origin 朝意图点再次 Trace，处理近墙遮挡并形成最终 `FHitResult`。
5. 合法 Miss 仍扣弹；Rejected 不扣弹；只有服务器最终命中有效 ASC 目标时应用 Damage GE。

### 6. 射击确认组件

新增：

```text
EApecoxShotConfirmation
UApecoxWeaponStateComponent : UControllerComponent
```

枚举值：

```text
Rejected
Miss
ConfirmedHit
```

组件由 `AApecoxPlayerController` 创建并复制，负责：

- Owning Client 记录未确认 Shot ID 与候选命中。
- Server 通过 `ClientConfirmShot(ShotId, Result)` 返回该发的正式结果。
- Client 移除未确认记录并广播/记录确认结果；本轮只提供日志与未来 HUD 委托入口，不绘制命中标记。
- 组件不做 Trace、不扣弹、不应用 GE，也不保存 Weapon Definition。

### 7. GameplayTag

新增 Native Gameplay Tags：

| Tag | 唯一职责 |
| --- | --- |
| `InputTag.Weapon.Fire` | IA 到武器开火 AbilitySpec 的稳定输入意图路由 |
| `GameplayCue.Weapon.Fire` | 开火动画之外的枪口、声音、曳光等表现通道 |
| `GameplayCue.Weapon.Impact` | 世界/角色命中点的粒子、声音、贴花等表现通道 |

这些 Tag 不承担射速限制、弹药状态、伤害类型、命中真假或 Ability 并发关系。

### 8. 修改边界

新增 C++ 文件必须遵守镜像目录：头文件进入 `Source/Apecox/Public`，实现进入 `Source/Apecox/Private`，两侧子目录一致。

本轮允许新增/修改的领域：

```text
AbilitySystem/Abilities/Weapons
AbilitySystem/TargetData
GameplayTags
Inventory/ApecoxInventoryItemInstance
Player/ApecoxPlayerController
Weapons
```

不修改二进制资产，不调用 MCP，不生成解决方案，不引入 ADS/Reload/Spread/Recoil/SSR/Shield/Damage Formula。

### 9. 完成标准

- `ApecoxEditor Win64 Development` 构建通过。
- 长按开火时，每次 GA 激活只结算一发，射速接近配置 RPM，无一帧多发或重复伤害。
- 单人、Listen Host、Listen Client 都由 Authority 唯一扣弹和应用伤害。
- 客户端候选命中与服务器结果通过 Shot ID 配对；重复、倒序、过快、无弹或来源武器已卸下的请求被拒绝且不扣弹。
- Dedicated Server 路径不依赖 FirstPerson/ThirdPerson Weapon Mesh、CameraComponent 渲染状态或本地 PlayerCameraManager。
- 客户端没有权威修改其他玩家 Health；V1 没有历史姿态回滚。

## 上一阶段审阅结论

Phase 2B-1 架构、类名、关键成员和 GameplayTag 已由用户批准并完成验证。
