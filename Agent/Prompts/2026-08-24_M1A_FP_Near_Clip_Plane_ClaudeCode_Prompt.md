# Claude Code 实施 Prompt：M1-A 第一人称近裁剪面修复

你是 Apecox 项目的具体实施子代理。本次只实现已经由用户批准、并通过 PIE 控制台实验确认根因的第一人称近裁剪面修复。禁止扩展范围。

项目根目录：

```text
D:/UnrealProject/Apecox
```

## 背景与已确认事实

- 第一人称完整步枪的拾取、显示、连续开火、Montage、枪口 VFX 和声音已经正常。
- 当前缺陷是靠近相机的枪身后段被近裁剪面截断。
- 将 Arms 从 `SK_IG_FP_Mannequin` 临时替换为 `SK_IG_FP_Arms` 无法解决。
- PIE 中执行 `r.SetNearClipPlane 1` 后缺陷消失，根因已经确认。
- RAR 原工程使用全局 `NearClipPlane=0.000100`，Apecox 不复制该极端全局配置。

先阅读：

```text
Agent/00_Coordination/Current_Code_Design.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Source/Apecox/Public/Camera/ApecoxPlayerCameraManager.h
Source/Apecox/Private/Camera/ApecoxPlayerCameraManager.cpp
Source/Apecox/Private/Player/ApecoxPlayerController.cpp
```

## 批准的代码设计

只修改：

```text
Source/Apecox/Public/Camera/ApecoxPlayerCameraManager.h
Source/Apecox/Private/Camera/ApecoxPlayerCameraManager.cpp
```

### 1. 新增近裁剪面成员

在 `AApecoxPlayerCameraManager` 中新增私有配置成员：

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Camera|First Person", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
float FirstPersonNearClipPlane = 1.0f;
```

语义：本地第一人称视图使用的透视近裁剪距离，单位为厘米。不要将值降低为 RAR 原工程的 `0.0001`。

### 2. 覆写相机视图更新

在类的 `protected` 区域声明：

```cpp
virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;
```

在 `.cpp` 中实现：

1. 必须先调用 `Super::UpdateViewTargetInternal(OutVT, DeltaTime)`，保留引擎正常的 CameraComponent/CalcCamera 计算。
2. 父类计算结束后设置：

```cpp
OutVT.POV.PerspectiveNearClipPlane = FirstPersonNearClipPlane;
```

3. 添加简洁中文注释，解释这里使用 UE 5.8 的每视图近裁剪面，避免修改全局 `NearClipPlane` 并影响其他视图的深度精度。

## 设计边界

- 不修改 `Config/DefaultEngine.ini` 或任何其他配置文件。
- 不调用或持久化 `r.SetNearClipPlane`；它只用于此前的根因实验。
- 不新增 CameraComponent 子类，不修改 PlayerController 的 CameraManager 绑定。
- 不修改 Character、EquipmentComponent、WeaponPresentationActor、DataAsset、GAS、射击、网络或动画代码。
- 不调整任何 Arms、Weapon、Sight、Muzzle 的 Transform。
- 不创建、迁移或修改 `.uasset/.umap`。
- 不新增 GameplayTag、GA、GE、GameplayCue、RPC 或复制属性。
- 不执行 Git，不生成解决方案，不使用 MCP。

## 构建与报告

执行：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

新增中文报告：

```text
Agent/Reports/2026-08-24_M1A_FP_Near_Clip_Plane_Report.md
```

报告必须列出：

- 实际修改文件。
- 新增成员的名称、类型、默认值、UPROPERTY 和职责。
- 覆写函数的完整签名与执行顺序。
- 构建命令和结果。
- 是否存在范围外改动。

构建完成后停止，等待 Codex 复审。不要继续 UE 编辑器操作或 Git 操作。
