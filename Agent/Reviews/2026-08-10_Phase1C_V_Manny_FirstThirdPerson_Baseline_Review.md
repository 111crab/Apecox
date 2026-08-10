# Phase 1C-V Manny 第一/第三人称角色基线审查

日期：2026-08-10

结论：**需要一次很小的代码修复后再进入 UE 配置。**

## P1：隐藏的第三人称源 Mesh 没有保证刷新骨骼

位置：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`，构造函数的 `ThirdPersonMesh` 配置。

`ABP_FP_Copy` 使用 `Copy Pose From Mesh`，并通过 `bUseAttachedParent` 从附着父组件 `GetMesh()` 读取组件空间骨骼姿态。拥有者本地的 `GetMesh()` 又被设置为 `OwnerNoSee=true`。

`ACharacter` 在引擎构造函数中把默认 Mesh 设置为 `EVisibilityBasedAnimTickOption::AlwaysTickPose`。这能在未渲染时更新动画 Pose，但不保证每帧刷新组件空间骨骼变换。第一人称复制 Mesh 可能因此读到未刷新的姿态，表现为手臂停在参考姿势、只在偶发帧更新，或 Run/Jump 姿态不跟随。

UE 5.8 官方 `BP_FirstPersonCharacter` 对源 Mesh 显式配置：

```cpp
ThirdPersonMesh->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

该配置应成为双 Mesh 架构的 C++ 不变量，而不是依赖用户在蓝图中记住一个隐蔽选项。

## P2：补齐 `UCollisionProfile` 的直接头文件

当前位置使用了：

```cpp
UCollisionProfile::NoCollision_ProfileName
```

但 `.cpp` 没有直接包含 `Engine/CollisionProfile.h`。当前完整构建因传递包含而成功，但不符合 IWYU，后续引擎头文件关系变化可能造成编译失败。

## 已通过项

- `FirstPersonMesh` / `FirstPersonCamera` 命名和职责符合已批准设计。
- `OnlyOwnerSee` / `OwnerNoSee` 分层正确。
- Camera 附着、First Person FOV/Scale 参数与 UE 5.8 官方模板一致。
- 使用单一 Character、Capsule 和 CMC，没有重复移动模拟或新增 RPC。
- Move、Look、Jump 使用现有 Native InputAction 配置层，没有污染 Ability 输入路由。
- Tactical Ability 输入、ASC、Health、Death、Respawn 逻辑没有被本轮重新改写。
- `SetupPlayerInputComponent` 对同一 Pawn InputComponent 只在创建时执行；当前 Native 绑定无需额外句柄管理。
- `ApecoxEditor Win64 Development` 构建成功。

## 修复后通过条件

1. `GetMesh()` 显式设置 `AlwaysTickPoseAndRefreshBones`，并有简短中文注释解释它服务于隐藏源 Mesh 的第一人称姿态复制。
2. `.cpp` 直接包含 `Engine/CollisionProfile.h`。
3. 重新完整构建成功。
4. 更新原实施报告或新增小修报告，说明修改和构建结果。

