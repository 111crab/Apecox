# 火球术最小配置化运行时 — 实施报告

*创建日期：2026-07-07*
*实施者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-07_Fireball_Minimal_Configured_Runtime.md*

## 简要总结

在 Aura 项目中实现了 V1-A 配置化技能主路径的最小闭环：

```
UAuraSkillDefinition (配置资产)
  -> FGameplayAbilitySpec.SourceObject
  -> UAuraConfiguredActiveAbility (通用 GA)
  -> 鼠标目标 TargetData
  -> 服务器生成配置化投射物
  -> GE_Damage + Damage.Fire SetByCaller
  -> 现有火球表现资产
```

旧教程代码（`GA_FireBolt`、`UAuraProjectileSpell`）完全保留，新路径与旧路径并行运行。

## 修改/新增文件列表

### 新增文件

| 文件 | 模块 | 职责 |
| --- | --- | --- |
| `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h` | Aura (Runtime) | 技能配置数据资产 + Targeting/Delivery 枚举定义 |
| `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp` | Aura (Runtime) | 数据资产类实现（当前为纯数据类） |
| `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h` | Aura (Runtime) | 通用主动技能 GA 类声明 |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp` | Aura (Runtime) | 通用主动技能 GA 完整实现 |

### 修改文件

| 文件 | 变更内容 |
| --- | --- |
| `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h` | 新增 `AddCharacterSkillDefinitions()` 方法声明 + `UAuraSkillDefinition` 前向声明 |
| `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp` | 实现 `AddCharacterSkillDefinitions()`；新增 include |
| `Source/Aura/Public/Character/AuraCharacterBase.h` | 新增 `StartupSkillDefinitions` 数组 + `UAuraSkillDefinition` 前向声明 |
| `Source/Aura/Private/Character/AuraCharacterBase.cpp` | `AddCharacterAbilities()` 中新增调用 `AddCharacterSkillDefinitions` |
| `Config/DefaultGameplayTags.ini` | 新增 `Ability.Fire.Fireball` Tag |

### 未修改的模块依赖

`Aura.Build.cs` 无需修改。新代码使用的所有依赖（`GameplayAbilities`、`GameplayTags`、`Engine`、`CoreUObject`）均已在现有模块依赖列表中。

## 新增类的职责解释

### `UAuraSkillDefinition`（数据资产）

继承 `UPrimaryDataAsset`，是一个纯配置数据类。每个此类资产（如 `DA_Fireball`）代表一个具体技能的完整可编辑配置：

- **Identity 组**：`SkillTag`、`DisplayName`、`Description`、`Icon`
- **Grant/Input 组**：`AbilityClass`、`InputTag`、`DefaultLevel`
- **Activation/Timing 组**：`CastMontage`（软引用）、`MontageSpawnEventTag`
- **Targeting 组**：`TargetingMode`（V1-A 仅 `CursorHit`）
- **Delivery 组**：`DeliveryMode`（V1-A 仅 `Projectile`）、`ProjectileClass`
- **Effect 组**：`DamageEffectClass`、`DamageTypes`（`TMap<FGameplayTag, FScalableFloat>`）

### `UAuraConfiguredActiveAbility`（通用主动 GA）

所有主动配置技能共享的技能激活类。它不包含任何具体技能逻辑——所有行为由 `UAuraSkillDefinition` 配置驱动。

执行流程：
1. `CanActivateAbility`：校验 `SourceObject` 中是否有有效的 `UAuraSkillDefinition` + 投射物类是否已配置
2. `ActivateAbility`：回读配置 -> `CommitAbility`（成本/冷却）-> 创建 `UTargetDataUnderMouse` 任务
3. `OnTargetDataReceived`：提取目标位置 -> 播放 Montage（双方）-> 服务端等待 AnimNotify 事件或 fallback 直接生成
4. `OnMontageSpawnEvent`（服务端）：在精确动画时机生成投射物
5. `ExecuteProjectileSpawn`（服务端）：复用现有 `AAuraProjectile` 生成逻辑，从配置资产读取 `DamageEffectClass` 和 `DamageTypes`，通过 `AssignTagSetByCallerMagnitude` 注入伤害数值
6. `EndAbility`：清理缓存状态

关键设计决策：
- **Listen Server 防护**：使用 `bHasProcessedTargetData` 标志防止 `ValidData` 双重广播导致的重复生成
- **Fallback 路径**：如果未配置 `CastMontage` 或 `MontageSpawnEventTag`，收到 TargetData 后直接在服务端生成投射物（适配专用服务器或无动画技能）
- **Montage 中断处理**：`OnMontageInterrupted` 记录日志，不补救生成（被打断的技能不应再发射）

### `EAuraSkillTargetingMode` / `EAuraSkillDeliveryMode`（枚举）

V1-A 最小枚举，各含一个值（`CursorHit` / `Projectile`）。后续技能编辑器扩展时增加新枚举值。

## 为什么这样设计，而不是每个技能一个 GA

如果每个技能写一个独立的 GameplayAbility C++ 类：

1. **代码重复**：火球术、复苏脉冲、烈焰领域等都需要类似的"输入 -> 目标采集 -> 生成/应用效果 -> 表现"流程，每个类都要重复实现。
2. **维护灾难**：修复一个通用 bug（如 Listen Server 重复生成）需要修改 N 个类。
3. **编辑器不友好**：未来技能编辑器的目标是让设计师创建和修改技能配置资产，而不是生成 C++ 代码后重新编译。

当前设计：

- **1 个通用 GA**（`UAuraConfiguredActiveAbility`）= 所有技能的运行时执行器
- **N 个配置资产**（`UAuraSkillDefinition`）= 每个技能的差异化数据
- `AbilitySpec.SourceObject` = 连接配置资产和通用 GA 的桥梁

这直接服务未来技能编辑器：编辑器修改 `UAuraSkillDefinition` 资产 -> Play 时可被角色使用 -> 不需要生成 C++ 代码。

## GameplayTag 选择说明

**`Ability.Fire.Fireball`**：作为火球术的技能身份 Tag，仅写入 `DefaultGameplayTags.ini`，不创建 native C++ 变量。

**选择原因**：这个 Tag 作为配置资产的 `SkillTag` 字段使用，用于身份标识和可能的 Tag 查询。它不需要在 C++ 代码中直接引用（不像 `Damage.Fire` 需要在 `DamageTypesToResistances` 映射中被访问），因此使用 ini 配置而非 native tag 更轻量，也更容易在编辑器中管理。

**已有可复用 Tag**（均已在 `AuraGameplayTags.cpp` 中注册为 native tag）：
- `InputTag.1` — 火球术推荐使用的输入 Tag
- `Damage.Fire` — 火焰伤害 SetByCaller Tag
- `Event.Montage.FireBolt` — Montage AnimNotify 事件 Tag（仅在 ini 中）

**关于输入 Tag 的建议**：
`DA_Fireball` 推荐使用 `InputTag.1`（键盘 1 键），以避免和旧的左键火球（`GA_FireBolt` 可能绑定 `InputTag.LMB`）产生双触发。如果用户确认 `GA_FireBolt` 实际上不占用 `InputTag.LMB`（通过 `StartupInputTag` 查看），也可安全使用左键。

## 是否创建了 DA_Fireball

**未创建**。当前环境无法访问 UE MCP/编辑器，也没有通过脚本硬写 `.uasset` 的必要工具。

以下是完整的手动创建步骤：

### 手动创建 `DA_Fireball` 步骤

1. 打开 UE 编辑器（5.8），加载 Aura 项目。
2. 在 Content Browser 中导航到 `Content/Blueprints/AbilitySystem/`。
3. 右键 -> 新建文件夹，命名为 `SkillDefinitions`。
4. 右键 -> Miscellaneous -> Data Asset，选择 `UAuraSkillDefinition` 作为类型。
5. 命名为 `DA_Fireball`。
6. 打开 `DA_Fireball`，填写以下字段：

| 分组 | 字段 | 值 |
| --- | --- | --- |
| Identity | SkillTag | `Ability.Fire.Fireball` |
| Identity | DisplayName | 火球术 |
| Identity | Description | 向鼠标指向位置发射一枚火球，命中后造成火焰伤害。 |
| Identity | Icon | `/Game/Assets/UI/Spells/FireBolt` |
| Grant\|Input | AbilityClass | `UAuraConfiguredActiveAbility` |
| Grant\|Input | InputTag | `InputTag.1` |
| Grant\|Input | DefaultLevel | 1 |
| Activation\|Timing | CastMontage | `/Game/Assets/Characters/Aura/Animations/Abilities/AM_Cast_FireBolt` |
| Activation\|Timing | MontageSpawnEventTag | `Event.Montage.FireBolt` |
| Targeting | TargetingMode | Cursor Hit |
| Delivery | DeliveryMode | Projectile |
| Delivery | ProjectileClass | `/Game/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/BP_FireBolt` |
| Effect | DamageEffectClass | `/Game/Blueprints/AbilitySystem/GameplayEffects/GE_Damage` |
| Effect | DamageTypes | `Damage.Fire` -> 50（添加一个元素，Tag 选 `Damage.Fire`，Value 设 50） |

7. 保存资产（Ctrl+S）。

### 配置角色使用 DA_Fireball

1. 打开 `Content/Blueprints/Character/BP_AuraCharacter`（或玩家角色的蓝图）。
2. 在 Details 面板找到 `Attributes` 分类下的 `Startup Skill Definitions` 数组。
3. 添加一个元素，选择刚创建的 `DA_Fireball`。
4. 保存蓝图。

### 验证输入不会和旧火球双触发

1. 打开旧 `Content/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/GA_FireBolt`。
2. 查看 `StartupInputTag` 字段的值。
3. 如果旧 `GA_FireBolt` 的 `StartupInputTag` 是 `InputTag.LMB` 或其他非 `InputTag.1` 的 Tag，则不会冲突。
4. 如果旧 `GA_FireBolt` 也使用 `InputTag.1`，请将新 `DA_Fireball` 的 `InputTag` 改为未占用的按键（如 `InputTag.2`），或在用户确认后从 `BP_AuraCharacter` 的 `StartupAbilities` 中移除旧 `GA_FireBolt`。

## 运行了哪些操作

### 编译/构建

**未执行编译**。当前环境无法定位 UE 5.8 安装路径，因此无法运行 `UnrealBuildTool`。

**建议**：
- 在 UE 编辑器中打开项目时会自动编译（Hot Reload）。
- 或在 Visual Studio 中打开 `Aura.sln`，Build Solution（`Ctrl+Shift+B`）。
- 或在命令行中运行（需要先找到 UE 安装路径）：
  ```powershell
  & "<UE5.8_Path>\Engine\Build\BatchFiles\Build.bat" AuraEditor Win64 Development -Project="D:\UnrealProject\Aura\Aura.uproject"
  ```

### 文件验证

已逐一检查：
- 所有新增 `.h` 文件正确使用 `#pragma once`、`.generated.h` 最后包含、`UCLASS`/`UENUM` 宏
- 所有新增 `.cpp` 文件包含对应的 `.h` 作为第一个 include
- `Aura.Build.cs` 依赖无需修改（所有新类型依赖已在模块依赖列表中）
- 前向声明和 include 无循环依赖

## 单人验证结果

**未执行运行时验证**（无法打开 UE 编辑器）。

请在 UE 编辑器中打开项目后，按以下步骤验证：

1. 确认 C++ 编译通过（编辑器启动时自动编译；如有编译错误，修复后再继续）。
2. 在 Content Browser 中确认 `UAuraSkillDefinition` 出现在 Data Asset 类型列表中。
3. 按上述步骤手动创建 `DA_Fireball`。
4. 在 `BP_AuraCharacter` 的 `StartupSkillDefinitions` 中添加 `DA_Fireball`。
5. 单人 PIE（Play In Editor）：
   - 按 `1` 键（或配置的输入键）
   - 验证：播放 `AM_Cast_FireBolt` 施法动画
   - 验证：动画中在合适的时机生成火球投射物（从武器/战斗 Socket 位置）
   - 验证：火球朝鼠标命中点飞行
   - 验证：火球命中敌人后敌人生命值下降（查看 `AAuraCharacterBase` 的 AttributeSet）
   - 验证：命中时触发 Niagara 爆炸特效和音效
   - 验证：火球未命中任何目标时在超时后自动销毁
6. 如果未配置 Montage，验证 fallback 路径：直接生成投射物，并在 Output Log 中看到对应的 Warning 日志。
7. 确认旧左键火球（`GA_FireBolt`，如果有）仍然可以正常使用。

## 多人验证结果

**未执行**（无法打开 UE 编辑器）。

如果环境能运行 Listen Server / 两客户端，请按以下步骤验证：

1. 主窗口选择 "Play as Listen Server"，额外打开一个客户端窗口。
2. 主机按 `1` 键释放火球：
   - 验证：主机看到 Montage 和投射物
   - 验证：客户端也能看到投射物的飞行和命中表现
   - 验证：敌人受到的伤害正确（只有服务端权威应用）
3. 客户端按 `1` 键释放火球：
   - 验证：客户端看到投射物（服务端生成后复制）
   - 验证：服务端确认伤害应用
4. 检查 Listen Server Host 是否出现重复命中特效/音效（`AAuraProjectile::OnSphereOverlap` 中已有 `bHit` 标志防护，应无重复）。
5. 确认投射物命中后正常销毁，超时（LifeSpan = 15s）后也正常清理。

如果环境无法验证多人：
- **不要假装通过**
- 在后续 Codex 审查和人工测试中补充多人验证
- 关键风险点：`bHasProcessedTargetData` 防护标志是否在所有网络条件下正确工作；`AAuraProjectile` 的复制是否正常

## 已知问题

1. **UE 5.8 定位失败**：当前环境无法找到 UE 5.8 安装路径，因此无法运行编译和 PIE 验证。需要用户在本地编译和测试。

2. **Montage 同步加载**：`UAuraConfiguredActiveAbility::PlayCastMontageIfConfigured()` 使用了 `TSoftObjectPtr::LoadSynchronous()` 同步加载 Montage。这对 V1-A 小资产是可接受的折中，但已在注释中标记为原型期妥协。未来技能数量增多后应改为 `UAssetManager::GetStreamableManager().RequestAsyncLoad()`。

3. **Montage 加载失败处理**：如果 `CastMontage.LoadSynchronous()` 返回 nullptr（资产路径无效或已被移动），当前仅记录日志并跳过 Montage，然后 fallback 到直接生成投射物。这是预期行为。

4. **专用服务器适配**：`WaitGameplayEvent` 依赖 Montage 的 AnimNotify 发送 GameplayEvent。在专用服务器上，虽然 Montage 会在不可见的 Mesh 上播放且 AnimNotify 仍会触发，但此路径尚未在专用服务器环境下测试。如果 AnimNotify 未触发，系统会 fallback 到直接生成。

5. **`UAuraSkillDefinition` 使用 `UPrimaryDataAsset` 而非项目默认的 `UDataAsset`**：这是按照 `Skill_Runtime_Model.md` 建议的有意选择，为未来 Asset Manager Primary Asset 扫描做准备。如果编译时遇到 `UPrimaryDataAsset` 相关错误，可切换为 `UDataAsset`（与项目现有 `UCharacterClassInfo` 一致）。

6. **旧 GA 和新配置技能使用相同输入 Tag 时的双触发**：如果旧 `GA_FireBolt` 的 `StartupInputTag` 和新 `DA_Fireball` 的 `InputTag` 相同（如都使用 `InputTag.LMB`），一次按键会触发两个技能。防护方案见上文"手动创建步骤"中的输入冲突检查。

## 下一步建议

1. **立即执行**：用户在 UE 编辑器中编译、创建 `DA_Fireball` 并完成独立 PIE 验证。
2. **Codex 审查**：Codex 对本次实施进行代码审查，关注 GAS 生命周期正确性、多人边界和注释完整性。
3. **输入系统清理**：在验证配置路径可行后，讨论是否需要统一输入分配策略（哪些按键给旧教程技能，哪些给新配置技能）。
4. **后续 V1-A 技能**：火球术闭环验证通过后，按顺序实施复苏脉冲、烈焰领域、蓄力贯穿箭、三段裂刃、荆棘护盾。每个技能应复用 `UAuraConfiguredActiveAbility` 作为基础，并增量扩展 `UAuraSkillDefinition` 的字段和枚举值。
5. **Asset Manager 集成**：为 `UAuraSkillDefinition` 覆盖 `GetPrimaryAssetId()`，使其被 `UAuraAssetManager` 的 Primary Asset 扫描识别，方便后续编辑器列举和异步加载。
