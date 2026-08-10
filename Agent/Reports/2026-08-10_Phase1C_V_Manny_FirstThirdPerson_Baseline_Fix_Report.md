# Phase 1C-V 第一人称姿态复制骨骼刷新小修报告

**日期**：2026-08-10  
**审查来源**：Codex — `Agent/Reviews/2026-08-10_Phase1C_V_Manny_FirstThirdPerson_Baseline_Review.md`  
**状态**：等待 Codex 复审

---

## 一、修改文件

| 文件 | 修改内容 | 对应审查问题 |
|------|----------|-------------|
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 2 处修改 | P1 + P2 |

仅修改此一个文件，其余所有源码未动。

---

## 二、逐问题修复详情

### P1：隐藏的第三人称源 Mesh 没有保证刷新骨骼

**位置**：构造函数 `ThirdPersonMesh` 配置块（约第 40 行附近）

**问题**：`ABP_FP_Copy` 通过 `Copy Pose From Mesh` 从 `GetMesh()` 读取组件空间骨骼姿态。拥有者本地的 `GetMesh()` 因 `OwnerNoSee` 不渲染，`ACharacter` 默认的 `EVisibilityBasedAnimTickOption::AlwaysTickPose` 在未渲染时只更新动画 Pose，不保证每帧刷新组件空间骨骼变换。第一人称复制 Mesh 可能读到未刷新的姿态，表现为手臂停在参考姿势或跳帧。

**修复**：在 `SetOwnerNoSee(true)` 和 `FirstPersonPrimitiveType` 之后追加：

```cpp
ThirdPersonMesh->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

附带简短中文注释，解释该配置服务于隐藏源 Mesh 的第一人称姿态复制需求。

**依据**：UE 5.8 官方 `BP_FirstPersonCharacter` 对源 Mesh 显式使用相同配置。

---

### P2：补齐 `UCollisionProfile` 的直接头文件

**位置**：文件顶部 includes 区域

**问题**：代码使用了 `UCollisionProfile::NoCollision_ProfileName`，但没有直接包含 `Engine/CollisionProfile.h`。当前构建因传递包含侥幸通过，不符合 IWYU 原则，后续引擎头文件关系变化可能造成编译失败。

**修复**：在 `Camera/CameraComponent.h` 之后添加：

```cpp
#include "Engine/CollisionProfile.h"
```

---

## 三、构建结果

**构建命令**：
```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

**结果**：✅ **Succeeded**（0 错误，0 新增警告）

---

## 四、未做操作声明

- ❌ 未修改 `ApecoxPlayerCharacter.h`
- ❌ 未修改 GameplayTags、HealthComponent、VitalAttributeSet、DeathAbility、GameMode、ASC 或输入组件
- ❌ 未创建/修改/删除任何 `.uasset` 或 `.umap`
- ❌ 未操作 UE 编辑器
- ❌ 未执行任何 Git 命令
- ❌ 未调用 MCP

---

**修复完成，等待 Codex 复审。**
