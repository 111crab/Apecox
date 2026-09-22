# Phase 2A 第一把步枪拾取与装备闭环：代码初审

更新日期：2026-08-11

## 审查结论

**初审未通过，暂不进入 UE 编辑器配置。**

当前分层方向正确：私有 Inventory 位于 PlayerController、当前 Equipment 位于 Character、Definition 与 Instance 分离、公开装备摘要驱动远端表现，这些都符合已批准设计。ClaudeCode 报告的 `ApecoxEditor Win64 Development` 构建也已通过。

但代码仍有 4 个高优先级正确性问题和若干生命周期、配置可用性问题。它们主要影响多人库存复制、拾取事务原子性以及武器卸下时对英雄技能输入的隔离，不能留到 PIE 后再处理。

## 审查发现

### P1-1：FastArray 没有使用自己的脏标记 API

位置：

- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:79`
- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:99`
- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:111`

`AddEntry` 没有调用 `MarkItemDirty(NewEntry)`；删除和清空也没有调用 `MarkArrayDirty()`。调用方使用的 `MARK_PROPERTY_DIRTY_FROM_NAME` 只服务于 Push Model 属性脏标记，不能替代 `FFastArraySerializer` 的 ReplicationID、ReplicationKey 和 ArrayReplicationKey 管理。

影响：拥有客户端可能收不到新增、删除或死亡清空的库存 Delta，或者 FastArray 回调行为不稳定。

修复：新增或修改条目调用 `MarkItemDirty`，只删除条目调用 `MarkArrayDirty`。组件层不再把 `MARK_PROPERTY_DIRTY_FROM_NAME(InventoryList)` 当作 FastArray 脏标记。按照原 Prompt 补回 `OwnerComponent` 非复制回指针，并在组件构造时使用 `InventoryList(this)` 初始化。

### P1-2：拾取事务会在装备失败时仍然吞掉拾取物

位置：

- `Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h:94`
- `Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp:65`
- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:357`

`EquipWeapon` 返回 `void`。当 Character、EquipmentComponent、ASC、Definition 或槽位验证失败时，调用方无法知道装备失败，仍然广播成功、`Pickup->Consume()` 并返回 WeaponInstance。

影响：服务器可能销毁世界拾取物，但玩家没有真正装备成功；库存、槽位和表现会出现半完成状态，违反本轮明确要求的原子事务。

修复：将 `EquipWeapon` 改为 `bool`。所有前置失败返回 `false`，完整写入装备状态后返回 `true`。`TryAddAndEquipWeapon` 必须要求当前 Character、EquipmentComponent 和 `EquipWeapon(...)` 全部成功；否则逆序撤销槽位、FastArray 条目、Registered Subobject 和 Claim，且保留 Pickup。

### P1-3：卸下武器会清空所有英雄技能的输入缓存

位置：`Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp:153`

`UnequipCurrentWeapon()` 调用全局 `AbilitySystemComponent->ClearAbilityInput()`。这会清空 ASC 的 Pressed、Held、Released 三组全部句柄，不只影响武器 GA，也会打断或丢失战术技能、被动监听等非武器输入。

影响：切枪或死亡卸枪可能误伤英雄技能，违背 AbilitySet 按句柄隔离的设计。

修复：删除 Equipment 中的全局清空。由 `UApecoxAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec&)` 覆写，在任意 Spec 被服务器移除或复制移除时，仅从三组输入缓存删除该 SpecHandle，再调用 `Super::OnRemoveAbility`。随后继续使用 `FApecoxAbilitySetGrantedHandles::RemoveFromAbilitySystem` 精确撤销武器 AbilitySet。全局 `ClearAbilityInput()` 只保留给死亡、Avatar 切换和全局输入阻断。

### P1-4：服务器没有实际拒绝死亡中的 Pawn 拾取

位置：

- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:192`
- `Source/Apecox/Private/Weapons/ApecoxWeaponPickup.cpp:46`

注释声称验证了 Pawn 存活，但实际只检查 Pawn 存在、距离、视线和 Pickup 状态。死亡输入阻断不能代替服务器校验，晚到 RPC 仍可能在死亡窗口内执行。

修复：Server RPC 在 Character 类型验证后读取已有 `UApecoxHealthComponent::IsDeadOrDying()`；组件缺失或死亡中都拒绝请求。不要直接读取 AttributeSet，也不要新增临时死亡 Tag。

### P2-1：武器槽清除路径没有完整维护复制状态

位置：

- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:388`
- `Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:445`

死亡清理和 `DestroyInstance` 修改 `WeaponSlotState` 后没有统一做属性脏标记；`DestroyInstance` 在找不到槽位时还会将 `None` 传给 `ClearSlot`，触发不必要的 ensure。

修复：只有找到真实槽位时才清除，并在每次实际修改 `WeaponSlotState` 后标记该属性；死亡 `ClearAllSlots` 后同样标记。清理必须保持幂等。

### P2-2：Pickup 的 Definition 无法在蓝图类默认值中配置

位置：`Source/Apecox/Public/Weapons/ApecoxWeaponPickup.h:84`

`WeaponDefinition` 使用 `EditInstanceOnly`，但本轮批准的工作流是创建 `BP_WeaponPickup_Rifle` 并在其类默认值中绑定 `DA_Weapon_Rifle`。当前元数据会迫使用户逐个设置关卡实例，和资产设计冲突。

修复：改为 `EditDefaultsOnly`。未来运行时死亡掉落需要动态赋值时，再增加服务器初始化 API，不在本轮预造。

### P2-3：动态武器 Mesh 没有使用 UE 5.8 第一人称渲染类型

位置：`Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp:230`

Character 的 FP/TP Mesh 已设置 `EFirstPersonPrimitiveType`，动态创建的武器 Mesh 却只设置了 OnlyOwnerSee/OwnerNoSee。UE 5.8 官方 Shooter 模板会为 FP 武器设置 `FirstPerson`，为 TP 武器设置 `WorldSpaceRepresentation`。

影响：第一人称武器可能无法获得与手臂一致的 FOV 和近裁剪处理，出现枪体比例或穿模不一致。

修复：FP 武器设置 `SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson)`；TP 武器设置 `WorldSpaceRepresentation`。

### P2-4：拾取检测的碰撞半径、视线起点和距离口径需要收紧

位置：

- `Source/Apecox/Private/Weapons/ApecoxWeaponPickup.cpp:22`
- `Source/Apecox/Private/Weapons/ApecoxWeaponPickup.cpp:77`
- `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp:591`

当前把服务器交互距离 `200` 同时作为 InteractionSphere 半径，导致可命中球体过大；客户端 Trace 又硬编码 `250` 并注释为“与交互距离一致”。服务器视线从 Pawn 中心出发，而且已经忽略 Pickup 后仍判断 `HitActor != this`。

修复：InteractionSphere 只保留合理的小型查询半径；服务器交互距离与客户端 Trace 默认统一为 `250`。服务器视线从 `Pawn->GetPawnViewLocation()` 出发，忽略 Pawn 与 Pickup 后只要出现 BlockingHit 就拒绝。服务器校验仍是最终真相。

### P2-5：手工调用 `MarkAsGarbage` 没有必要且增加 Subobject 生命周期风险

位置：`Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp:445`

实例已经通过移除槽位引用、FastArray 条目、RegisteredInstances 和 `RemoveReplicatedSubObject` 退出所有权链。此时应让 GC 自然回收；立即 `MarkAsGarbage` 会让仍处于复制/清理调用栈的引用更难审计。

修复：删除显式 `MarkAsGarbage()`。确保所有强引用和复制注册均已解除即可。

### P3-1：实现报告与文件系统不一致

报告声称没有创建空 `.cpp`，但以下文件实际存在且只有 include/说明注释：

- `Source/Apecox/Private/Inventory/ApecoxInventoryItemDefinition.cpp`
- `Source/Apecox/Private/Weapons/ApecoxWeaponPresentationDefinition.cpp`

修复：按原 Prompt 删除这两个无运行时实现的 `.cpp`；在修复报告中列出删除项。不要为纯数据类保留空实现文件。

## 已核对的正向结果

- Public/Private 目录镜像总体正确。
- Inventory/Equipment 所有权和私有/公开复制边界方向正确。
- ItemInstance 使用 PlayerController Outer，并实现 Registered Subobject 兼容路径。
- `WeaponInstance` 作为 AbilitySet SourceObject 的方向正确。
- Dedicated Server 表现路径有显式跳过。
- `InputTag.Interact` 使用 Native Gameplay Tag，未滥加临时状态 Tag。
- 本轮没有提前加入射击参数或空的 `FInstancedStruct`。

## 复审门槛

1. 上述 P1/P2 全部修复。
2. `ApecoxEditor Win64 Development` 重新构建通过。
3. 修复报告列出最终新增/变更 API，尤其是 `EquipWeapon` 返回值和 ASC 的 `OnRemoveAbility` 覆写。
4. Codex 静态复审通过后，才生成 UE 人工配置清单。
