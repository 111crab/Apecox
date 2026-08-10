# ClaudeCode 实施提示：Phase 1C-CAM PlayerCameraManager 基线

项目：`D:/UnrealProject/Apecox`

你在新的 ClaudeCode 上下文中执行具体代码实施。Codex 负责架构规划和后续代码审查，用户负责批准关键命名、UE 编辑器配置与运行验证。

## 先阅读

- `Agent/README.md`
- `.agents/ue-project-context.md`
- `Agent/00_Coordination/Current_Code_Design.md` 中最后一节 `Phase 1C-CAM：项目 PlayerCameraManager 基线`
- `Agent/00_Coordination/Subagent_Review_Checklist.md`
- `Source/Apecox/Public/Player/ApecoxPlayerController.h`
- `Source/Apecox/Private/Player/ApecoxPlayerController.cpp`
- `Source/Apecox/Private/Game/ApecoxGameMode.cpp`
- UE 5.8：`E:/UE_5.8/Engine/Source/Runtime/Engine/Classes/Camera/PlayerCameraManager.h`

## 已批准设计

新增项目自有类：

```text
AApecoxPlayerCameraManager : APlayerCameraManager
```

文件必须遵守 Public/Private 镜像目录：

```text
Source/Apecox/Public/Camera/ApecoxPlayerCameraManager.h
Source/Apecox/Private/Camera/ApecoxPlayerCameraManager.cpp
```

类只需要公开构造函数：

```cpp
AApecoxPlayerCameraManager();
```

构造函数设置父类已有属性：

```cpp
ViewPitchMin = -70.0f;
ViewPitchMax = 80.0f;
```

这些数值已从本机 UE 5.8 官方 `BP_FirstPersonCameraManager` 核准。添加简短中文注释，说明限制上下观察角度是为了减少完整 Manny 第一人称摄像机进入身体内部的穿模。

不要为这两个值重复创建成员变量或 UPROPERTY，不提前创建 ADS、后坐力、CameraMode 等 API。

## 修改 AApecoxPlayerController

在现有类中新增公开构造函数：

```cpp
AApecoxPlayerController();
```

在 `.cpp` 直接包含新 CameraManager 头文件，并在构造函数中设置：

```cpp
PlayerCameraManagerClass = AApecoxPlayerCameraManager::StaticClass();
```

现有 `PostProcessInput()` 代码和调用顺序必须保持不变。

`AApecoxGameMode` 已经指定 `AApecoxPlayerController::StaticClass()`，不修改 GameMode，不创建 PlayerController 蓝图。

## 范围限制

- 不修改 `AApecoxPlayerCharacter`、Camera Component、双 Mesh、FOV、Scale 或 Camera Offset。
- 不修改 CMC、输入、ASC、Ability、AttributeSet、Health、Death 或 Respawn。
- 不修改 `Config`；用户已经通过项目设置常驻关闭 Motion Blur。
- 不设置 `NearClipPlane`。
- 不创建或修改 `.uasset`、`.umap`、GameplayTag、GA、GE 或 DataAsset。
- 不使用 MCP，不操作 UE 编辑器，不执行 Git 命令。
- 不格式化或整理无关文件；工作区已有大量用户与前序阶段改动，必须保留。

## 构建

修改后执行完整构建：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

如果 UE 编辑器正在占用 DLL，应明确报告阻塞，不要擅自结束用户进程。

## 中文报告

写入：

`Agent/Reports/2026-08-10_Phase1C_CAM_PlayerCameraManager_Baseline_Report.md`

报告必须列出：

- 实际修改/新增的文件；
- 新类名、父类、文件路径和职责；
- 新增函数名、可见性和作用；
- 是否新增成员变量或 UPROPERTY（预期没有）；
- PlayerController 如何采用 CameraManager；
- 构建命令和结果；
- 明确没有修改 Config、Character、GAS、资产、地图或 Git。

完成后停止，等待 Codex 审查。
