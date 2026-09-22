# Claude Code 修复 Prompt：Phase 2B-1 自动步枪腰射 Hitscan

## 任务边界

修复首轮实现，不新增玩法功能。开始前必须阅读：

```text
D:/UnrealProject/Apecox/Agent/Prompts/2026-08-11_Phase2B1_Hitscan_Fire_ClaudeCode_Prompt.md
D:/UnrealProject/Apecox/Agent/Reports/2026-08-11_Phase2B1_Hitscan_Fire_Report.md
D:/UnrealProject/Apecox/Agent/Reviews/2026-08-11_Phase2B1_Hitscan_Fire_Review.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Phase.md
D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md
```

保持已批准的类、结构体和 GameplayTag；不要实现 ADS、Reload、Spread、Recoil、UI、Montage、Projectile、SSR、Shield 或正式伤害公式。不要操作 UE MCP、`.uasset/.umap`、项目文件生成、Git。

## 必须修复的 6 个阻断项

### 1. 让 Owning Client 获得精确的当前装备实例

修改 `UApecoxEquipmentComponent`：

- `CurrentWeaponInstance` 改为 Owner-only 复制的私有装备身份引用。
- 在 `GetLifetimeReplicatedProps()` 中使用 `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)`。
- Equip/Unequip 改值时按项目现有 PushModel 习惯标记 dirty（如果属性未启用 PushModel，也要保证普通复制成立）。
- 更新注释：它是 Server + Owner 的精确装备身份，不是远端表现真相；Simulated Proxy 仍只看 `EquippedWeaponState`。
- 不允许把整个私有库存或弹匣复制给其他玩家。

目标：Owning Client 的 `IsSourceWeaponCurrentlyEquipped()` 能比较同一个 replicated WeaponInstance，而 Authority 校验保持不变。

### 2. 修复 TargetData 内存所有权

`FGameplayAbilityTargetDataHandle::Add()` 的参数必须由 `new` 创建。

禁止：

```cpp
TargetDataHandle.Add(&ShotData);
```

改为由 Handle 接管堆分配的 `FApecoxHitscanShotTargetData`，后续不得保存指向局部栈变量的 TargetData 指针。

### 3. 重构“一次激活 = 一发”的结束与 RPM 门控

为 `UApecoxHitscanFireAbility` 增加自己的 `CanActivateAbility()` override：

- 先调用 `UApecoxWeaponGameplayAbility::CanActivateAbility()`。
- 对 `ActorInfo->IsLocallyControlled()` 的路径，使用当前 RangedWeaponInstance 的 `CanStartLocalShot(WorldTime)` 做 RPM/弹匣门控。
- 这样 ASC Held 每帧可以检查，但只有到达射速间隔才真正预测激活，不得每帧发送一次 Ability 激活请求。
- Remote Authority 在尚未收到 ShotId 前不要用本地门控替代服务端 `CanCommitServerShot()`。

重构 `ActivateAbility()/OnTargetDataReady()/EndAbility()`：

1. `ActivateAbility()` 不再一进入就无条件 `CommitAbility()`。
2. 先验证配置、注册 `AbilityTargetDataSetDelegate`。
3. Remote Authority 绑定后调用 `CallReplicatedTargetDataDelegatesIfSet()` 并等待数据。
4. Local Player 生成 ShotId 和堆分配 TargetDataHandle，然后显式走统一 `OnTargetDataReady()`，不要另写一套直发后永不回调的路径。
5. `OnTargetDataReady()` 使用 `FScopedPredictionWindow`：
   - 普通 Owning Client：本地 `CommitAbility()` 成功后记录未确认射击、预测 Fire Cue、调用 `CallServerSetReplicatedTargetData()`，随后正常结束该发 Ability。
   - Authority（包含 Listen Host）：验证 TargetData，服务器 `CommitAbility()`，执行权威射击事务，随后结束该发 Ability。
6. 所有 invalid type、missing source、rejected、commit failure、world/actor invalid 分支都必须最终结束 Ability，并通过现有 `EndAbility()` 对称移除 delegate、消费 replicated TargetData、清理 `CurrentShotId`。
7. `EndAbility()` 遵守 `IsEndAbilityValid()` 和 `ScopeLockCount/WaitingToExecute` 的 GAS 清理模式，不能在 delegate 广播或 ScopeLock 中留下悬空回调。
8. Client 必须先发送 TargetData，再复制结束；Authority 必须先完成或拒绝事务，再结束。

完成后长按输入应表现为：Ability 在一发结束后变为 inactive，达到下一 RPM 时间点时由 Held 再次激活。

### 4. 通过 AbilitySystemInterface 获取目标 ASC

删除：

```cpp
HitActor->FindComponentByClass<UAbilitySystemComponent>()
```

使用：

```text
UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor)
```

或语义完全等价的 `IAbilitySystemInterface` 路径。必须支持 Apecox 的 PlayerState ASC / Character Avatar 架构。

同时：

- Damage EffectContext 增加 `RangedWeaponInstance` 作为 SourceObject，保留武器伤害来源。
- 只有 TargetASC 和有效 Spec 都存在时才标记 `ConfirmedHit`。
- `DamageEffectClass` 为空时保持合法 Miss；若配置了非 Instant GE，记录明确 Warning 并拒绝作为本轮固定伤害应用，避免无跟踪 Active GE。

### 5. 修复两阶段 Trace 的最终命中语义

- Stage 1：Authority View -> MaxRange，只计算 `IntentPoint`。
- Stage 2：Gameplay Fire Origin -> `IntentPoint`，最多在终点方向增加约 1cm 的数值余量；不能从 Fire Origin 再延伸完整 MaxRange 越过意图点。
- 最终结果只允许来自 Stage 2。
- Stage 2 未命中必须返回 Miss；禁止回退 Stage 1 的相机 HitResult。
- 如果 FireOrigin 与 IntentPoint 过近，安全返回 Miss。
- `Acos` 前钳制 DotProduct 到 `[-1, 1]`。

### 6. Authority 认可后执行 Fire Cue

抽取一个空指针安全的 Fire Cue helper（函数名按现有风格决定并在报告列出）：

- 普通 Owning Client 在 Prediction Window 内预测执行一次 Fire Cue。
- Authority 只在该发通过验证、`CommitAbility()` 和 `CommitServerShot()` 成功后执行一次 Fire Cue，让 Remote Client 收到。
- 两端必须关联同一个 Ability PredictionKey，Owning Client 不应因预测+服务器认可明显双播。
- Listen Host 只走 Authority 执行，不得本地预测一次再 Authority 执行一次。
- Rejected 请求不得向 Remote Client 播放认可 Fire Cue。

Impact Cue 继续只由 Authority 最终 Stage 2 HitResult 触发。

## 同步修复项

### 7. 移除 StructUtils 弃用依赖

UE 5.8 的 `StructUtils/InstancedStruct.h` 和实现已经在 CoreUObject：

- 从 `Apecox.Build.cs` 删除 `StructUtils`。
- 从 `Apecox.uproject` 删除 `StructUtils` 插件项。
- 保留 `ModularGameplay` 模块和插件，因为 `UControllerComponent` 需要它。
- 删除后重新构建，证明 `TInstancedStruct` 仍能正常编译。

### 8. 加固服务端弹匣提交

- `CommitServerShot()` 必须检查 `GetTypedOuter<AActor>()` 存在且 `HasAuthority()`。
- 在修改状态前再次执行必要条件检查，防止弹匣降到负数或错误调用提交。
- 推荐改为返回 `bool`；调用方只有收到 true 才继续伤害和 Cue。若更改返回类型，更新 `Current_Code_Design.md` 对应一句和修复报告，但不要改变职责或另起概念。

### 9. 保持人工配置名称准确

不要操作资产，但修复报告必须明确后续人工配置：

```text
/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle
  InstanceClass = ApecoxRangedWeaponInstance
  FireConfig = ApecoxHitscanFireConfig
```

不要再写不存在的 `BP_RifleDefinition`。

## 构建与检查

运行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

构建前后静态确认：

- 全仓库不存在 `TargetDataHandle.Add(&`。
- Hitscan GA 的每个激活路径最终都有明确结束口。
- 本地 RPM 门控发生在 `CanActivateAbility()`，而不是每帧预测激活后才拒绝。
- 目标 ASC 不再使用 Character `FindComponentByClass`。
- Stage 2 Miss 不再返回 Stage 1 Hit。
- Authority 已执行认可 Fire Cue。
- Equipment CurrentWeaponInstance 已 OwnerOnly 复制。
- `.uproject`/`Build.cs` 不再含 `StructUtils`，仍保留 `ModularGameplay`。

## 报告与停止条件

创建中文修复报告：

```text
D:/UnrealProject/Apecox/Agent/Reports/2026-08-11_Phase2B1_Hitscan_Fire_Fix_Report.md
```

报告必须逐项对应本 Prompt 的 1-9，列出实际改动文件、函数签名变化、构建结果和仍未完成的 UE 人工配置。完成修复、构建和报告后停止，等待 Codex 复审。

