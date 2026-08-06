# Phase 玩家验证角色替换与基础移动计划

更新时间：2026-07-13

## 决策摘要

下一阶段先把 `ParagonPhase` 接入为玩家侧技能验证角色，完成基础移动和基础动画绑定，然后再继续设计新技能。

这不是最终游戏角色定型，也不是马上改成完整第三人称射击/英雄视角。它的目标更朴素：让我们有一个稳定的法术验证角色，用来承载后续火球、引导治疗、范围场、光束、链接等技能表现。

## 为什么先做 Phase

Phase 比当前 Aura 默认角色更适合作为法术验证角色：

- 有完整人物资产、骨骼、AnimBP 和 Character Blueprint 候选。
- 有法术语义更强的动作，例如 `Cast`、`RMB_Throw`、`RMB_Loop`。
- 适合验证手部 socket、施法 Montage、GameplayEvent Notify、投射物生成和法术 VFX。
- 后续可以作为技能编辑器预览角色候选。

## 阶段边界

### 本阶段要做

1. 确认 Phase 关键资产在 UE 编辑器内可打开、可预览。
2. 选择接入方式：
   - 优先方案：继续使用 Aura 的玩家 C++/GAS 路径，只替换 Mesh、AnimClass、相对位置和缩放。
   - 备选方案：如果 Phase 自带 Character Blueprint 更容易验证，则先做独立测试蓝图，但不能绕开 Aura 的长期 GAS 玩家路径。
3. 完成基础 locomotion：
   - Idle
   - Walk / Run
   - Jump / Falling / Land，如果现有 AnimBP 已支持则直接验证；不支持则先记录缺口。
4. 确认移动输入仍正常：
   - WASD 或当前 Aura 移动输入可移动角色。
   - 角色不会出现 Capsule 和 Mesh 错位、脚底悬空过大、朝向异常。
5. 确认 Phase 能进入 PIE 并作为玩家角色被 Possess。
6. 保留 Aura GAS 初始化链路：
   - `AAuraCharacter::PossessedBy`
   - `AAuraCharacter::OnRep_PlayerState`
   - PlayerState ASC ownership 不被破坏。
7. 输出中文实施报告，记录改动的蓝图/资产/代码路径和验证结果。

### 本阶段不做

- 不设计新技能。
- 不重构技能运行时。
- 不做完整准星视角。
- 不批量 retarget 全部动画。
- 不直接修改第三方原始动作资产的 Notify 作为长期方案。
- 不把 Phase 硬定为最终主角。
- 不为了动作表现临时破坏 Aura 原有 GAS、输入、UI 初始化链路。

## 接入策略

### 推荐策略：Aura Character 承载 Phase Mesh

Phase 最好先作为 `AAuraCharacter` 的表现层替换，而不是把 Phase 自带 Character Blueprint 直接变成主玩家类。

原因：

- Aura 当前 GAS、ASC、属性、输入、UI 都围绕 `AAuraCharacter` / `AAuraPlayerState` 建立。
- 直接换成 Phase 自带 Character Blueprint，可能绕过 Aura 的 GAS 初始化路径。
- 我们现在要验证的是“Phase 能成为技能系统载体”，不是研究第三方角色蓝图自身逻辑。

具体含义：

- GameMode / PlayerStart / 默认 Pawn 仍走 Aura 玩家角色路径。
- 玩家角色的 `Mesh` 使用 Phase 的 SkeletalMesh。
- 玩家角色的 `AnimClass` 使用 Phase 的 AnimBlueprint。
- 根据 Phase 模型调整 Mesh 的 Location、Rotation、Scale，使 Capsule 正常包住角色。
- 必要时新建一个 `BP_AuraCharacter_PhasePrototype`，继承 Aura 现有玩家蓝图，用于测试，不直接覆盖旧 `BP_AuraCharacter`。

## 视角策略

本阶段先不做完整准星视角。

短期目标：

- 当前 Aura 相机仍能看清 Phase。
- 移动和动画先稳定。
- 如果 Phase 动作在当前视角下能看清，就先继续使用。

后续独立阶段再做：

- 近第三人称相机。
- 屏幕中心小准星。
- 鼠标 yaw/pitch 控制视角。
- 准星 Trace 作为投射物/光束目标来源。
- 角色朝向采用控制器朝向或移动朝向的最终取舍。

## 动画原则

基础移动阶段只要求 locomotion 正常，不要求技能 Montage 完整。

需要关注：

- AnimBP 是否能读取速度、是否落地、移动方向等变量。
- Phase 自带 AnimBP 是否依赖它自己的 Character Blueprint 变量。
- 如果 Phase AnimBP 只依赖通用 Pawn/MovementComponent，则接入 Aura Character 成本低。
- 如果 Phase AnimBP 依赖自定义 Phase 角色变量，需要决定是补变量、改 AnimBP，还是创建一个更简单的 Aura Phase AnimBP。

对于后续技能：

- 技能关键帧仍走 Montage Notify / GameplayEvent Notify。
- 普通音效/VFX Notify 仍留在 Montage 或表现层，不承担 GA 关键逻辑。
- 如果使用 Phase 自己骨架，不需要 retarget 到 Aura 默认角色。

## 验收标准

### 单人 PIE

- Phase 作为玩家角色出生。
- 摄像机能正常跟随。
- WASD 或现有移动输入正常。
- Idle/Run 动画能切换。
- 跳跃/下落如果现有项目支持，应不破坏；如果暂不支持，报告中说明。
- 血条、属性初始化、ASC 初始化没有明显报错。

### Listen Server / Client

- 主机和客户端都能看到 Phase 角色。
- 远端能看到角色移动复制。
- 基础动画在远端可见。
- 不要求本阶段验证技能效果，但不能破坏已有火球/治疗输入链路。

### 资产与工程

- 不移动第三方原始资源。
- 如果新建试验蓝图/AnimBP，应放入 Aura 自己目录，例如 `/Game/Aura/SkillPrototype/Phase`。
- 报告列出所有新增/修改资产。

## 风险清单

| 风险 | 影响 | 处理 |
| --- | --- | --- |
| Phase AnimBP 依赖自定义角色变量 | 动画不动或报错 | 优先检查 AnimBP 变量来源，必要时新建简单 AnimBP |
| Mesh 和 Capsule 尺寸不匹配 | 碰撞、脚底、相机异常 | 调整 Mesh transform，不改 Capsule 为奇怪尺寸 |
| 直接替换默认玩家蓝图风险大 | 旧角色回退困难 | 优先新建 `BP_AuraCharacter_PhasePrototype` |
| 多人远端动画不同步 | 影响后续技能验证 | 至少做 Listen Server + Client 基础移动检查 |
| 相机同时重构 | 调试面过大 | 本阶段只保证能看清 Phase，准星视角后置 |

## 完成后进入的下一阶段

Phase 基础移动通过后，再进入两个可选方向：

1. **Phase 技能表现接入**：创建 Phase 火球 Montage，配置 GameplayEvent Notify，用 Phase 复验火球。
2. **近第三人称准星视角**：为 Phase 验证角色添加准星相机、准星 Trace、角色朝向策略。

推荐顺序：先做 Phase 技能表现接入，再做准星视角。因为技能系统本身仍是当前主线。
