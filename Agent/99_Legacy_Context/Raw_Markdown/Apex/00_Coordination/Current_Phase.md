# 当前局部阶段

更新日期：2026-08-05

## 顶层阶段

Phase 4 - 第一批真实技能原型与共用战斗基线。

## 上一阶段完成

- Phase Energy Bolt 已完成 SkillDefinition、ProjectileCast GA、Montage GameplayEvent、ViewCenter TargetData、服务器投射物、Damage GE 与 Impact GameplayCue 完整闭环。
- 单人和 Listen Server + Client PIE 全部通过。
- 功能提交：`7373f26 feat: add Phase Energy Bolt projectile ability`。

## 已完成的当前任务

在第二个技能开始前，建立第三人称瞄准与朝向基线：

```text
中心准星
-> Controller Yaw 驱动玩家朝向
-> 四向扫射 Locomotion
-> ViewCenter TargetData
-> Socket 朝准星目标发射
```

## 已确认

1. 准星由项目自有 Gameplay HUD 显示。
2. 玩家角色水平朝向持续跟随鼠标控制的视角。
3. WASD 保持相机相对移动并转为扫射模型。
4. 技能继续依据视口中心，而非鼠标光标坐标。
5. 投射物继续从手部 Socket 朝相机中心 AimPoint 发射。
6. Phase 使用已有 `Jog_Fwd / Jog_Bwd / Jog_Left / Jog_Right` 构建四向 BlendSpace。
7. 本阶段不实现 Turn In Place、Aim Offset、Orientation Warping 或技能级 FacingPolicy。
8. UMG 和 BlendSpace 由用户在代码审查通过后手动配置，不使用 MCP。
9. 新建 `BP_ApexGameMode` 与 `BP_ApexPlayerController` 作为项目正式入口，不再继续修改官方 ThirdPerson 示例蓝图。

## 当前状态

- Lyra 朝向模型、Apex 当前 Character/AnimInstance/TargetData/PlayerController 已完成核对。
- Phase 四向 Jog 与 Turn 动画候选已完成盘点。
- `Current_Code_Design.md` 已覆盖为本阶段设计。
- ClaudeCode 已完成 HUD 生命周期、玩家朝向、`MovementDirection` 与 ViewCenter Task 重命名。
- Codex 已完成代码审查和独立编译，`ApexEditor Win64 Development` 构建成功。
- 用户已完成 HUD、项目自有 GameMode/PlayerController、四向 BlendSpace 和 AnimBP 配置。
- 中心准星、角色朝向、四向移动、Energy Bolt 单人及 Listen Server/Client 验证均符合预期。

## 已批准但尚未实施

- Energy Bolt 仅修正动画表现：优先更换正面施法动作，AimOffset 作为后备；不修改正确的弹道链路。
- 调整启动、停止、跳跃、空中动量等 CharacterMovement 手感参数。
- 调整 SpringArm 构图、碰撞、FOV 与视角 Pitch 边界，并预留薄层 `AApexPlayerCameraManager`。
- 完整参数与边界记录在 `Decision_Log.md` 的 `2026-08-05` 条目。

## 当前下一步

暂停第二技能和手感优化实施，先进行一轮项目级讨论。讨论完成后再更新本文件，确定顶层阶段、近期目标和恢复实施的顺序。

## 实施顺序

1. ClaudeCode 完成 C++ HUD 入口、玩家朝向、MovementDirection 与 ViewCenter Task 重命名。
2. Codex 审查公开命名、生命周期、动画数据计算和 GAS TargetData 网络路径。
3. Codex 独立编译。
4. 覆盖 `Current_UE_Manual_Steps.md`。
5. 用户创建 GameplayHUD/Crosshair、配置 PlayerController 并更新 Phase 四向 BlendSpace。已完成。
6. 用户完成单人和多人 PIE。已完成。
7. 验证通过后形成一个功能提交并 push。正在收口。

## Git 状态

本阶段代码、文档和项目自有 UE 资产已暂存，准备以第三人称瞄准基线功能提交收口。
