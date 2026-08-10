# ClaudeCode 修复提示：Phase 1C-V 第一人称姿态复制骨骼刷新

项目：`D:/UnrealProject/Apecox`

请先阅读：

- `Agent/Reviews/2026-08-10_Phase1C_V_Manny_FirstThirdPerson_Baseline_Review.md`
- `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
- UE 5.8 参考：`E:/UE_5.8/Engine/Source/Runtime/Engine/Private/Character.cpp`

本轮只做以下两项修改：

1. 在 `ApecoxPlayerCharacter.cpp` 直接包含：

```cpp
#include "Engine/CollisionProfile.h"
```

2. 在构造函数配置 `ThirdPersonMesh` 的代码块中加入：

```cpp
ThirdPersonMesh->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

添加简短中文注释，解释：拥有者本地的第三人称 Mesh 因 `OwnerNoSee` 不渲染，但 `ABP_FP_Copy` 仍需要它每帧刷新组件空间骨骼，因此不能只使用 `ACharacter` 默认的 `AlwaysTickPose`。

不要修改其他逻辑，不操作 UE 编辑器、MCP、资产、Config 或 Git。

修改后重新执行完整构建：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

把中文小修报告写入：

`Agent/Reports/2026-08-10_Phase1C_V_Manny_FirstThirdPerson_Baseline_Fix_Report.md`

报告列出修改位置、原因、构建结果，并明确没有修改其他文件。完成后停止，等待 Codex 复审。

