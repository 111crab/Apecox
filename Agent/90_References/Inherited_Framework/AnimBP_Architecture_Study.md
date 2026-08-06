# Apex 自有 AnimBP 架构讨论稿

生成日期：2026-07-15
状态：架构方向、第一版范围与提议命名已获用户认可；尚未创建 C++ 类、AnimBP 或动画资产。

## 1. 当前决定与目标

已确认：Apex 不把 Paragon Phase 原有 `Phase_AnimBlueprint` 作为长期动画实现，而是建立项目自有 AnimBP。

“自有”表示：

- AnimInstance 基类、变量命名、状态机、Slot 和 GAS Tag 接入由 Apex 控制。
- 仍然使用 Phase 提供的 Skeleton、AnimSequence、BlendSpace、Montage 和 AimOffset。
- 不要求当前自行制作动画素材。

目标不是一次实现 Lyra 的全部动画复杂度，而是先建立能长期扩展的最小骨架。

## 2. 从 Lyra 借鉴什么

本地 Lyra UE 5.8 源码显示：

- `ULyraAnimInstance` 只承担稳定的运行时桥接，例如 GroundDistance 和 `FGameplayTagBlueprintPropertyMap`。
- Locomotion、姿势混合和具体动画资源主要留在 AnimBP。
- 武器通过 `FLyraAnimLayerSelectionSet` 按规则选择 Linked Anim Layer，而不是为每把武器复制完整主 AnimBP。
- ASC 在 Avatar 建立后重新初始化 AnimInstance 的 Tag 属性映射，适应 PlayerState Owner / Pawn Avatar 生命周期。

Apex 借鉴：

1. C++ AnimInstance 负责缓存稳定数据和 GAS 状态桥接。
2. AnimBP 负责姿势计算，不负责伤害、技能结算和权威玩法逻辑。
3. 当第二种武器姿势或角色模式真实出现时，再用 Linked Anim Layer 替换局部动画层。
4. 不复制 Lyra 的完整武器、Cosmetic Tag、Orientation Warping 和复杂动画管线。

## 3. 建议的分层

```text
AApexCharacterBase / CharacterMovement
        |
        v
UApexAnimInstance（C++，稳定状态桥）
        |
        v
ABP_Apex_Phase（Phase Skeleton，Apex 自有图）
  |- Locomotion State Machine
  |- Cached Locomotion Pose
  |- UpperBody Slot / Layered Blend Per Bone
  |- FullBody Slot
  `- Output Pose
```

### 3.1 `UApexAnimInstance`

建议作为项目公共 C++ AnimInstance 基类，缓存：

- `GroundSpeed`
- `MovementDirection`
- `VerticalSpeed`
- `bHasAcceleration`
- `bIsFalling`
- 后续需要时增加 `GroundDistance`、瞄准角度和稳定的 GAS 状态映射。

它不保存每个技能的 Montage，也不在 Tick 中激活 Ability。

可以把它理解为“玩法对象与动画图之间的状态翻译层”：CharacterMovement 提供 Velocity、Acceleration、MovementMode，`UApexAnimInstance` 将它们整理成 `GroundSpeed`、`bIsFalling` 等稳定变量，AnimBP 只读取这些变量选择姿势。

### 3.2 `ABP_Apex_Phase`

这是绑定 Phase Skeleton 的项目资产，父类是 `UApexAnimInstance`。

第一版状态机建议直接采用四个清晰状态：

```text
Grounded（Idle / Move 速度 BlendSpace）
-> Jump Start
-> Falling
-> Land
-> Grounded
```

Phase 已有 Idle、四向 Jog、Jump Start、Jump Apex、Jump Land、转身、AimOffset 和多组技能动作，可以创建 Apex 自己的 Locomotion BlendSpace 与 Montage。

当前 `AApexCharacterBase` 使用 `bOrientRotationToMovement = true`，角色会转向移动方向。因此第一版 `BS_Apex_Phase_Locomotion` 使用 1D Speed 轴即可：速度为 0 播 Idle，速度接近最大值时播放 Jog Forward，中间自动混合。Phase 的左、右、后退动画先保留。

以后加入锁定目标或瞄准模式，让角色保持面朝目标并允许横移、后退时，再建立 2D Strafe BlendSpace：一个轴是相对移动方向，另一个轴是速度。此时才真正需要 Jog Left、Jog Right 和 Jog Backward。

## 4. 哪些内容适合配置

### 4.1 适合按角色或姿势配置

- Idle / Jog / Jump / Land 动画序列。
- Locomotion BlendSpace。
- AimOffset。
- Turn In Place 动画。
- 武器或姿势对应的 Linked Anim Layer 类。
- 技能、受击、眩晕、死亡等离散动作的 Montage。

### 4.2 不应字段化的内容

- Grounded、InAir 等稳定状态机语义。
- 上半身/全身混合顺序。
- Montage Slot 的职责。
- GAS Montage 复制、技能 Commit、伤害和取消逻辑。

这些是动画框架或 GAS 的稳定规则，不应让每个角色重新配置一遍。

## 5. 基础动作不应该全部使用 Montage

| 动画类型 | 推荐机制 |
| --- | --- |
| Idle、Walk、Run、空中循环 | State Machine + Sequence / BlendSpace。 |
| Jump Start、Land | 状态机中的短 Sequence；特殊重落地可用 Montage。 |
| 技能施法、攻击、受击、眩晕、死亡 | Montage。 |
| 持枪/弓箭/法术姿势差异 | Linked Anim Layer 或同骨骼的 AnimBP 子实现。 |
| 上半身施法同时移动 | UpperBody Slot + Layered Blend Per Bone。 |
| 锁定移动的全身技能 | FullBody Slot，移动限制仍由玩法状态处理。 |

Montage 是离散、有开始结束和事件点的动作载体，不适合作为持续 Locomotion 的配置容器。

## 6. 不同角色怎样复用

### 相同 Skeleton

可以复用同一个 AnimBP 框架，并通过以下方式替换资产：

- AnimBP 子类的 Asset Override。
- 角色动画配置引用。
- Linked Anim Layer。

### 不同 Skeleton

AnimBP 和 Montage 不能因为字段指向不同资产就自动跨 Skeleton 工作。需要：

1. IK Retarget 动画资源。
2. 为目标 Skeleton 创建或重定向对应 AnimBP / Montage。
3. 继续继承同一个 `UApexAnimInstance` C++ 基类和相同的图职责规范。

因此“逻辑框架可以复用”和“同一个 AnimBP 资产能绑定所有人物”是两件事。

## 7. 技能 Montage 放在哪里

Locomotion 配置不保存技能 Montage。

建议边界：

- SkillDefinition 的表现配置选择该技能使用的 Montage。
- AnimBP 只提供约定好的 `UpperBody` / `FullBody` Slot 和姿势混合能力。
- Montage 中的关键 GameplayEvent Notify 驱动 GA 的技能时机。
- GAS 使用 `PlayMontageAndWait` 或项目封装 Task 负责多人 Montage 播放与结束/取消。

同一技能逻辑给不同骨骼角色使用时，可以创建不同 SkillDefinition 表现实例，或者以后增加 Hero Presentation Override；不能把一个只适配 Phase Skeleton 的 Montage 强行给其他 Skeleton 使用。

## 8. Apex 第一版建议范围

第一版只建立：

1. `UApexAnimInstance` 公共基类。
2. `ABP_Apex_Phase` 项目自有 AnimBP。
3. 一个 Phase 速度型 1D Locomotion BlendSpace。
4. Grounded / InAir 最小状态机。
5. `UpperBody` 与 `FullBody` 两个职责明确的 Slot。
6. Idle、移动、跳跃和落地 PIE 验证。

已通过 Phase Skeleton 和现有攻击 Montage 的资产元数据确认：原始 `phase_Skeleton` 已包含 `DefaultSlot`、`UpperBody`、`FullBody` 等 Slot，`Primary_Attack_A_Medium_Montage` 使用 `UpperBody`。第一版复用这些既有 Slot，不修改第三方 Skeleton。进入 UE 时仍需在 Skeleton Slot Manager 中做一次可视化确认。

暂不建立：

- 通用 AnimationProfile DataAsset。
- 多武器 Linked Anim Layer 选择系统。
- Motion Matching、Foot IK、复杂转身和距离匹配。
- 技能 Montage 与 GameplayEvent Notify。

等第二名角色或第一种武器姿势出现时，再用真实差异决定 AnimationProfile 或 Linked Anim Layer 的字段，避免提前造出另一个万能动画配置。

## 9. 已认可的第一版命名

当前采用：

| 类型 | 名称 | 作用 |
| --- | --- | --- |
| C++ 类 | `UApexAnimInstance` | 缓存移动状态，后续桥接 GAS Tag。 |
| AnimBP | `ABP_Apex_Phase` | Phase Skeleton 的 Apex 自有主动画蓝图。 |
| BlendSpace | `BS_Apex_Phase_Locomotion` | Phase 四向地面移动。 |
| State Machine | `Locomotion` | 地面与空中基础状态。 |
| Slot | `UpperBody` | 可移动施法/攻击。 |
| Slot | `FullBody` | 全身覆盖动作。 |

下一步先写入事前代码设计供用户最终审阅，再进入子代理 Prompt 和 UE 人工操作清单。
