# Apecox 正式收口清理报告

日期：2026-09-18

## 目标与方法

本轮依据用户此前约定，在收到“项目正式收口”后才执行全项目清查。清查覆盖 C++、模块依赖、Config、项目自有二进制资产、Asset Registry、Blueprint 编译、自动化测试和协作文档。第三方 RAR、InfimaGames 与 Epic 资产只读检查，没有为整理名称或消除供应商警告而改写。

删除标准是：已经被当前实现取代、没有运行时或资产引用、删除后能够通过重新构建与资产加载验证。仍被 Blueprint 序列化路径引用的类型和字段，即使名称带有历史痕迹，也按兼容入口保留。

## 删除的 C++ 分支

实体 Projectile 已是步枪唯一运行时模型，因此删除旧 Hitscan Ability 与专用 TargetData：

- `Source/Apecox/Public/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.h`
- `Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp`
- `Source/Apecox/Public/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.h`
- `Source/Apecox/Private/AbilitySystem/TargetData/ApecoxHitscanShotTargetData.cpp`

同时删除 `FApecoxHitscanFireConfig`、`GetHitscanFireConfig` 和对应旧注释；Projectile、移动战斗与换弹测试夹具统一使用 `UApecoxProjectileFireAbility`、`FApecoxProjectileFireConfig` 与 `FApecoxRangedShotTargetData`。

## 移除的调试与废弃资产

下列资产在删除前均确认无剩余引用，删除后又通过 Asset Registry 重新扫描确认不存在：

- `/Game/Blueprints/AbilitySystem/Abilities/GA_Debug_SelfDamage`
- `/Game/Blueprints/AbilitySystem/Effects/GE_Debug_SelfDamage`
- `/Game/Blueprints/Input/Actions/IA_Ability_Tactical`
- `/Game/Blueprints/AbilitySystem/Abilities/GA_Phase1B_InputLifecycle`
- `/Game/Blueprints/Characters/Animations/FirstPerson/Unarmed/RTG_Apecox_LPSP_To_IGMannequin`
- `/Game/Blueprints/Weapons/Rifle/Animations/FirstPerson/ABP_Apecox_Rifle_FP_Arms`

同步修改：

- `IMC_Apecox_Gameplay` 删除 `X → IA_Ability_Tactical`。
- `DA_Apecox_InputConfig` 的 Ability 输入只保留 `IA_WeaponFire`。
- `DA_Phase1B_AbilitySet` 只保留 `GA_Apecox_Death`。
- 删除 `InputTag.Ability.Tactical` 原生 Tag。

## 机器人与模块清理

- 机器人最终范围是静止靶标。`AApecoxWanderAIController` 删除随机点查询、MoveTo、等待计时器、半径/速度开关和错误重试逻辑，只保留 `bWantsPlayerState=true`，以继续复用 PlayerState ASC 生命周期。
- `AApecoxBotCharacter` 删除为失败的移动 AI 尝试加入的 CMC 加速度/朝向补丁和运行时诊断日志。
- `Apecox.Build.cs` 删除不再使用的 `NavigationSystem`。`AIModule` 因 `AAIController` 基类保留；`ModularGameplay` 因 `UApecoxWeaponStateComponent : UControllerComponent` 保留。

## 其他冗余清理

- 删除 `UApecoxInventoryItemDefinition.MaxStackSize`：当前库存只有独立武器实例，没有堆叠事务。
- 删除 `bHasLeftHandIKTarget`：没有 C++、Config 或资产引用。
- 保留 `LeftHandIKEffectorTransform`：活动 AnimBP 仍保存一个未接线变量节点，删除反射字段会制造无效 Blueprint 节点。
- 删除 GameplayAbility `NonInstanced` 兼容检查：Apecox 基类固定为 `InstancedPerActor`，该 UE 5.8 枚举值也已经弃用。
- 删除 `DefaultEngine.ini` 中重复的 `DefaultGraphicsRHI=DefaultGraphicsRHI_DX12`。
- 更新注释和测试名称，清除把当前 Projectile 流程描述为 Hitscan 的旧术语。

## 有意保留的历史名称

- `AApecoxWanderAIController`：`BP_ApecoxBotCharacter` 已序列化该类路径。
- `DA_Phase1B_AbilitySet`：玩家和人机蓝图都引用该资产路径。
- `GE_Weapon_Rifle_Damage_Debug`：当前步枪正式引用的权威伤害 GE；重命名会给已稳定的二进制引用链增加 Redirector 风险。
- 供应商资源及历史 Agent 报告：前者是当前动画/VFX/SFX 依赖，后者用于设计追溯，不参与运行时。

## 验证证据

### C++ 构建

`ApecoxEditor Win64 Development` 最终完整构建成功，0 编译警告：

- `Saved/Diagnostics/ProjectClosure/build_final.log`

### Blueprint 编译

`CompileAllBlueprints` 完成，Apecox Blueprint 为 0 错误、0 警告、0 加载失败：

- `Saved/Diagnostics/ProjectClosure/compile_blueprints.log`

命令加载第三方资源时另报告 14 条 InfimaGames Manny PoseAsset 与源动画版本不一致警告。它们属于供应商资产状态，不是 Apecox Blueprint 编译失败，因此未在收口中改写商业资产。

### 资产验证

- 清理后 `/Game/Blueprints` Asset Registry 记录数：49。
- 11 个关键运行资产全部存在并可加载。
- 6 个删除目标全部不存在。
- 已删除调试、旧 AnimBP、Retargeter 与 Hitscan 分支的 Asset Registry 残留引用：0。
- `DA_Apecox_InputConfig` 的 Ability 输入只剩 `IA_WeaponFire`。
- `DA_Phase1B_AbilitySet` 的 Ability 只剩 `GA_Apecox_Death`。

证据：

- `Saved/Diagnostics/ProjectClosure/asset_references_before_cleanup.json`
- `Saved/Diagnostics/ProjectClosure/asset_references.json`
- `Saved/Diagnostics/ProjectClosure/closure_asset_verification.json`

### 自动化

全量 `Apecox.*`：64 项成功，0 失败，0 未运行。其中 63 项 Success、1 项 Success With Warnings；唯一警告是引擎后台访问 Google `generate_204` 地址超时，测试断言仍为成功。

- `Saved/Diagnostics/ProjectClosure/Tests/index.json`
- `Saved/Diagnostics/ProjectClosure/tests.log`

## 收口结论

当前代码、配置与项目自有资产已经与“第一人称单步枪 + 静止可重生靶标”的交付范围一致。已放弃的 Hitscan、自由漫游和战术自伤分支不再参与编译或资产加载。路线图中的 AI、比赛规则、多人加固、武器生态和英雄系统已经明确转为未来重新立项内容。
