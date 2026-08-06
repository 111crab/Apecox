# Phase 玩家验证角色替换与基础移动 — 实施报告

*创建日期：2026-07-13*
*执行者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-13_Phase_PlayerReplacement_BasicMovement.md*

## 1. 摘要

通过 MCP 创建了 `BP_AuraCharacter_PhasePrototype`（继承 `BP_AuraCharacter`，保留完整 Aura GAS/ASC/输入/UI 初始化链路）。由于 MCP 无法设置 Blueprint CDO 的组件子属性（`SkeletalMeshComponent::SkeletalMesh` / `AnimClass`）以及 Phase AnimBlueprint 存在编译错误，剩余配置需在 UE 编辑器内手动完成。

## 2. 已完成的 MCP 操作

| 操作 | 结果 |
|------|------|
| MCP 连接 | ✅ `unreal-mcp v0.6.0` |
| 检查 BP_AuraCharacter CDO | ✅ Mesh: SKM_Aura, AnimClass: ABP_Aura, 8 个组件 |
| 检查 PhasePlayerCharacter CDO | ⚠️ 编译错误（`ResetOrientationAndPosition` 缺失），父类 `Character`（非 AuraCharacter） |
| 检查 Phase_AnimBlueprint | ❌ 编译错误：Skeleton asset missing |
| 创建 BP_AuraCharacter_PhasePrototype | ✅ duplicate_asset 成功，路径 `/Game/Aura/SkillPrototype/Phase/` |
| 设置 SkeletalMesh | ❌ MCP `set_property` 无法设置组件子属性 |
| 保存 | ✅ 1 个内容包已保存 |

## 3. 新增资产

| 资产路径 | 类型 | 说明 |
|----------|------|------|
| `/Game/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype` | Blueprint | 基于 BP_AuraCharacter 的 Phase 原型，继承 AAuraCharacter → AuraCharacterBase → ACharacter |

**未修改**：旧 `BP_AuraCharacter`、Aura C++ 代码、第三方原始资产。

## 4. 使用的 Phase 资产路径

| 资产 | 路径 |
|------|------|
| SkeletalMesh | `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC` |
| Skeleton | `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/phase_Skeleton` |
| AnimBlueprint | `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Phase_AnimBlueprint`（当前有编译错误） |
| Character BP（参考） | `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/PhasePlayerCharacter`（有编译错误，不使用） |
| Prototype BP（新建） | `/Game/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype` |

## 5. GAS 初始化链路状态

`BP_AuraCharacter_PhasePrototype` 继承 `BP_AuraCharacter` → `AAuraCharacter` → `AAuraCharacterBase`（IAbilitySystemInterface + ICombatInterface）。

以下初始化链路完全保留：
- `AAuraCharacter::PossessedBy` — 服务器 ASC 初始化
- `AAuraCharacter::OnRep_PlayerState` — 客户端 ASC 初始化
- PlayerState ASC ownership — 不改变
- 输入绑定（`SetupInputComponent` / `AuraInputComponent`）— 不改变
- UI / WidgetController — 不改变
- 属性初始化 GE — 不改变
- `StartupAbilities` / `StartupSkillDefinitions` — 不改变

## 6. 手动完成步骤（需在 UE 编辑器中操作）

### 步骤 A：修复 Phase AnimBlueprint Skeleton

1. 打开 `Content/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Phase_AnimBlueprint`
2. 在 Class Settings → 将 Target Skeleton 设为 `phase_Skeleton`
3. 编译并保存
4. 如果 AnimBlueprint 有其他编译错误（蓝图节点缺失），酌情修复或跳过

### 步骤 B：配置 BP_AuraCharacter_PhasePrototype Mesh

1. 打开 `Content/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype`
2. 在 Components 面板选中 `CharacterMesh0 (Inherited)`
3. Details 面板设置：
   - **Skeletal Mesh** → `Phase_GDC`
   - **Anim Class** → `Phase_AnimBlueprint_C`（步骤 A 修复后）
4. 调整 Transform（适配 Phase 模型身材）：
   - Location Z: `-97`（Phase 原始值，如脚悬空再微调）
   - Rotation Z: `0`（Phase 原始 Yaw 270°，改为朝向相机方向；暂不改也可）

### 步骤 C：移除或隐藏 Weapon 组件

Phase 角色自身不需要 Aura 的 Staff 武器：
1. 选中 `Weapon (Inherited)` 组件
2. 如果编辑器允许，清空 Skeletal Mesh 或隐藏该组件
3. 或在 Details 中将 `Skeletal Mesh` 设为 None

### 步骤 D：配置测试 GameMode

1. 打开 `Content/Blueprints/GameModes/BP_AuraGameModeBase`（或当前使用的 GameMode）
2. 在 Details 中设置：
   - **Default Pawn Class** → `BP_AuraCharacter_PhasePrototype`
3. 或在地图 `StartupMap` 的 World Settings 中覆写 GameMode 的 Default Pawn

### 步骤 E：单人 PIE 验证

1. 启动 PIE（Selected Viewport 或 New Editor Window）
2. 验证：
   - Phase 角色正常生成
   - 摄像机跟随正常
   - WASD 移动可用
   - Idle / Walk / Run 动画播放（如 AnimBP 已修复）
   - Capsule 和 Mesh 没有明显错位
3. Output Log 中无新 GAS 初始化错误

### 步骤 F：回退路径

如 Phase 原型有问题，将 GameMode Default Pawn 改回 `BP_AuraCharacter` 即可恢复。

## 7. 当前阻塞问题

| 阻塞 | 影响 | 解决方案 |
|------|------|----------|
| MCP 无法设置 SkeletalMesh/AnimClass | 无法完全自动化 | 手动步骤 B |
| Phase_AnimBlueprint 编译错误 | 无动画播放 | 手动步骤 A 修复 Skeleton |
| PhasePlayerCharacter 编译错误 | 不可直接复用 | 不影响（我们不走此路径） |
| Socket 未确认 | 后续技能无法确定发射点 | 编辑器打开 phase_Skeleton 查看 |

## 8. 未验证项

- **单人 PIE**：未执行（需 UE 编辑器手动完成配置后）
- **Listen Server / Client**：未执行
- **Jump / Falling / Land 动画**：待 AnimBP 修复后验证

## 9. 下一步建议

1. **立即**：在 UE 编辑器中完成手动步骤 A-D
2. **验证**：PIE 确认 Phase 移动和动画正常
3. **Socket 检查**：打开 `phase_Skeleton`，记录手部 socket
4. **阶段 C**：创建 Phase 测试 Montage + GameplayEvent Notify
5. **阶段 D**：用 Phase 重新验证火球术链路
