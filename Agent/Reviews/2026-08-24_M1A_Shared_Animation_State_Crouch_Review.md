# M1-A 共享动画状态基础与蹲伏输入复审

复审日期：2026-08-24  
结论：**代码审查与完整 Development Editor 构建均通过，无需子代理返工。**

## 审查结果

未发现需要修改的代码问题。

- `UApecoxCharacterAnimInstance` 只读取 Character、CMC 与 Equipment，没有维护第二份玩法或复制状态。
- UObject 访问全部位于游戏线程 `NativeUpdateAnimation()`，没有在线程安全更新阶段访问 UObject。
- `GroundSpeed`、`VerticalSpeed`、`MovementDirection`、移动/下落/蹲伏状态计算符合四向 Locomotion 和跳跃状态机需要。
- `AimPitch` 通过 `GetBaseAimRotation()` 计算；对于 Simulated Proxy，UE 5.8 的 `APawn::GetBaseAimRotation()` 会读取复制的 `RemoteViewPitch16`，因此不是仅本地有效。
- `AnimationFamily` 每帧由公开装备摘要和 Weapon Presentation Definition 派生；卸下或配置缺失时安全回退 `Unarmed`。
- `InputTag.Crouch` 只表达输入意图，没有新增重复的蹲伏状态 Tag。
- `Crouch()` / `UnCrouch()` 复用 CMC 内建预测与服务器校正，没有不必要的 RPC、GA 或 GE。
- 未增加 `AnimGraphRuntime` 依赖，也未修改射击、命中、弹匣、死亡和重生链路。

## 新增公开设计

### 类型

```text
EApecoxCharacterAnimationFamily
  - Unarmed
  - Rifle

UApecoxCharacterAnimInstance : UAnimInstance
```

### Presentation 字段

```cpp
EApecoxCharacterAnimationFamily EquippedAnimationFamily;
```

### AnimInstance 状态

```text
GroundSpeed
VerticalSpeed
MovementDirection
bIsMoving
bIsFalling
bIsCrouching
AimPitch
AnimationFamily
```

### Character 输入

```text
InputTag_Crouch = InputTag.Crouch
HandleCrouchStarted()
HandleCrouchCompleted()
```

CMC 配置：

```text
bCanCrouch = true
MaxWalkSpeedCrouched = 300.0
```

## 构建状态

用户关闭 Unreal Editor 后，Codex 重新执行完整 Development Editor 构建：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development
  -Project="D:/UnrealProject/Apecox/Apecox.uproject"
  -WaitMutex -NoHotReloadFromIDE
```

结果：

```text
Link [x64] UnrealEditor-Apecox.dll  Succeeded
WriteMetadata ApecoxEditor.target   Succeeded
Result: Succeeded
```

此前的 DLL 占用只是运行中的编辑器锁定文件，不是代码错误。

## 下一步

1. 按 `Current_UE_Manual_Steps.md` 完成 IA、IMC、InputConfig、Presentation DA 和 FP AnimBP 配置。
2. 先验证 CMC 蹲伏和共享状态，再进入 FP Locomotion 图表。
