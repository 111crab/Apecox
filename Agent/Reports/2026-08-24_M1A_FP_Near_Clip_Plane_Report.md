# M1-A 第一人称近裁剪面修复实施报告

实施日期：2026-08-24  
构建结果：**编译通过，最终 DLL 链接因编辑器占用而失败**（详见第 4 节）  
状态：等待 Codex 复审。

## 1. 实际修改文件

严格限定在 Prompt「批准的代码设计」范围内，未超出：

| 文件 | 改动 |
| --- | --- |
| `Source/Apecox/Public/Camera/ApecoxPlayerCameraManager.h` | 新增私有成员 + protected 覆写声明 |
| `Source/Apecox/Private/Camera/ApecoxPlayerCameraManager.cpp` | 实现 `UpdateViewTargetInternal` 覆写 |
| `Agent/Reports/2026-08-24_M1A_FP_Near_Clip_Plane_Report.md` | 新增本报告 |

未修改 `Config/DefaultEngine.ini` 或其他配置、PlayerController 的 CameraManager 绑定、Character、EquipmentComponent、WeaponPresentationActor、DataAsset、GAS、射击、网络或动画代码；未调用 `r.SetNearClipPlane`，未调整任何 Transform，未创建/迁移/修改 `.uasset/.umap`；未新增 GameplayTag/GA/GE/GameplayCue/RPC/复制属性；未执行 Git、未生成解决方案、未使用 MCP。

## 2. 新增成员

| 名称 | 类型 | 默认值 | UPROPERTY | 职责 |
| --- | --- | --- | --- | --- |
| `FirstPersonNearClipPlane` | `float` | `1.0f` | `EditDefaultsOnly, Category = "Camera|First Person", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm")` | 本地第一人称视图使用的透视近裁剪距离（厘米），值保持 1.0 而非 RAR 原工程的 0.0001 |

## 3. 覆写函数

完整签名：

```cpp
virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;
```

执行顺序：

1. `Super::UpdateViewTargetInternal(OutVT, DeltaTime);` —— 先保留引擎正常的 CameraComponent / CalcCamera 计算。
2. `OutVT.POV.PerspectiveNearClipPlane = FirstPersonNearClipPlane;` —— 父类计算结束后应用项目级每视图近裁剪距离。

中文注释说明：使用 UE 5.8 的每视图近裁剪面 `PerspectiveNearClipPlane`，只调整本地第一人称视图，避免修改全局 `NearClipPlane` 影响其他视图的深度精度。

## 4. 构建命令与结果

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

结果：**编译全部通过，仅最终 DLL 链接失败**。

- 所有源码编译成功：`ApecoxPlayerCameraManager.cpp`、`ApecoxPlayerController.cpp`、`Module.Apecox.cpp` 均编译通过，`UnrealEditor-Apecox.lib` 成功生成；0 编译错误。
- 最终 `Link [x64] UnrealEditor-Apecox.dll` 失败，错误为：

```text
LNK1104: 无法打开文件 "D:\UnrealProject\Apecox\Binaries\Win64\UnrealEditor-Apecox.dll"
原因：The process cannot access the file because it is being used by another process. - UnrealEditor.exe
```

即 Unreal Editor 正在运行并锁定了目标 DLL。按项目约定「编辑器运行导致 DLL 锁定时，只报告，不强制结束用户进程」，本轮未强制结束编辑器进程。

非代码提示：`UbaSessionServer - Killed process ... Low on memory` —— 构建机内存压力导致 UBA 反复重试个别编译动作（最终仍编译成功），与代码无关。

建议：关闭正在运行的 Unreal Editor 后重新执行同一构建命令，DLL 链接即可完成。代码本身已通过编译，无 C++ 错误。

## 5. 是否存在范围外改动

无。改动仅涉及 `AApecoxPlayerCameraManager` 两个文件，未触碰其他系统。

---

*等待 Codex 复审。构建的 DLL 链接因编辑器占用未完成，不视为通过。*
