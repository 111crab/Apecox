# ClaudeCode 修复 Prompt：Phase 2A 第一把步枪拾取与装备闭环

## 一、任务身份和边界

你仍是 Apecox 项目的 C++ 实施子代理。首轮代码已完成并构建通过，但 Codex 初审发现多人复制、事务原子性和输入隔离问题。

项目：`D:/UnrealProject/Apecox`  
引擎：`E:/UE_5.8`

本轮只修复审查问题：

- 不操作 UE 编辑器、MCP、`.uasset/.umap`。
- 不执行 Git add/commit/push。
- 不重新生成 Rider/Visual Studio 工程文件。
- 不实现 Fire、ADS、Reload、Ammo、Switch、Drop、动画或 UI。
- 不修改已批准的所有权、类名、槽位枚举和 GameplayTag。

## 二、开始前必须阅读

1. `Agent/Reviews/2026-08-11_Phase2A_Rifle_Pickup_Equipment_Review.md`
2. `Agent/00_Coordination/Current_Code_Design.md`
3. `Agent/Reports/2026-08-11_Phase2A_Rifle_Pickup_Equipment_Report.md`
4. 本轮涉及的所有真实源码。

审查报告是本轮修复规格。不要只修编译错误，也不要忽略报告中的 P2 项。

## 三、允许修改的文件

```text
Source/Apecox/Public/Inventory/ApecoxInventoryComponent.h
Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h
Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponPickup.h
Source/Apecox/Private/Weapons/ApecoxWeaponPickup.cpp
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp（仅允许统一本地 Trace 距离/注释）
```

允许删除两个没有运行时实现的空文件：

```text
Source/Apecox/Private/Inventory/ApecoxInventoryItemDefinition.cpp
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationDefinition.cpp
```

允许新增修复报告：

```text
Agent/Reports/2026-08-11_Phase2A_Rifle_Pickup_Equipment_Fix_Report.md
```

不得修改 `Agent/00_Coordination`、RFC、原始实施报告或其他无关文件。若修复确实必须扩大 C++ 文件范围，先停止并在回复中写明证据。

## 四、强制修复项

### 4.1 正确实现 FastArray 脏标记

`FApecoxInventoryList`：

- 补回原 Prompt 要求的 `OwnerComponent` 非复制回指针。
- 提供默认构造和接收 `UApecoxInventoryComponent*` 的构造。
- `UApecoxInventoryComponent` 构造函数使用 `InventoryList(this)` 初始化。
- `AddEntry` 在完成新条目赋值后调用 `MarkItemDirty(NewEntry)`。
- `RemoveEntry` 成功删除后调用 `MarkArrayDirty()`。
- `ClearAllEntries` 只有在确实存在条目时 Reset 并调用 `MarkArrayDirty()`，保持幂等。
- 删除或修正所有“由 `MARK_PROPERTY_DIRTY_FROM_NAME` 代替 FastArray 脏标记”的注释和调用。FastArray 必须维护自己的 ReplicationID/ReplicationKey。
- `WeaponSlotState` 仍是普通复制结构；每次实际修改它后保留/补上对应属性脏标记。

### 4.2 让拾取与装备成为真实原子事务

将：

```cpp
void UApecoxEquipmentComponent::EquipWeapon(...)
```

改为：

```cpp
bool UApecoxEquipmentComponent::EquipWeapon(...)
```

要求：

- 非 Authority、ASC 无效、Instance 无效、Definition 无效或 Slot=None 时返回 `false`，不得改变已有装备。
- 所有验证必须发生在 `UnequipCurrentWeapon()` 之前。
- 完整授予 AbilitySet、写入 CurrentWeaponInstance 和 EquippedWeaponState 后返回 `true`。
- 当前阶段 `EquippedAbilitySets` 为空仍算合法装备成功。

`TryAddAndEquipWeapon`：

- 必须获得有效的 `AApecoxPlayerCharacter` 和 `UApecoxEquipmentComponent`。
- 只有 `Equipment->EquipWeapon(...) == true` 后才广播成功、Consume Pickup 并返回实例。
- 任一失败都调用统一回滚：清除真实槽位、移除 FastArray 条目、注销 Registered Subobject、从 RegisteredInstances 移除、释放 Claim。
- 回滚后 Pickup 仍存在，库存和槽位没有残留。
- `DestroyInstance` 找不到武器槽时不得将 `None` 传给 `ClearSlot`。
- 删除 `Instance->MarkAsGarbage()`；解除全部强引用和复制注册后让 GC 自然回收。

### 4.3 精确清理被移除 Ability 的输入缓存

禁止 Equipment 调用全局 `ClearAbilityInput()`。

在 `UApecoxAbilitySystemComponent` 覆写：

```cpp
virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;
```

要求：

- 在调用 `Super::OnRemoveAbility(AbilitySpec)` 前，将 `AbilitySpec.Handle` 从 `InputPressedSpecHandles`、`InputHeldSpecHandles`、`InputReleasedSpecHandles` 三组中移除。
- 该覆写要同时覆盖服务器 `ClearAbility` 和拥有客户端收到 Spec 删除复制的路径，防止 Held 中永久残留无效句柄。
- 不发送伪造的 InputReleased；Spec 删除本身会结束/移除 Ability。
- `ClearAbilityInput()` 继续保留给 Avatar 切换、死亡和全局输入阻断，不改变其语义。
- Equipment 只调用现有 `WeaponAbilitySetHandles.RemoveFromAbilitySystem(ASC)` 精确撤销武器授予。

### 4.4 服务器拒绝死亡 Pawn 拾取

在 `ServerRequestPickupWeapon_Implementation` 完成 Character Cast 后：

- 读取现有 `UApecoxHealthComponent`。
- 组件缺失或 `IsDeadOrDying()==true` 时拒绝请求。
- 可以在该 `.cpp` 内通过 Character 的现有组件查询获得 HealthComponent；不要修改 AttributeSet、DeathAbility，不新增 Getter 或临时 GameplayTag。
- 此校验必须在 TryClaim 和任何库存写入之前完成。

### 4.5 修正 WeaponSlotState 清理

- `ClearInventoryForDeath()` 调用 `ClearAllSlots()` 后标记 `WeaponSlotState` 已修改。
- `DestroyInstance()` 只在 `FindSlotForInstance` 返回 Primary/Secondary 时调用 `ClearSlot`，然后标记该属性。
- 多次死亡清理、回滚和卸下必须幂等，不产生 ensure。

### 4.6 修正 Pickup 资产配置和检测

- `WeaponDefinition` 从 `EditInstanceOnly` 改为 `EditDefaultsOnly`，使 `BP_WeaponPickup_Rifle` 能在 Class Defaults 绑定 `DA_Weapon_Rifle`。
- `InteractionDistance` 默认改为 `250.0f`，与本地短 Trace 默认一致。
- InteractionSphere 只作为拾取 Actor 的小型 Query 命中体，不要把交互距离直接当成球体半径；设置约 `35~50 cm` 的合理默认查询半径。
- 服务器视线起点使用 `Pawn->GetPawnViewLocation()`。
- 既然 Query 忽略 Pawn 和 Pickup，本次 LineTrace 只要出现 BlockingHit 就表示被障碍阻挡，不再保留永远无意义的 `HitActor != this` 判断。
- `ApecoxPlayerCharacter.cpp` 中本地 Trace 继续使用 `250.0f`，修正注释，不创建通用交互框架。

### 4.7 修正 UE 5.8 FP/TP 武器渲染类型

动态 Mesh 创建后设置：

```cpp
FirstPersonWeaponMeshComponent->SetFirstPersonPrimitiveType(
    EFirstPersonPrimitiveType::FirstPerson);

ThirdPersonWeaponMeshComponent->SetFirstPersonPrimitiveType(
    EFirstPersonPrimitiveType::WorldSpaceRepresentation);
```

保持原有 `OnlyOwnerSee`、`OwnerNoSee`、无碰撞、Socket 和 Transform 逻辑。

### 4.8 删除空实现文件

删除：

- `ApecoxInventoryItemDefinition.cpp`
- `ApecoxWeaponPresentationDefinition.cpp`

纯数据类没有 `.cpp` 逻辑时不保留说明性空实现文件。

## 五、必须自检

1. FastArray 新增、删除、死亡清空都使用自身 Mark API。
2. Equipment 缺失或 ASC 未初始化时，Pickup 不销毁且库存无残留。
3. 卸下武器只清理被移除 Spec 的输入句柄，英雄技能输入不受影响。
4. Server 拒绝死亡中 Pawn 的晚到拾取 RPC。
5. Blueprint 类默认值可以配置 WeaponDefinition。
6. FP/TP 动态武器 Mesh 使用 UE 5.8 对应 FirstPersonPrimitiveType。
7. 没有引入射击、弹药、切枪、动画或 UI 代码。
8. 没有修改 UE 资产、MCP、Git 或项目文件。

## 六、构建

执行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

修复所有本轮引入的编译错误。不得终止用户 UE/Rider 进程；若外部占用导致失败，记录真实证据。

## 七、修复报告

创建 `Agent/Reports/2026-08-11_Phase2A_Rifle_Pickup_Equipment_Fix_Report.md`，中文说明：

1. 每个审查项如何修复及文件/函数位置。
2. 最终新增或改变的 API，重点列出 `EquipWeapon` 返回值和 `OnRemoveAbility` 覆写。
3. FastArray 最终脏标记方式。
4. 原子回滚的最终顺序。
5. 输入缓存为什么不会误伤英雄技能，以及服务器/拥有客户端如何都得到清理。
6. 删除的空 `.cpp`。
7. 构建命令、结果和警告。
8. 明确声明没有操作 UE 编辑器、MCP、Git 和项目文件生成。

## 八、停止条件

以上修复全部完成、构建通过并写完修复报告后停止，等待 Codex 复审。不要开始 UE 配置，也不要继续 Phase 2B。
