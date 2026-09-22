# M1-A 第一人称完整步枪表现 Prefab 代码审查

审查日期：2026-08-24  
结论：**复审通过，进入 UE 人工配置与单人验证。**

## 审查结论

本轮公开类名、成员名、配置字段和职责边界均符合已批准设计。FP Actor 只在本地控制角色生成，Dedicated Server 跳过表现，TP 仍保持原单 Mesh 路径；没有改动 GAS、射击预测、服务器命中、弹药、复制状态或资产。

第一轮发现集中在 Presentation Actor 的碰撞与 Spawn 契约，不涉及架构返工；子代理已按审查意见完成修复并重新构建通过。

## 复审结果

- `AApecoxWeaponPresentationActor` 构造函数已增加 `SetActorEnableCollision(false)`。
- `RefreshWeaponPresentation()` 已删除错误的 `bNoFail` 用法，改为显式 `AlwaysSpawn` 碰撞策略。
- Spawn 返回值检查和项目 Warning 保持不变。
- 修复只涉及两个批准的 `.cpp` 文件，没有改动公开 API、配置字段、网络、GAS 或资产。
- 子代理报告 Development Editor 增量构建通过，0 错误、0 新增警告。

## 发现

### 已修复 P2：Presentation Actor 没有关闭 Actor 级碰撞

文件：`Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp`

构造函数只关闭了 `WeaponMesh` 的碰撞，没有调用：

```cpp
SetActorEnableCollision(false);
```

EquipmentComponent 虽然会在生成后遍历所有 Primitive 关闭碰撞，但 Actor 自身不再满足“纯表现、默认无碰撞”的独立类契约；Blueprint 子类新增组件时也会在生成后的统一配置前短暂保留自己的默认碰撞设置。

修复：在 Actor 构造函数中显式关闭 Actor 级碰撞，继续保留 Primitive 的运行时兜底配置。

### 已修复 P2：`bNoFail` 的用途理解错误

文件：`Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp`

当前注释称 `SpawnParams.bNoFail = true` 用于“关闭引擎默认失败日志”。UE 5.8 的 `FActorSpawnParameters::bNoFail` 并不负责日志开关；它会放宽特定 Spawn 失败条件，并把可能拒绝生成的碰撞策略提升为总是生成。

修复：删除 `bNoFail` 及错误注释；由于该 Actor 是无碰撞纯表现对象，显式设置：

```cpp
SpawnParams.SpawnCollisionHandlingOverride =
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
```

保留现有 Spawn 返回值检查与项目 Warning。

## 已通过项目

- `AApecoxWeaponPresentationActor` 的父类、UCLASS、API 宏、Public/Private 镜像目录正确。
- `PresentationRoot`、`WeaponMesh`、`MuzzlePoint` 和两个访问器命名正确。
- Definition 只把 FP 单 Mesh 字段替换为 `FirstPersonWeaponPresentationClass`。
- FP Actor 使用 Owner Character，本地控制角色生成，远端角色不生成。
- Blueprint 新增 Primitive 会被统一设置 OnlyOwnerSee、无碰撞和 First Person Primitive Type。
- Destroy、OnRep、Unequip、EndPlay 路径保持幂等意图。
- Arms/Character 不会因缺 AnimInstance 被切换到 Single Node。
- FP MuzzlePoint 与 TP Socket 的调用路径分离正确。
- 子代理报告称 Development Editor 构建通过，0 错误、0 新增警告。

## 复审标准

执行 `Agent/00_Coordination/Current_UE_Manual_Steps.md`。本轮先完成 FP Presentation Blueprint、RAR 固定枪械装配和单人验证；暂不进入 TP 表现。
