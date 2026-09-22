# M1-A 第一人称近裁剪面修复复审

复审日期：2026-08-24  
结论：**代码与完整构建通过；等待 PIE 回归验证。**

## 审查结论

未发现需要子代理返工的代码问题。

- `AApecoxPlayerCameraManager::UpdateViewTargetInternal()` 先调用父类，再设置 `OutVT.POV.PerspectiveNearClipPlane`，执行顺序正确。
- `FirstPersonNearClipPlane` 使用 `1.0 cm` 默认值，并通过元数据限制编辑器最小值，未采用 RAR 的 `0.0001 cm`。
- 未修改全局 `NearClipPlane`、PlayerController 绑定、Character、武器、GAS、网络或资产。
- 本轮没有新增类、GameplayTag、GA、GE、GameplayCue、RPC 或复制属性。

## 公开设计变化

现有类：

```text
AApecoxPlayerCameraManager : APlayerCameraManager
```

新增私有成员：

```cpp
float FirstPersonNearClipPlane = 1.0f;
```

新增 protected 覆写：

```cpp
virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;
```

该覆写服务于当前仅由本地玩家使用的第一人称镜头。未来若加入第三人称操控、观战或独立 Cinematic ViewTarget，应重新确认这些镜头是否也需要 `1.0 cm` 近裁剪面。

## 构建状态

关闭 Unreal Editor 后，Codex 已重新执行 Development Editor 构建。`UnrealEditor-Apecox.dll` 链接和 Target Metadata 写入成功，UnrealBuildTool 最终结果为 `Succeeded`。

## 下一验证

按照 `Agent/00_Coordination/Current_UE_Manual_Steps.md` 完成完整构建和 PIE 回归。验证时不得输入 `r.SetNearClipPlane 1`，以确认效果来自代码。
