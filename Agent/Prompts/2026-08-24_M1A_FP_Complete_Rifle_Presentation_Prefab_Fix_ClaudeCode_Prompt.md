# Claude Code 修复 Prompt：M1-A FP Weapon Presentation Actor Spawn 契约

你是 Apecox 项目的具体实施子代理。Codex 已完成第一轮代码审查。本次只修复审查指出的两项问题，禁止扩展范围。

项目根目录：

```text
D:/UnrealProject/Apecox
```

先阅读：

```text
Agent/Reviews/2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_Review.md
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationActor.cpp
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp
```

## 修复 1：关闭 Actor 级碰撞

在 `AApecoxWeaponPresentationActor` 构造函数中显式调用：

```cpp
SetActorEnableCollision(false);
```

保留 `WeaponMesh` 的 `NoCollision` 和 `SetGenerateOverlapEvents(false)`，也保留 EquipmentComponent 对 Blueprint Primitive 的运行时统一配置。

## 修复 2：删除错误的 bNoFail 用法

在 `UApecoxEquipmentComponent::RefreshWeaponPresentation()` 中：

1. 删除 `SpawnParams.bNoFail = true;`。
2. 删除或改写“bNoFail 关闭引擎默认失败日志”的错误注释。
3. 显式设置：

```cpp
SpawnParams.SpawnCollisionHandlingOverride =
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
```

4. 保留 `SpawnActor` 返回值检查和现有项目 Warning。

## 禁止范围

- 不改公开类名、成员名、函数名或 UPROPERTY。
- 不改 Definition 的字段。
- 不改 FP/TP 分流、Montage、Muzzle、GAS、射击、网络或资产。
- 不修改 Build.cs、Config、`.uasset/.umap`。
- 不执行 Git，不生成解决方案，不使用 MCP。

## 构建与报告

执行：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

新增中文报告：

```text
Agent/Reports/2026-08-24_M1A_FP_Complete_Rifle_Presentation_Prefab_Fix_Report.md
```

报告只需列出两项修复的实际代码位置、构建结果和是否有范围外改动。构建完成后停止，等待 Codex 复审。
