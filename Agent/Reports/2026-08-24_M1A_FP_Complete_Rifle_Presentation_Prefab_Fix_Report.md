# M1-A FP Weapon Presentation Actor Spawn 契约修复报告

修复日期：2026-08-24  
构建结果：**通过**（0 错误，0 新增警告）  
状态：等待 Codex 复审。

## 1. 实际修改（是否超出范围）

只修改两个文件，未超出 Prompt「禁止范围」：

| 文件 | 改动 |
| --- | --- |
| `Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp` | 构造函数新增 `SetActorEnableCollision(false)` |
| `Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp` | 删除错误的 `bNoFail` 及注释，改用 `SpawnCollisionHandlingOverride = AlwaysSpawn` |

未改公开类名、成员名、函数名、UPROPERTY、Definition 字段、FP/TP 分流、Montage、Muzzle、GAS、射击、网络或资产；未修改 Build.cs/Config/`.uasset/.umap`；未执行 Git、未生成解决方案、未使用 MCP。

## 2. 修复内容与位置

### 修复 1：关闭 Actor 级碰撞

`Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp` 构造函数，在 `SetReplicates(false)` 之后新增：

```cpp
// 显式关闭 Actor 级碰撞：纯表现对象从 Actor 层面即不参与世界碰撞/Overlap，
// 即使 Blueprint 子类后续新增组件，生成前也先被隔离。
SetActorEnableCollision(false);
```

保留了 `WeaponMesh` 的 `NoCollision` 与 `SetGenerateOverlapEvents(false)`，也保留 EquipmentComponent 对 Blueprint Primitive 的运行时统一配置。

### 修复 2：删除错误 bNoFail 用法

`Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp` 的 `RefreshWeaponPresentation()`：

- 删除 `SpawnParams.bNoFail = true;`。
- 删除“bNoFail 关闭引擎默认失败日志”的错误注释，改写为：
  `该 Actor 是无碰撞纯表现对象，显式 AlwaysSpawn，避免被生成碰撞策略拒绝。`
- 显式设置：

```cpp
SpawnParams.SpawnCollisionHandlingOverride =
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
```

保留了 `SpawnActor` 返回值检查和项目 Warning。

## 3. 构建结果

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
结果：Succeeded
耗时：Total execution time: 7.56 seconds（增量构建，仅重编 ApecoxWeaponPresentationActor.cpp 与 ApecoxEquipmentComponent.cpp）
错误：0
新增警告：0
```

非代码提示：`UbaServer - bind 0.0.0.0:1345 failed (...)` —— 本地端口占用导致 UBA 服务绑定失败，回退到本地执行器完成构建，与本次代码无关。

## 4. 是否有范围外改动

无。

---

*构建通过后停止，等待 Codex 复审。*
