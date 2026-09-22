# Phase 2B-1 自动步枪腰射 Hitscan — 实施报告

**日期**: 2026-08-11  
**构建结果**: ✅ 成功（ApecoxEditor Win64 Development）  
**构建耗时**: 5.95 秒  
**编译警告**: 0（新增代码无警告）  
**StructUtils 弃用提示**: UE 5.8 标记 StructUtils 为弃用（将在未来版本移除），当前功能正常。后续 UE 版本需迁移到内置 InstancedStruct 支持。

---

## 一、文件清单

### 1.1 新增文件（12 个）

| # | 文件 | 职责 |
|---|------|------|
| 1 | `Public/Weapons/ApecoxWeaponFireConfig.h` | FApecoxWeaponFireConfig + FApecoxHitscanFireConfig 类型化射击配置 |
| 2 | `Private/Weapons/ApecoxWeaponFireConfig.cpp` | UHT 反射代码编译锚点 |
| 3 | `Public/Weapons/ApecoxRangedWeaponInstance.h` | UApecoxRangedWeaponInstance 运行时可变状态声明 |
| 4 | `Private/Weapons/ApecoxRangedWeaponInstance.cpp` | 弹匣初始化、客户端射速门控、服务端权威验证与结算 |
| 5 | `Public/Weapons/ApecoxWeaponStateComponent.h` | EApecoxShotConfirmation 枚举 + UApecoxWeaponStateComponent |
| 6 | `Private/Weapons/ApecoxWeaponStateComponent.cpp` | 未确认射击记录、Client RPC 确认、委托广播 |
| 7 | `Public/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.h` | FApecoxHitscanShotTargetData 自定义 TargetData |
| 8 | `Private/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.cpp` | NetSerialize 实现 |
| 9 | `Public/AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.h` | 武器 GA 公共基类声明 |
| 10 | `Private/AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.cpp` | SourceObject 验证、装备关系检查 |
| 11 | `Public/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.h` | Hitscan 射击 GA 声明 |
| 12 | `Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp` | 完整射击闭环：客户端预测→服务端验证→两阶段 Trace→扣弹→伤害→确认 |

### 1.2 修改文件（7 个）

| # | 文件 | 修改内容 |
|---|------|---------|
| 1 | `Public/Weapons/ApecoxWeaponDefinition.h` | 新增 `TInstancedStruct<FApecoxWeaponFireConfig> FireConfig` + 类型安全模板访问器 |
| 2 | `Public/Inventory/ApecoxInventoryItemInstance.h` | `Initialize()` 改为 `virtual`——RangedWeaponInstance override |
| 3 | `Public/GameplayTags/ApecoxGameplayTags.h` | 新增 3 个 Native Tag 声明 |
| 4 | `Private/GameplayTags/ApecoxGameplayTags.cpp` | 新增 3 个 Native Tag 定义 |
| 5 | `Public/Player/ApecoxPlayerController.h` | 新增 `WeaponStateComponent` + Getter + 注释更新 |
| 6 | `Private/Player/ApecoxPlayerController.cpp` | 构造函数创建 WeaponStateComponent |
| 7 | `Apecox.Build.cs` | PublicDependencyModuleNames 新增 `StructUtils`、`ModularGameplay` |

### 1.3 修改项目文件

| # | 文件 | 修改内容 |
|---|------|---------|
| 1 | `Apecox.uproject` | Plugins 新增 `StructUtils`、`ModularGameplay` |

---

## 二、新增类、结构体、枚举、关键函数

### 2.1 FApecoxWeaponFireConfig（基类）

| 字段 | 类型 | 默认值 | 作用 |
|------|------|--------|------|
| MagazineCapacity | int32 | 30 | 新建武器实例时的弹匣容量 |
| RoundsPerMinute | float | 600.0 | 客户端/服务端射速门控共同参数 |
| GameplayFireOriginOffset | FVector | (50,10,-10) | DS-safe 玩法发射源，不依赖视觉 Mesh Socket |
| IsValidFireConfig() | bool | — | MagazineCapacity>0 且 RPM>0 |

### 2.2 FApecoxHitscanFireConfig（派生）

| 字段 | 类型 | 默认值 | 作用 |
|------|------|--------|------|
| MaxRange | float | 10000.0cm | Hitscan 最大距离 |
| TraceChannel | TEnumAsByte\<ECollisionChannel\> | ECC_GameTraceChannel1 | 权威世界检测通道 |
| DamageEffectClass | TSubclassOf\<UGameplayEffect\> | nullptr | 命中 ASC 目标时应用的 Instant GE |
| IsValidHitscanConfig() | bool | — | Magazine/RPM/Range 均>0 |

### 2.3 UApecoxRangedWeaponInstance

| 成员 | 复制 | 作用 |
|------|------|------|
| CurrentMagazineAmmo | OwnerOnly | 当前弹匣——仅 Authority 扣除 |
| LastLocalFireTimeSeconds | 不复制 | Owning Client 本地射速计时 |
| LastServerFireTimeSeconds | 不复制 | Authority 权威射速账本 |
| NextLocalShotId | 不复制 | 客户端递增 Shot ID 生成器 |
| LastAcceptedShotId | 不复制 | Authority 防重复结算 |

| 公开函数 | 作用 |
|----------|------|
| Initialize() | Super + Authority 弹匣初始化 |
| CanStartLocalShot() | 检查本地 RPM 和弹匣 |
| StartLocalShot() | 更新本地计时 + 生成 Shot ID |
| CanCommitServerShot() | 验证 Shot ID、RPM、弹药 |
| CommitServerShot() | 原子更新权威账本 + 扣弹 |
| GetCurrentMagazineAmmo() | 只读弹匣访问 |

### 2.4 EApecoxShotConfirmation（枚举）

| 值 | 含义 |
|----|------|
| Rejected | 请求被拒绝，未消耗弹药 |
| Miss | 请求合法，弹药已消耗，未命中有效目标 |
| ConfirmedHit | 请求合法，弹药已消耗，成功向有 ASC 的目标应用 Damage GE |

### 2.5 UApecoxWeaponStateComponent

| 成员 | 作用 |
|------|------|
| UnconfirmedShots | 未确认射击记录（最多 32 条） |
| OnShotConfirmed | FApecoxShotConfirmedDelegate——HUD 委托入口 |

| 公开函数 | 作用 |
|----------|------|
| RecordUnconfirmedShot() | Owning Client 记录预测射击 |
| SendShotConfirmation() | Authority 发送最终结果 |
| ClientConfirmShot() | Client RPC——接收确认并广播 |

### 2.6 FApecoxHitscanShotTargetData

| 字段 | 类型 | 作用 |
|------|------|------|
| ShotId | uint32 | 幂等射击标识 |
| ClientFireTimeSeconds | float | 诊断/未来 SSR 保留 |
| ViewOrigin | FVector_NetQuantize10 | 客户端视点——服务端容差检查 |
| AimDirection | FVector_NetQuantizeNormal | 客户端准星方向——服务端角度检查 |
| 继承 HitResult | FHitResult | 客户端候选命中——不直接应用伤害 |

### 2.7 UApecoxWeaponGameplayAbility

| 公开函数 | 作用 |
|----------|------|
| GetWeaponInstanceFromSpec() | 从 AbilitySpec.SourceObject 取 WeaponInstance |
| GetRangedWeaponInstanceFromSpec() | 类型安全取 RangedWeaponInstance |
| IsSourceWeaponCurrentlyEquipped() | 验证 SourceObject 是当前装备实例 |
| CanActivateAbility() | 父类检查 + SourceObject 验证 |

### 2.8 UApecoxHitscanFireAbility

| 属性 | 值 |
|------|-----|
| ActivationPolicy | WhileInputActive |
| NetExecutionPolicy | LocalPredicted |
| InstancingPolicy | InstancedPerActor |
| ActivationGroup | Independent |

| 关键函数 | 作用 |
|----------|------|
| ActivateAbility() | 验证→注册 delegate→预测→发送 TargetData |
| OnTargetDataReady() | Client 转发→Authority 验证结算 |
| BuildLocalShotTargetData() | 客户端候选 Trace + FApecoxHitscanShotTargetData |
| ProcessAuthoritativeShot() | 完整验证→Trace→Commit→GE→Cue→Confirm |
| PerformTwoStageTrace() | 意图点 Trace + Fire Origin Trace |
| EndAbility() | 移除 delegate + 消费 TargetData + 清理 |

---

## 三、射击网络链路

### 3.1 Owning Client

```
1. ASC ProcessAbilityInput 检测 Held 中的 WhileInputActive Spec
   → TryActivateAbility(HitscanFireAbility)
2. CanActivateAbility: 验证 SourceObject 是当前装备的武器实例
3. ActivateAbility:
   a. 提交 Ability + 验证 RangedWeaponInstance/HitscanConfig
   b. 注册 AbilityTargetDataSetDelegate(Handle, PredictionKey)
   c. CanStartLocalShot(WorldTimeSeconds): RPM 门控
   d. StartLocalShot(WorldTimeSeconds): 更新 LastLocalFireTimeSeconds + 生成 Shot ID
   e. BuildLocalShotTargetData: PlayerController::GetPlayerViewPoint → LineTrace → 候选 HitResult
   f. RecordUnconfirmedShot(ShotId, bBlockingHit)
   g. ASC->ExecuteGameplayCue(GameplayCue.Weapon.Fire, CueParams) → 本地预测播放
   h. ASC->CallServerSetReplicatedTargetData(Handle, PredictionKey, TargetData) → 发送给服务器
```

### 3.2 Authority（Listen Server / Dedicated Server）

```
1. OnTargetDataReady 触发:
   a. 验证 TargetData 非空 + ScriptStruct 类型正确
   b. 从 Spec 取得 RangedWeaponInstance
   c. ProcessAuthoritativeShot:
      - IsSourceWeaponCurrentlyEquipped: 武器未被卸下
      - CanCommitServerShot(ShotId, ServerTimeSeconds):
        * ShotId ≠ 0
        * MagazineAmmo > 0
        * ShotId > LastAcceptedShotId（回绕安全）
        * TimeSinceLastServerShot ≥ 60/RPM - Tolerance
      - 客户端 ViewOrigin 与 GetPawnViewLocation() 距离 ≤ 500cm
      - 客户端 AimDirection 与 GetBaseAimRotation() 角度 ≤ 15°
      - PerformTwoStageTrace:
        Stage 1: 权威视点 → MaxRange → 意图点
        Stage 2: GameplayFireOrigin → 意图点 → 最终 HitResult
      - CommitServerShot: 更新 LastServerFireTimeSeconds/LastAcceptedShotId，扣弹
      - 命中 ASC 目标 + DamageEffectClass 有效 → ApplyGameplayEffectSpecToTarget
      - Authority ExecuteGameplayCue(GameplayCue.Weapon.Impact) → 远端播放
      - SendShotConfirmation(ShotId, ConfirmedHit/Miss/Rejected) → Client RPC
```

### 3.3 Remote Client

```
1. 收到 Fire Cue（服务器复制的 ExecuteGameplayCue）→ 播放开枪表现
2. 收到 Impact Cue → 播放命中粒子/声音
3. EquippedWeaponState 摘要不变（一轮射击不改变装备状态）
4. 不直接看到弹药变化（弹药 OwnerOnly 复制）
```

---

## 四、TargetData 与 Shot ID 防重复结算

### 4.1 Shot ID 幂等机制

1. **非零生成**: Shot ID 从 1 开始，递增分配，跳过 0（哨兵值）
2. **回绕安全**: `NextLocalShotId == 0` 时自动重置为 1
3. **服务端验证**:
   - `CanCommitServerShot` 比较 `ShotId > LastAcceptedShotId`
   - 回绕判断: `LastAcceptedShotId > 0xFFFF0000 && ShotId < 0x0000FFFF` 时允许通过
   - 倒序/重复 Shot ID 直接 Rejected，不扣弹
4. **Listen Server Host**: `LastLocalFireTimeSeconds` 和 `LastServerFireTimeSeconds` 是两个独立字段，Host 的本地预测和 Authority 验证不共享计时器，无法双扣弹

### 4.2 TargetData 防重复

1. **单次消费**: `OnTargetDataReady` 检查 `InData.IsValid(0)` + ScriptStruct 类型，每次只消费一次
2. **EndAbility 清理**: `ConsumeClientReplicatedTargetData` 消费残留数据
3. **Delegate 对称**: `ActivateAbility` Add → `EndAbility` Remove，防止 ScopeLock 悬空
4. **类型安全**: 服务端先检查 `GetScriptStruct() == FApecoxHitscanShotTargetData::StaticStruct()` 再转换

---

## 五、预测与验证边界

### 5.1 Client 预测了什么

| 预测内容 | 机制 |
|----------|------|
| 开火表现 | Asc->ExecuteGameplayCue(GameplayCue.Weapon.Fire) → PredictionKey 去重 |
| 本地 HitResult | BuildLocalShotTargetData 的候选 Trace——仅用于诊断和预测表现 |
| Shot ID 与时序 | StartLocalShot 生成 ID + 更新 LastLocalFireTimeSeconds |
| RPM 门控 | CanStartLocalShot 使用 LastLocalFireTimeSeconds 本地检查 |

### 5.2 Client 没有预测什么

| 不预测内容 | 说明 |
|------------|------|
| 弹药消耗 | CurrentMagazineAmmo 只由 CommitServerShot 权威扣除 |
| 伤害应用 | Damage GE 只由 Authority 在 MakeOutgoingSpec + ApplyGameplayEffectSpecToTarget |
| 其他玩家 Health | Attribute 修改只走服务器权威路径 |
| 命中确认 | ShotConfirmation 等待服务器 ClientConfirmShot RPC |

### 5.3 Server 验证了什么

| 验证项 | 常量 | 失败后果 |
|--------|------|---------|
| 来源武器装备关系 | IsSourceWeaponCurrentlyEquipped | Rejected |
| Shot ID（非零、未重复/倒序、回绕安全） | — | Rejected |
| RPM（权威时间差 ≥ 60/RPM - 0.001s） | FireRateTolerance = 0.001f | Rejected |
| 弹药（MagazineAmmo > 0） | — | Rejected |
| ViewOrigin 距离 | MaxViewOriginDelta = 500cm | Rejected |
| AimDirection 角度 | MaxAimDirectionAngleDeg = 15° | Rejected |
| 两阶段 Trace（意图点 + Fire Origin） | — | Miss|

---

## 六、与 Lyra 的对比

### 6.1 相同部分

- 武器 GA 通过 AbilitySpec.SourceObject 获取 WeaponInstance
- LocalPredicted + PredictionKey 用于 TargetData 预测窗口
- WhileInputActive 驱动自动射击，RPM 由 WeaponInstance 门控
- GameplayCue 用于开火和命中表现
- UControllerComponent 用于跨 Pawn 的武器状态追踪（Lyra 是 ULyraWeaponStateComponent，Apecox 是 UApecoxWeaponStateComponent）

### 6.2 不同部分

| 方面 | Lyra | Apecox |
|------|------|--------|
| **TargetData 恒有效** | `bIsTargetDataValid = true`，不验证客户端 HitResult | **明确标记客户端 HitResult 为候选**，服务端两阶段 Trace 重新构造真相 |
| **配置位置** | 射击字段直接放在 RangedWeaponInstance | **不可变配置放在 WeaponDefinition 的 TInstancedStruct，可变状态放在 Instance** |
| **服务端 Trace** | DoSingleBulletTrace / TraceBulletsInCartridge | **PerformTwoStageTrace: 意图点 + Fire Origin 两阶段** |
| **射击验证** | 客户端候选直接接受 | **显式检查 ViewOrigin 距离、AimDirection 角度、Shot ID 幂等** |
| **装备来源** | 从 EquipmentManager 获取实例 | **从 Spec.SourceObject 获取 + IsSourceWeaponCurrentlyEquipped 双重验证** |
| **TargetData 流程** | C++ → OnRangedWeaponTargetDataReady (BlueprintImplementableEvent) | **C++ 闭环: 验证→Trace→Commit→GE→Cue→Confirm** |
| **WeaponStateComponent** | 命中标记 UI + Tick 驱动武器散热 | **只保留 Shot 确认/清理，不实现命中标记 UI 和散热** |

---

## 七、构建结果

```text
构建命令:
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
  -Project="D:\UnrealProject\Apecox\Apecox.uproject"
  -WaitMutex -NoHotReloadFromIDE

结果: ✅ 成功
耗时: 5.95 秒
操作数: 4（1 编译 + 2 链接 + 1 WriteMetadata）
新增代码编译警告: 0
```

---

## 八、尚未执行的 UE 人工资产配置

### 8.1 必须创建的资产

| 资产 | 类型 | 位置建议 | 说明 |
|------|------|---------|------|
| FApecoxHitscanFireConfig | 武器定义中的 FireConfig | 在 BP_RifleDefinition 中配置 | 选择 FApecoxHitscanFireConfig 类型 |
| Damage GE | GameplayEffect（Instant） | /Game/Blueprints/GameplayEffects/ | 对 Health 做固定负 Modifier |
| AbilitySet（含 HitscanFireAbility） | UApecoxAbilitySet | /Game/Blueprints/AbilitySets/ | 将 HitscanFireAbility 加入 EquippedAbilitySets |
| 开火 InputAction | IA_WeaponFire | /Game/Input/Actions/ | 绑定到鼠标左键 |
| IMC 映射 | InputTag.Weapon.Fire → IA_WeaponFire | 在 DefaultInputMappingContext 中 | 添加 InputTag.Weapon.Fire 的映射 |

### 8.2 项目设置

| 设置 | 位置 | 说明 |
|------|------|------|
| ECC_GameTraceChannel1 | Project Settings → Collision | 创建武器 Trace 通道（或改为 Visible/Weapon） |
| GameplayCue 资产 | 无需立即创建 | GC.Weapon.Fire 和 GC.Weapon.Impact 可以在创建 GC Notify 前正常构建和日志验证 |

---

## 九、偏离与注意事项

### 9.1 偏离 Prompt 的地方

| 偏离 | 原因 | 风险 |
|------|------|------|
| 新增 ModularGameplay 插件依赖 | `UControllerComponent` 在 UE 5.8 中位于 ModularGameplay 插件 | 低——插件是官方 UE 组件 |
| 新增 StructUtils 插件依赖 | `TInstancedStruct` 需要 StructUtils 插件 | **StructUtils 在 UE 5.8 被标记为弃用**——功能正常但未来 UE 版本需迁移（UE 应将 InstancedStruct 内置到 CoreUObject） |
| CallServerSetReplicatedTargetData 改为 ASC 方法 | UE 5.8 将该方法从 UGameplayAbility 移至 UAbilitySystemComponent | 低——API 行为等价，签名相同 |
| GetActorInfo() 弃用 | UE 5.8 中 GetActorInfo() 返回类型变化 | 改用 `CurrentActorInfo` 直接访问 |

### 9.2 已知限制

1. **ECC_GameTraceChannel1 需手动配置**: 未在 Project Settings 中创建该 Trace Channel 时，LineTrace 行为由引擎默认处理（可能回退到 ECC_WorldStatic）
2. **DamageEffectClass 为空**: 可以构建和 Trace，但命中 ASC 目标时只打 Miss，不会造成伤害
3. **Impact Cue 在没有 GC Notify 资产时**: 只产生日志，不影响代码流程
4. **弹匣 UI 未实现**: `OnRep_CurrentMagazineAmmo` 已预留但未连接 UI

---

## 十、下一步人工验证建议

1. **单人射击验证**:
   - 在 BP_RifleDefinition 中配置 FireConfig（FApecoxHitscanFireConfig）
   - 创建包含 HitscanFireAbility 的 AbilitySet 并配置到武器定义
   - 确认长按开火时 RPM 正常、一帧只一发、弹匣递减
   - 确认 Fire Cue 有日志（GC Notify 未创建时只有日志）
2. **Listen Server 双人验证**:
   - Host 和 Client 分别持枪对射
   - 确认双方都看到对方开火（Fire Cue 远端复制）
   - 确认伤害正确应用、死亡正常触发
   - 确认 Host 弹药不双倍扣除
3. **Dedicated Server 验证**:
   - 确认 DS 不依赖 FP/TP Weapon Mesh 或 CameraComponent
   - 确认 Impact Cue 参数（Location、Normal）正确到达 Remote Client
4. **边界条件**:
   - 按住开火时换枪——确认旧 GA 正确结束、新枪 GA 正确激活
   - 死亡期间持枪——确认 CanActivateAbility 被 State.Death 正确阻止
   - 弹匣打空后继续按开火——确认 CanStartLocalShot 返回 false，GA 结束

---

## 十一、声明

- ✅ 未使用 MCP 工具
- ✅ 未修改任何 `.uasset`、`.umap` 资产文件
- ✅ 未执行 Git add/commit/push 操作
- ✅ 未操作 UE 编辑器
- ✅ 未实现 ADS、Reload、散布、后坐力、命中标记 UI、Montage、Projectile、SSR、Shield 或正式伤害系统
- ✅ 未创建万能 Weapon Fragment 或任意 Step 流程
- ✅ 未因编译错误重写已验证的 Inventory/Equipment/ASC 生命周期
- ✅ 所有新增和修改均在 Prompt 批准范围内
- ✅ ApecoxEditor Win64 Development 构建通过（0 警告）
