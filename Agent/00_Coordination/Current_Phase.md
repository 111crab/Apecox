# 当前局部阶段

更新日期：2026-08-05

## 顶层阶段

Phase 0 - 工程与协作基线收口；下一步进入 Phase 1。

## 上一阶段完成

- Aura/Apex 技术思想迁移、精选参考和只读历史归档已经完成。
- Lyra Framework、GAS、Inventory、Equipment、Weapon、Health、动画和网络边界已经定向解构。
- `Apecox_Combat_Framework_Architecture_RFC.md` 已完成，并通过十个场景压力测试。
- 用户已完成 RFC 首轮审阅；已确认内容和延后问题已写回 RFC 与决策记录。

## 当前目标

在写 Gameplay C++ 之前，完成一个可编译、可 PIE、可进行多人验证、可提交的干净工程基线。

本阶段不创建 Gameplay Framework 类，不接入武器资产，也不提前设计类名和 GameplayTag。

## 当前项目事实

- UE 5.8 C++ 空项目，源码只有自动生成的主模块。
- Git 已初始化为 `main` 分支，`origin` 指向 `https://github.com/111crab/Apecox.git`，LFS 与忽略规则已经建立；尚未形成首个提交。
- 用户已创建并完成 DevGym 的单人/Listen Server 验证。
- `Substrate=False`、DX12/SM6/Lumen/VSM 正确；`r.RayTracing=True` 仍需关闭。
- `GASToolsets` 已移除，`GameplayAbilities` 已显式写入 `.uproject`。
- 当前默认地图位于已接受的 `/Game/Blueprints/Maps`；旧同名 ObjectRedirector 已清理。

## Phase 0 最终复核状态

- `GameplayAbilities` 已在 `.uproject` 中显式启用，`GASToolsets` 已关闭。
- `r.RayTracing=False`、Ray Tracing Proxies 关闭、`Substrate=False`；DX12、SM6、Lumen 和 VSM 保留。
- 默认地图正确指向 `/Game/Blueprints/Maps/L_Apecox_DevGym`，正式地图受 Git LFS 管理。
- 用户已验证单人和 2 人 Listen Server 正常。
- Codex 已完成 `ApecoxEditor Win64 Development` 构建，结果为 `Succeeded`。
- `/Game/L_Apecox_DevGym` 的 ObjectRedirector 已修复，Phase 0 技术验证全部通过。

## 已确认的原型范围

- 当前只做战斗原型，不实现胜利条件或完整比赛模式；最小规则层仍需负责出生、死亡和复活。
- V1 输入只覆盖键盘和鼠标，不实现手柄输入。
- 普通武器槽为两个；技能临时武器不默认占用普通槽。
- 第一把步枪首版只实现腰射；Hitscan/Projectile 尚未决定。
- 死亡后武器、配件、弹药和电池成为世界掉落；EvolutionProgress 保留。
- 护盾电池允许移动使用，受伤不取消，切枪或技能会取消。
- 对敌人造成的有效伤害转化为 EvolutionProgress。
- V1 配件规则由武器/武器类型决定，不受英雄被动影响。

## 已确认的工程边界

- 关闭 Hardware Ray Tracing 与 Substrate，保留 DX12、SM6、Lumen 和 VSM。
- 禁用 `GASToolsets`，同时显式启用官方 Gameplay Ability System。
- 采用 `/Game/Blueprints/...` 作为 Apecox 项目自有 Gameplay 内容根目录；其下按 `Maps`、`Characters`、`Weapons`、`Abilities`、`Input`、`UI`、`Animation`、`VFX` 等领域分类，不限制只能存放 Blueprint 类资产。
- GitHub 远端为 `https://github.com/111crab/Apecox`；第三方 Marketplace/Fab 商业资产默认不提交。
- 项目自建 `.uasset/.umap` 及其 UE 二进制伴随文件使用 Git LFS。
- Phase 0 新建非 World Partition 的轻量 `L_Apecox_DevGym`，不直接迁移 Lyra GameFeature 地图或旧 Apex 模板地图。

## 当前实施顺序

1. Codex 初始化 Git、远端、忽略规则和 LFS，但暂不提交。已完成。
2. 用户已完成首轮 UE 设置、DevGym、单人和 Listen Server 验证。
3. 用户按 `Current_UE_Manual_Steps.md` 修正 Ray Tracing、GameplayAbilities 与 Redirector。已完成。
4. Codex 复核配置和 Git 工作区，形成 Apecox 初始项目提交与 push。等待 Android File Server 配置确认后收口。
5. Phase 0 验证通过后，覆盖 `Current_Code_Design.md`，开始审阅 Phase 1 的 Gameplay Framework/GAS 公开设计。
6. 用户批准公开设计后，才生成第一份 ClaudeCode 实施 Prompt。

## Phase 0 成功标准

- 项目能在 UE 5.8 DX12 下稳定打开、编译和 PIE。
- 官方 GAS 与 Enhanced Input 依赖明确，没有实验性工具的隐式运行时依赖。
- 项目自有开发地图可启动，Listen Server + Client 能进入同一世界。
- Git 基线只包含项目自有工程与允许提交的二进制资产，不包含第三方商业资产和生成目录。

## 延后到对应阶段

- 第一把枪的 Hitscan/Projectile、双视角具体 Mesh/动画方案和弹道口径：Phase 2 前讨论。
- 电池消耗时点、掉落散布细节、动作关系矩阵：对应功能实施前讨论。
- 首批英雄技能与临时武器规则：Phase 6 前讨论。
- 技能编辑器：运行时稳定后再预研。

## 当前下一步

- 处理公开仓库中的 Android File Server 自动生成令牌后，完成首次 commit 与 push。
- 随后进入 Phase 1：玩家生命周期与 GAS 基线。
- Phase 1 先覆盖 `Current_Code_Design.md`，审阅 Gameplay Framework 候选类、职责、公开命名和对称 `Public/Private` 路径；批准前不写业务代码。
