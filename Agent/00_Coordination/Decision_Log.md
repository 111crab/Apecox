# Apecox 决策记录

## 2026-08-05 - 新项目方向与上下文迁移

- 放弃继续在 Aura/Apex 旧业务代码上演进，Apecox 从干净 UE 5.8 C++ 项目重新建立运行时。
- 新项目面向 UE 游戏客户端求职作品，方向为小规模多人英雄射击垂直切片；不以完整大逃杀内容量为目标。
- Lyra 用于学习职责边界、GAS、武器、输入、预测与复制，不整体照搬 Experience、GameFeature、ModularGameplay。
- 旧项目代码不自动迁移；旧技术思想通过“综合基线 + 精选参考 + 去重原文归档”保存。
- Apecox 继续使用 Codex 规划/审查、ClaudeCode 实施、用户审核和 UE 手工验证的协作方式，但减少重复文档。
- `Current_Phase.md` 是当前状态、下一步和待确认事项的唯一入口。

## 继承但仍需 Apecox RFC 落地的原则

- 不追求万能 GA 或万能配置；追求可复用流程、受控扩展和快速开发。
- GA 扩展采用：公共基类、稳定流程模板、AbilityTask、虚函数钩子、必要的专用 GA。
- SkillDefinition 是组装入口；AbilitySet 是授予层；InputConfig 是 IA 到 InputTag 的映射层。
- 目标选择拆分为施法目标、衍生物生成规则和衍生物检测规则。
- GameplayTag 只表达稳定语义；封闭选项优先枚举或类型化配置，避免特殊情况驱动 Tag 膨胀。
- Effect/State/Buff、CombatEntity、Presentation、Timeline 都按真实需求分阶段引入。
- 单人完成不等于技能完成；涉及玩法的阶段必须给出 Listen Server + Client 验证口径。

## 2026-08-05 - 战斗框架首轮审阅与实施启动

- 架构 RFC 作为后续专题设计的框架基线，但不代表其中所有玩法分叉已经一次性冻结。
- 用户列举的武器、护盾、配件、特殊技能、双视角和滑铲行为用于压力测试架构；具体实现仍按局部阶段逐项讨论。
- 当前先做可玩的战斗原型，不实现胜利条件、积分结算或完整比赛模式；最小 GameMode/GameState 仍承担多人出生、死亡与复活职责。
- V1 输入范围只覆盖键盘和鼠标，不规划手柄映射、辅助瞄准和手柄 UI。
- 普通武器槽固定为两个；技能临时武器或特殊装备不默认占用普通槽，具体规则随技能确认。
- 第一把步枪首版只实现腰射；Hitscan/Projectile、双视角具体资产方案和相机/枪口弹道口径在武器垂直切片前讨论。
- “FP Arms/Weapon + TP World Body/Weapon”同时描述人物姿态和武器表示。运行时有两个表现通道，但不强制制作两份独立源 Mesh；FP/TP 动画可以不同，玩法时序和结算语义必须只有一个来源。
- 死亡后武器、配件、弹药和护盾电池形成世界掉落并在死亡位置附近散落；EvolutionProgress 保留。安装配件是否拆分、掉落散布和并发拾取稍后确定。
- 护盾电池允许移动使用，受伤不取消，切枪或技能会取消；持续时间、消耗时点和取消结果稍后确定。
- 对敌人造成的有效伤害转化为 EvolutionProgress，且进化进度在死亡/复活后保留。
- V1 配件规则由武器及武器类型决定，不允许英雄被动直接改变单把武器的配件兼容和聚合规则。
- 实施按 `Project_Roadmap.md` 的纵向阶段推进；日常只维护 `Current_Phase.md`，不新增重复的实施计划文件。
- Phase 0 关闭 Hardware Ray Tracing 与 Substrate，保留 DX12、SM6、Lumen 和 VSM；禁用实验性 `GASToolsets`，显式启用官方 Gameplay Ability System。
- 项目自有 Gameplay 资产统一进入 `/Game/Blueprints/...`。这里把 `Blueprints` 作为项目内容命名空间，不限制只能存放 Blueprint 类资产；第三方商业资产仍放在其他独立顶层目录且不进入公开仓库。
- GitHub 远端使用 `https://github.com/111crab/Apecox`；项目自建 `.uasset/.umap/.ubulk/.uexp` 使用 Git LFS。
- 初期验证地图新建为非 World Partition 的轻量 `L_Apecox_DevGym`。Lyra `L_ShooterGym` 仅作为布局参考；不迁移其 GameFeature/Experience 依赖，也不迁移收益有限的旧 Apex 模板地图。
- Phase 0 已完成 `GameplayAbilities` 显式启用、Hardware Ray Tracing/Substrate 关闭、轻量 DevGym、单人/2 人 Listen Server 和 `ApecoxEditor Win64 Development` 构建验证；旧地图 Redirector 已清理。

## 2026-08-06 - Phase 1A 玩家与 ASC 生命周期设计

- Phase 0 已以提交 `e115f8f chore: establish Apecox project baseline` 推送到 `origin/main`，开始 Phase 1。
- Phase 1 拆分为三个可独立编译验证的小闭环：1A Gameplay Framework/ASC 所有权，1B AbilitySet/InputTag/最小 Ability，1C Health/Death/Respawn。
- Phase 1A 批准创建 `AApecoxGameMode`、`AApecoxGameState`、`AApecoxPlayerController`、`AApecoxPlayerState`、`AApecoxPlayerCharacter`、`UApecoxAbilitySystemComponent`、`UApecoxVitalAttributeSet`。
- `AApecoxGameMode` 继承 `AGameModeBase`，当前不引入完整比赛状态机。
- 玩家 ASC 由 `AApecoxPlayerState` 真正拥有并使用 Mixed 复制；`AApecoxPlayerCharacter` 只作为当前 Avatar 和 ASC 访问桥梁。
- `UApecoxVitalAttributeSet` 本批只建立 `Health/MaxHealth`，不提前加入 Shield、EvolutionProgress 或死亡行为。
- 新 ClaudeCode 窗口必须先阅读 Apecox 当前协作规范和设计文件；子代理继续只负责按批准 Prompt 实施并提交中文报告，Codex 负责后续审查。

## 2026-08-06 - Git 采用主动暂存、阶段提交、低频推送

- 已验证的小目标完成后，Codex 可以主动暂存该目标的明确改动。
- commit 以可说明的功能闭环或小阶段为单位，不为零散改动频繁提交。
- push 由用户负责最终收口；Codex 仅在形成足够稳定的里程碑时建议，并在获得确认后执行。
- Git 服务于恢复和协作，不应拖慢架构讨论、实现和验证。

## 2026-08-06 - Phase 1B AbilitySet 与输入生命周期

- `UApecoxGameplayAbility` 作为项目抽象 GA 基类；激活策略采用 `OnInputTriggered / WhileInputActive / OnAvatarSet`。
- Ability 并发采用 `Independent / ExclusiveReplaceable / ExclusiveBlocking` 封闭枚举；本阶段不引入 Tag Relationship Mapping。
- `UApecoxAbilitySet` 是 Authority 授予、可由 Handles 撤销的 `UPrimaryDataAsset`，不是技能完整定义。
- 输入链采用 IA -> InputConfig -> InputTag -> AbilitySpec；ASC 在 PlayerController `PostProcessInput` 阶段统一处理 Pressed/Held/Released。
- 首批 Native Tag 仅为 `InputTag.Ability.Tactical` 与 `State.Input.AbilityBlocked`；Tactical 绑定键盘 Q。
- Pawn AbilitySet 由 Character 保存授予 Handles 并随 Avatar 生命周期撤销；装备和英雄玩家级授予以后由各自所有者管理。
