# Phase 法术验证角色资产预检报告

*创建日期：2026-07-13*
*执行者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-13_PhaseSpellValidation_Precheck.md*

## 1. 结论摘要

**Phase 可以直接作为法术验证角色。** 不需要强制 retarget。

### 关键发现

- ✅ Phase 有完整的 SkeletalMesh、Skeleton、AnimBlueprint 和 Character Blueprint
- ✅ 存在 5 个可用的 AnimMontage（已由第三方制作）
- ✅ 存在可直接复用的动作序列：`Cast`（施法）、`RMB_Throw`（发射）、`RMB_Loop`（引导循环）
- ✅ 有大量 Niagara VFX 候选：Free_Magic（投射物/命中）和 Free_Spells（治疗/Buff）
- ⚠️ Socket 信息无法在编辑器外确认，需在 UE 编辑器中打开 Skeleton 查看
- ⚠️ 动作真实类型（AnimSequence vs AnimMontage）基于文件大小和命名推断，需编辑器确认

### 当前最大风险

1. **Socket 缺失**：如果 Phase Skeleton 没有手部 socket，火球和光束的生成点需要手动添加（下一阶段可做）
2. **动作幅度**：Paragon 是第三人称俯视视角游戏，动作可能偏大，需在 UE 编辑器中预览确认是否适合我们的视角
3. **Cascade vs Niagara**：Paragon 附带的 `P_*` 特效大概率是 Cascade 粒子，不适合长期使用；但 Free_Magic/Free_Spells 的 Niagara 资源可绕过此问题

## 2. 资产类型表

| 路径 | 推测类型 | 确认方式 | 备注 |
|------|----------|----------|------|
| `Meshes/Phase_GDC.uasset` | **SkeletalMesh** | 文件尺寸推断（11.6MB，含几何数据） | 角色主体网格 |
| `Meshes/phase_Skeleton.uasset` | **Skeleton** | 文件尺寸推断（44KB，骨骼结构） | 骨骼资产 |
| `Meshes/Phase_Extents.uasset` | SkeletalMesh（LOD） | 文件尺寸推断（111KB） | 可能是简化碰撞/范围网格 |
| `Meshes/Paragon_Proto_Retarget.uasset` | Retarget 配置 | 命名推断 | 可重定向到 UE5 标准人形的参考 |
| `Phase_AnimBlueprint.uasset` | **AnimBlueprint** | 文件尺寸推断（574KB，蓝图逻辑） | ABP 已存在 |
| `PhasePlayerCharacter.uasset` | **Blueprint（Character）** | 文件尺寸推断（288KB） | 测试角色蓝图已存在 |

## 3. 动作候选表

### 3.1 法术施法类动作（优先级最高）

| 动作路径 | 推测类型 | 文件大小 | 推荐用途 | 原因 |
|----------|----------|----------|----------|------|
| `Animations/Cast.uasset` | AnimSequence | 小 | **火球术施法** | 明确法术施法动作，名称直观，适合作为火球/飞弹施法 Montage 的源 |
| `Animations/Cast_MSA.uasset` | AnimMontage | 小 | **火球术施法 Montage**（已有） | `MSA` 可能指 Montage Sequence Asset，如果 UE 编辑器确认类型则直接可用 |
| `Animations/RMB_Throw.uasset` | AnimSequence | 小 | **发射/投掷关键帧** | 适合用作发射事件帧；配合 Cast 可以组成"蓄力→发射" |
| `Animations/RMB_Intro.uasset` | AnimSequence | 小 | 技能启动/前摇 | 适合需要前摇的蓄力技能 |
| `Animations/RMB_Loop.uasset` | AnimSequence | 小 | **引导治疗/持续施法** | 循环动作，适合 ChannelCast 或持续引导技能 |
| `Animations/R_Ability_Intro.uasset` | AnimSequence | 小 | 大招启动 | 如果后续做大招模板可复用 |
| `Animations/R_Ability_Loop.uasset` | AnimSequence | 小 | 大招循环/持续 | 持续施法的大招版本 |

### 3.2 已存在的 Montage（可直接或参考使用）

| Montage | 推测用途 | 是否可直接复用 |
|---------|----------|---------------|
| `LevelStart_Montage.uasset` | 出场/入场动画 | 不推荐给技能使用 |
| `Primary_Attack_A_Medium_Montage.uasset` | 普攻 A 连段 | 可参考，但近战普攻，不适合法术 |
| `Primary_Attack_B_Medium_Montage.uasset` | 普攻 B 连段 | 同上 |
| `Primary_Attack_C_Medium_Montage.uasset` | 普攻 C 连段 | 同上 |
| `Primary_Attack_D_Medium_Montage.uasset` | 普攻 D 连段 | 同上 |

**关于 `Cast_MSA.uasset`**：如果 UE 编辑器确认它是 AnimMontage（且包含 Cast AnimSequence），则它可能已经是"施法 Montage"的最直接候选。如果它不是 Montage，下一阶段应基于 `Cast` + `RMB_Throw` 创建 `AM_Phase_Cast_Fireball_Test`。

### 3.3 不推荐的动作

- 普攻系列（`Primary_Attack_*`）：近战动作，不适合法术
- 跑步/跳跃/受击系列：移动和反馈用，暂时不急需
- 表情（`Emote_*`）：完全不相关

## 4. Socket / 发射点检查

**当前状态：无法在编辑器外确认。**

Phase Skeleton (`phase_Skeleton.uasset`) 的 socket 列表需要 UE 编辑器的 Skeleton Tree 面板查看。

### 预测

- Paragon 角色通常会在双手（`hand_r`、`hand_l`）或武器骨骼上挂 socket
- `Phase_GDC` 的武器在右手，推测右手有 `weapon` 或 `hand_r` socket

### 建议

| 场景 | 推荐 Socket | Plan B |
|------|------------|--------|
| 火球发射点 | 右手 socket（`hand_r` 或 `weapon`） | 左手；或从 Spine/Chest 正前方偏移 |
| 光束起点 | 双手之间 | 单手；或 Camera forward 方向 |
| 治疗附着 | 自身 Root/Chest | 头顶 Aura 位置 |

### 如果没有 Socket

下一阶段可在 Phase Skeleton 上添加一个 socket（例如 `FX_R_Hand_Cast`），**不修改任何第三方原始动作**，只是在 Skeleton 资产上加一个骨骼相对偏移。这是 UE 编辑器中安全且常见的操作。

## 5. VFX 候选表

### 5.1 投射物/命中（Free_Magic）

| 路径 | 推测类型 | 推荐用途 |
|------|----------|----------|
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1` | Niagara System | **火球飞行主体** |
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile2` | Niagara System | 备选投射物（不同颜色/形状） |
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1` | Niagara System | **火球命中爆炸** |
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2` | Niagara System | 备选命中效果 |
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Area1` | Niagara System | 范围场持续效果 |
| `Free_Magic/VFX_Niagara/NS_Free_Magic_Circle1` | Niagara System | 法阵/光环 |

### 5.2 治疗/Buff（Free_Spells）

| 路径 | 推测类型 | 推荐用途 |
|------|----------|----------|
| `Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Healing` | Niagara System | **引导治疗自身光环** |
| `Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing` | Niagara System | 治疗完成 Buff 效果 |
| `Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire` | Niagara System | 火焰 Buff 光环 |
| `Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion` | Niagara System | 范围爆炸 |

**注意**：Free_Magic/Free_Spells 的 `VFX_Niagara` 命名可信度较高，但最终仍需在 UE 编辑器中确认真实类型。两个资产包还包含 `Mannequin`/`RTG_Mannequin`，可能是 UE5 人形骨架的重定向参考，对 retarget 有帮助。

## 6. 下一阶段建议

### 6.1 立即在编辑器中确认（不写代码）

1. **打开 `phase_Skeleton`** → 查看 Socket 列表 → 截图记录手部 socket
2. **打开 `Cast_MSA`** → 确认类型（Montage vs Sequence）→ 预览动作内容
3. **打开 `Cast`、`RMB_Throw`、`RMB_Loop`** → 确认类型 → 预览动作是否符合预期
4. **打开 `NS_Free_Magic_Projectile1` 和 `NS_Free_Magic_Hit1`** → 确认 Niagara 类型 → 确认比例是否适合 Phase 角色

### 6.2 阶段 C：创建最小试验资产（通过后）

1. 如果 `Cast_MSA` 不是 Montage，在 Aura 自己目录创建：
   - `Content/Aura/SkillPrototype/Phase/AM_Phase_Cast_Fireball_Test`
   - 基于 `Cast` 的 AnimSequence，在释放帧添加 GameplayEvent Notify
2. 如果手部无 socket，在 `phase_Skeleton` 上添加 `FX_R_Hand_Cast`
3. 不直接修改第三方原始动作

### 6.3 阶段 D：火球复验（再之后）

用 Phase + 新 Montage + Free_Magic VFX + 当前技能系统代码做一条最小火球链路验证。

### 6.4 关于 Retarget

**当前不建议 retarget 到 Aura 主角。** 先直接用 Phase 跑通法术链路。如果 Phase 验证通过，后续可以把相同的 Montage 和 SkillDefinition 通过 retarget 迁移到其他角色。这样调试时问题不会混在一起。

## 7. 方法说明

- UE 编辑器未运行，MCP 不可用。
- 资产类型判断基于文件大小、目录结构和命名约定。
- 标记了"已由文件/命名推断"的项目需在 UE 编辑器中最终确认。
- `Cast_MSA` 是否为 AnimMontage 需 UE 编辑器确认——如果确认则是可直接复用的最大利好。
