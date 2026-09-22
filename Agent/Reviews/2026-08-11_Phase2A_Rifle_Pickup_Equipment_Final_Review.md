# Phase 2A 第一把步枪拾取与装备闭环：最终代码复审

更新日期：2026-08-11

## 结论

**代码复审通过，可以进入 UE 编辑器人工配置和运行验证。**

首轮发现的 FastArray、原子事务、输入隔离、死亡校验、Pickup 配置和第一人称渲染问题均已在真实代码中修复。当前没有新的代码阻断项。

## 已核对修复

1. `FApecoxInventoryList` 新增使用 `MarkItemDirty`，删除和清空使用 `MarkArrayDirty`；不再用属性 Push Model 宏代替 FastArray 内部版本管理。
2. `UApecoxEquipmentComponent::EquipWeapon` 返回 `bool`，全部前置验证发生在卸下旧武器之前。
3. `TryAddAndEquipWeapon` 只有装备成功后才 Consume Pickup；失败会撤销槽位、条目、Registered Subobject 和 Claim。
4. Equipment 不再调用全局 `ClearAbilityInput()`。
5. `UApecoxAbilitySystemComponent::OnRemoveAbility` 精确删除当前 SpecHandle 的 Pressed/Held/Released 缓存，并同时覆盖服务器和拥有客户端的 Spec 删除路径。
6. Server RPC 在任何 Claim 和库存写入前拒绝 HealthComponent 缺失或 `IsDeadOrDying()` 的 Pawn。
7. 死亡和回滚路径不会将 `None` 传给 `ClearSlot`，也不会手工 `MarkAsGarbage`。
8. Pickup 的 `WeaponDefinition` 可以在 Blueprint Class Defaults 配置；客户端和服务器默认交互距离统一为 250 cm，查询球体缩小为 40 cm。
9. FP/TP 动态武器 Mesh 分别使用 UE 5.8 的 `FirstPerson` 与 `WorldSpaceRepresentation` 渲染类型。
10. 两个纯数据类的空 `.cpp` 已删除。

## 接受的范围例外

子代理额外修改了 `ApecoxPlayerCharacter.h`，新增：

```cpp
UApecoxHealthComponent* GetHealthComponent() const;
```

这超出了修复 Prompt 原本允许的文件范围，但接口是只读组件访问器，命名与现有 `GetEquipmentComponent()` 一致，后续 UI、生命状态查询和服务器玩法校验都会使用，Codex 建议保留。

## 报告勘误

修复报告把服务器能力删除路径写成了 `SetRemoveAbilitySpec`。真实代码仍由 `FApecoxAbilitySetGrantedHandles::RemoveFromAbilitySystem` 调用 `ClearAbility(Handle)`；随后进入本轮新增的 `OnRemoveAbility` 覆写。真实代码正确，以源码为准。

## 构建与剩余风险

- 子代理报告 `ApecoxEditor Win64 Development` 构建成功，耗时 12.16 秒，只有既有 NonInstanced 弃用警告。
- Codex 独立复构建时检测到另一条 `Build.bat` 已在运行，等待 120 秒后超时；没有终止用户进程。该结果不是编译失败，也不否定子代理已经完成的成功构建。
- 仍需通过真实 PIE 验证 FastArray/Subobject 到拥有客户端的复制、FP/TP 可见性、双客户端争抢以及死亡清空。这些属于运行验证，不是继续补写代码的理由。

## 下一步

执行 `Agent/00_Coordination/Current_UE_Manual_Steps.md`，完成单人和多人验证后将结果交给 Codex 收口。
