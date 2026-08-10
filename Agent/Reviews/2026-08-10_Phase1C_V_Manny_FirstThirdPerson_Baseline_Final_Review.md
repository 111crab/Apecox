# Phase 1C-V Manny 第一/第三人称角色基线最终复审

日期：2026-08-10

结论：**通过，可以进入 UE 资产配置与运行验证。**

## 修复确认

1. `AApecoxPlayerCharacter` 构造函数已将隐藏的第三人称源 Mesh 设置为
   `EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones`。
   这保证拥有者本地即使不渲染第三人称 Mesh，`ABP_FP_Copy` 仍能读取已经刷新的组件空间骨骼姿态。
2. `ApecoxPlayerCharacter.cpp` 已直接包含 `Engine/CollisionProfile.h`，符合 IWYU。
3. 本次小修只修改 `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`，没有改动既有输入、ASC、Health、Death 或 Respawn 逻辑。
4. `ApecoxEditor Win64 Development` 完整构建成功。

## 复审判断

- 第一/第三人称双 Mesh 只拆分表现，仍共享一个 Character、Capsule 与 CMC。
- `OwnerNoSee`、`OnlyOwnerSee` 与骨骼刷新策略形成了完整的本地/远端可见性闭环。
- 第一人称姿态复用第三人称 AnimBP 的结果，不重复模拟移动状态。
- Move、Look、Jump 继续走 Native Input 配置层和 CMC 原生网络预测，没有新增移动 RPC。

本轮没有遗留必须在进入 UE 编辑器前修复的代码问题。运行结果仍需按当前人工清单验证。
