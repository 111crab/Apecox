# Phase 2A 第一把步枪拾取与装备闭环 — 修复报告

**日期**: 2026-08-11  
**构建结果**: ✅ 成功（ApecoxEditor Win64 Development）  
**构建耗时**: 12.16 秒（8.93 秒 UBA）  
**编译警告**: 1 个（ApecoxAbilitySystemComponent.cpp:228 — 非本次修改引入的 NonInstanced 弃用警告）

---

## 一、修复清单与实现

### P1-1：FastArray 使用自身脏标记 API ✅

**文件**: `Public/Inventory/ApecoxInventoryComponent.h`、`Private/Inventory/ApecoxInventoryComponent.cpp`

**变更**:
1. `FApecoxInventoryList` 补回 `OwnerComponent` 非复制回指针（`UPROPERTY(NotReplicated)`）
2. 新增默认构造和带 `UApecoxInventoryComponent*` 参数的构造函数
3. `UApecoxInventoryComponent` 构造函数使用 `InventoryList(this)` 初始化
4. `AddEntry` — 在完成 `NewEntry` 赋值后调用 `MarkItemDirty(NewEntry)`
5. `RemoveEntry` — 在 `RemoveAt` 后调用 `MarkArrayDirty()`
6. `ClearAllEntries` — 只在 `Entries.Num() > 0` 时 `Reset()` 并调用 `MarkArrayDirty()`
7. 删除 `TryAddAndEquipWeapon`、`ClearInventoryForDeath`、`DestroyInstance` 中对 `InventoryList` 的 `MARK_PROPERTY_DIRTY_FROM_NAME` 调用

**函数位置**:
- `FApecoxInventoryList::AddEntry` — InventoryComponent.cpp:79-97
- `FApecoxInventoryList::RemoveEntry` — InventoryComponent.cpp:99-108
- `FApecoxInventoryList::ClearAllEntries` — InventoryComponent.cpp:110-119
- `UApecoxInventoryComponent::UApecoxInventoryComponent` — InventoryComponent.cpp:163-167

---

### P1-2：拾取与装备成为真实原子事务 ✅

**文件**: `Public/Equipment/ApecoxEquipmentComponent.h`、`Private/Equipment/ApecoxEquipmentComponent.cpp`、`Private/Inventory/ApecoxInventoryComponent.cpp`

**变更**:

**`EquipWeapon` 改为返回 `bool`**:
- 签名: `bool EquipWeapon(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* WeaponInstance)`
- 全部验证（非 Authority、ASC 无效、Instance 无效、Definition 无效、Slot=None）在 `UnequipCurrentWeapon()` 之前执行
- 验证失败返回 `false`，不修改已有装备
- 完整授予 AbilitySet、写入状态后返回 `true`
- 位置: EquipmentComponent.cpp:65-135

**`TryAddAndEquipWeapon` 原子回滚**:
- 在装备前验证 Character 和 EquipmentComponent 存在——任缺一项即回滚 Claim
- 调用 `Equipment->EquipWeapon(FreeSlot, WeaponInstance)` 检查返回值
- 返回 `false` 时逆序回滚:
  1. `WeaponSlotState.ClearSlot(FreeSlot)` + `MARK_PROPERTY_DIRTY_FROM_NAME`
  2. `InventoryList.RemoveEntry(WeaponInstance)` — 内部调用 `MarkArrayDirty`
  3. `RemoveReplicatedSubObject` + `RegisteredInstances.Remove`
  4. `Pickup->ReleaseClaim()` — 拾取物保留不被 Consume
- 不在回滚路径调用 `MarkAsGarbage`
- 位置: InventoryComponent.cpp:305-420

---

### P1-3：精确清理被移除 Ability 的输入缓存 ✅

**文件**: `Public/AbilitySystem/ApecoxAbilitySystemComponent.h`、`Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`

**变更**:
1. 在 `UApecoxAbilitySystemComponent` 新增 `OnRemoveAbility` 覆写声明
2. 实现中在 `Super::OnRemoveAbility(AbilitySpec)` 前从三组缓存移除 `AbilitySpec.Handle`:
   ```cpp
   InputPressedSpecHandles.Remove(AbilitySpec.Handle);
   InputHeldSpecHandles.Remove(AbilitySpec.Handle);
   InputReleasedSpecHandles.Remove(AbilitySpec.Handle);
   ```
3. 不发送伪造 InputReleased——Spec 删除本身会结束/移除 Ability
4. 覆盖服务器 `ClearAbility` 和拥有客户端收到 Spec 删除复制两条路径
5. `ClearAbilityInput()` 保留给 Avatar 切换、死亡和全局输入阻断，语义不变
6. Equipment 删除了 `ClearAbilityInput()` 调用——只使用 `WeaponAbilitySetHandles.RemoveFromAbilitySystem(ASC)` 精确撤销

**函数位置**:
- ASC 声明: ApecoxAbilitySystemComponent.h:50-58
- ASC 实现: ApecoxAbilitySystemComponent.cpp:144-155
- Equipment 中移除 ClearAbilityInput: EquipmentComponent.cpp:164-167

**为什么不会误伤英雄技能**: `OnRemoveAbility` 只移除正在被删除的 `AbilitySpec.Handle` 对应的输入条目。英雄技能（如战术技能）的 Spec 不受影响，因为它们没有被 `RemoveFromAbilitySystem` 删除。全局 `ClearAbilityInput()` 仍然存在，但只由 `InitAbilityActorInfo`（Avatar 切换）、`ProcessAbilityInput`（`State.Input.AbilityBlocked` 触发）和死亡流程使用。

---

### P1-4：服务器拒绝死亡 Pawn 拾取 ✅

**文件**: `Private/Inventory/ApecoxInventoryComponent.cpp`

**变更**:
- 在 `ServerRequestPickupWeapon_Implementation` 的 Character 类型验证后，立即查询 `Character->GetHealthComponent()`
- 组件缺失或 `IsDeadOrDying() == true` 时拒绝请求（返回，不执行后续逻辑）
- 此检查在 `TryClaim` 和任何库存写入之前——晚到 RPC 不会产生残留
- 位置: InventoryComponent.cpp:244-253

**附**: `AApecoxPlayerCharacter` 新增 `GetHealthComponent()` 只读 getter（ApecoxPlayerCharacter.h:43）

---

### P2-1：修正 WeaponSlotState 清理 ✅

**文件**: `Private/Inventory/ApecoxInventoryComponent.cpp`

**变更**:
1. `ClearInventoryForDeath`: `ClearAllSlots()` 后添加 `MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, WeaponSlotState, this)`
2. `DestroyInstance`: 只在 `FindSlotForInstance` 返回 Primary/Secondary 时调用 `ClearSlot`，然后标记 `MARK_PROPERTY_DIRTY_FROM_NAME`；不再将 `None` 传入 `ClearSlot`
3. 位置: InventoryComponent.cpp:430-431（ClearInventoryForDeath）、457-462（DestroyInstance）

---

### P2-2：Pickup WeaponDefinition 改为 EditDefaultsOnly ✅

**文件**: `Public/Weapons/ApecoxWeaponPickup.h`

**变更**: `WeaponDefinition` 的 UPROPERTY 元数据从 `EditInstanceOnly` 改为 `EditDefaultsOnly`，使 `BP_WeaponPickup_Rifle` 可在 Class Defaults 绑定 `DA_Weapon_Rifle`

---

### P2-3：动态武器 Mesh 设置 UE 5.8 FirstPersonPrimitiveType ✅

**文件**: `Private/Equipment/ApecoxEquipmentComponent.cpp`

**变更**: 在 `RefreshWeaponPresentation` 中:
- FP 武器: `SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson)` — 获得与手臂一致的 FOV 和近裁剪
- TP 武器: `SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation)` — 与第三人称角色 Mesh 一致
- 位置: EquipmentComponent.cpp:241-242（FP）、268-269（TP）

---

### P2-4：Pickup 检测收紧 ✅

**文件**: `Private/Weapons/ApecoxWeaponPickup.cpp`、`Private/Character/ApecoxPlayerCharacter.cpp`

**变更**:
1. **InteractionSphere 半径**: 从 `InteractionDistance`（250）改为固定 `40.0f`——交互球体只做小型 Query，交互距离另有独立校验
2. **InteractionDistance 默认**: 从 `200.0f` 改为 `250.0f`，与客户端 Trace 一致
3. **服务器视线起点**: 从 `Pawn->GetActorLocation()` 改为 `Pawn->GetPawnViewLocation()`——模拟真实玩家视角
4. **BlockingHit 检查简化**: 已忽略 Pawn 和 Pickup 自身，任何 `bBlockingHit` 都表示被障碍阻挡；移除无意义的 `HitActor != this` 判断
5. **客户端 Trace 注释**: 修正为"与 Pickup InteractionDistance 默认一致"

---

### P2-5：删除手工 MarkAsGarbage ✅

**文件**: `Private/Inventory/ApecoxInventoryComponent.cpp`

**变更**: `DestroyInstance` 中删除 `Instance->MarkAsGarbage()`。实例已通过移除槽位引用、FastArray 条目、`RegisteredInstances` 和 `RemoveReplicatedSubObject` 退出所有权链——让 GC 自然回收。

---

### P3-1：删除空实现文件 ✅

**已删除**:
- `Source/Apecox/Private/Inventory/ApecoxInventoryItemDefinition.cpp`
- `Source/Apecox/Private/Weapons/ApecoxWeaponPresentationDefinition.cpp`

纯数据类没有 .cpp 逻辑时不保留说明性空实现文件。

---

## 二、最终新增或变更的 API

| API | 变更类型 | 说明 |
|-----|---------|------|
| `bool UApecoxEquipmentComponent::EquipWeapon(...)` | 返回值 `void`→`bool` | 验证失败返回 false，不修改已有装备 |
| `UApecoxAbilitySystemComponent::OnRemoveAbility(...)` | 新增覆写 | 精准从三组输入缓存移除被删除 Spec 的句柄 |
| `AApecoxPlayerCharacter::GetHealthComponent()` | 新增 Getter | 只读访问 HealthComponent——支持 Server RPC 死亡校验 |
| `FApecoxInventoryList::FApecoxInventoryList(UApecoxInventoryComponent*)` | 新增构造 | 接收 OwnerComponent 回指针 |
| `UApecoxWeaponPickup::WeaponDefinition` | 元数据变更 | `EditInstanceOnly`→`EditDefaultsOnly` |
| `UApecoxWeaponPickup::InteractionDistance` | 默认值变更 | `200.0f`→`250.0f` |

---

## 三、FastArray 最终脏标记方式

| 操作 | API | 说明 |
|------|-----|------|
| 新增条目 | `MarkItemDirty(NewEntry)` | 在 `FApecoxInventoryList::AddEntry` 内部调用 |
| 删除条目 | `MarkArrayDirty()` | 在 `FApecoxInventoryList::RemoveEntry` 内部调用 |
| 清空数组 | `MarkArrayDirty()` | 在 `FApecoxInventoryList::ClearAllEntries` 内部调用（幂等：空数组跳过） |
| `InventoryList` 属性脏 | 不再使用 | `MARK_PROPERTY_DIRTY_FROM_NAME` 只用于 `WeaponSlotState` 和 `EquippedWeaponState` |

FastArray 必须维护自己的 ReplicationID/ReplicationKey。`MARK_PROPERTY_DIRTY_FROM_NAME` 是 Push Model 属性脏标记，不能替代 FastArray 的内部脏标记。

---

## 四、原子回滚的最终顺序

```
TryAddAndEquipWeapon (InventoryComponent.cpp:305-420):
  1. 验证 WeaponDef、InstanceClass、InstanceClass 子类检查
  2. TryClaim (Pickup)                          ← 原子占用
  3. 验证空槽位
  4. 验证 Character 和 EquipmentComponent      ← 新增：P1-2
  5. NewObject + Initialize + RegisterInstance
  6. InventoryList.AddEntry                     ← 内部 MarkItemDirty
  7. WeaponSlotState.SetSlot + MARK_PROPERTY_DIRTY
  8. Equipment->EquipWeapon()                   ← 检查返回值！
     ├─ true  → 9. 广播 → 10. Consume → 返回实例
     └─ false → 回滚:
        a. WeaponSlotState.ClearSlot + MARK_PROPERTY_DIRTY
        b. InventoryList.RemoveEntry            ← 内部 MarkArrayDirty
        c. RemoveReplicatedSubObject + RegisteredInstances.Remove
        d. Pickup->ReleaseClaim()               ← 拾取物保留
        e. 返回 nullptr
```

---

## 五、输入缓存清理路径

**服务器路径**（RemoveFromAbilitySystem → AbilitySet → ASC）:
1. `RemoveFromAbilitySystem` 调用 `SetRemoveAbilitySpec` → UAbilitySystemComponent 标记 Spec 待删除
2. ASC 处理删除时调用 `OnRemoveAbility(AbilitySpec)`
3. 覆写从 `InputPressedSpecHandles`、`InputHeldSpecHandles`、`InputReleasedSpecHandles` 移除该 Handle
4. 继续 `Super::OnRemoveAbility` 完成 Spec 销毁

**拥有客户端路径**（Spec 删除复制到达）:
1. 客户端收到 Spec 删除复制
2. ASC 调用 `OnRemoveAbility`
3. 同样的覆写清除输入缓存——防止 Held 中残留无效句柄

**不会误伤英雄技能**: 只移除正在被删除的 Spec 的 Handle。其他 Spec（英雄技能）不受影响。`ClearAbilityInput()` 保留给 Avatar 切换、死亡和全局输入阻断。

---

## 六、删除的空 .cpp

- `Source/Apecox/Private/Inventory/ApecoxInventoryItemDefinition.cpp`
- `Source/Apecox/Private/Weapons/ApecoxWeaponPresentationDefinition.cpp`

---

## 七、构建结果

```text
构建命令:
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
  -Project="D:\UnrealProject\Apecox\Apecox.uproject"
  -WaitMutex -FromMsBuild -architecture=x64

结果: 成功
耗时: 12.16 秒（UBA 8.93 秒）
操作数: 8（5 个编译 + 1 个 Module + 1 个 .lib + 1 个 .dll + 1 个 WriteMetadata）

编译警告:
- ApecoxAbilitySystemComponent.cpp(228): warning C4996
  'NonInstanced' 弃用——非本次修改引入（Phase 1C 已有代码）

首次构建报错记录:
- InventoryComponent.cpp:247: "GetHealthComponent" 不是 AApecoxPlayerCharacter 的成员
  原因: HealthComponent 是 protected 成员，无公共 getter
  修复: AApecoxPlayerCharacter.h 新增 GetHealthComponent() getter
```

---

## 八、声明

- ✅ 未使用 MCP 工具
- ✅ 未修改任何 `.uasset`、`.umap` 资产文件
- ✅ 未执行 Git add/commit/push 操作
- ✅ 未操作 UE 编辑器
- ✅ 未修改 `Agent/00_Coordination`、RFC 或原始实施报告
- ✅ 所有修复均在 Prompt 批准范围内
- ✅ ApecoxEditor Win64 Development 构建通过
- ✅ 没有引入射击、弹药、切枪、动画或 UI 代码
