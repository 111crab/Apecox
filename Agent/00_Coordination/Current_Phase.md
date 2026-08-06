# 当前局部阶段

更新日期：2026-08-06

## 顶层阶段

Phase 1 - 玩家生命周期与 GAS 基线。

Phase 0 已完成并形成远端基线提交：`e115f8f chore: establish Apecox project baseline`。

## 当前局部任务

Phase 1A：最小 Gameplay Framework、PlayerState ASC 所有权和 Pawn Avatar 对称 Init/Uninit。**已完成。**

本批目标是先证明下列关系在单人和多人环境中稳定：

```text
OwnerActor  = AApecoxPlayerState
AvatarActor = 当前 AApecoxPlayerCharacter
Replication = Mixed
```

## 已批准设计

- 创建 `AApecoxGameMode`、`AApecoxGameState`、`AApecoxPlayerController`、`AApecoxPlayerState`、`AApecoxPlayerCharacter`。
- 创建 `UApecoxAbilitySystemComponent` 和只包含 `Health/MaxHealth` 的 `UApecoxVitalAttributeSet`。
- `AApecoxGameMode` 继承 `AGameModeBase`，当前不实现完整比赛状态机。
- PlayerState 真正拥有 ASC 和 AttributeSet；Character 只作为 Avatar 和访问桥梁，禁止创建第二个 ASC。
- 新 C++ 文件严格使用对称 `Source/Apecox/Public/<领域>` 与 `Private/<领域>`。
- 公开 API、成员变量和路径以 `Current_Code_Design.md` 为准。

## 当前执行顺序

1. Codex 更新当前设计与阶段记录。已完成。
2. Codex 生成适用于全新 ClaudeCode 窗口的冷启动实施 Prompt。已完成。
3. ClaudeCode 完成首轮代码和中文报告。已完成。
4. Codex 完成首轮审查。结论：暂不通过，需要修复两个 ASC 生命周期问题和若干基础问题。
5. ClaudeCode 完成首轮修复；Codex 复审确认两个 P1 已解决，但发现一个 MaxHealth Base 回写的剩余 P2。
6. ClaudeCode 完成二次精简修复；Codex 最终静态复审和独立 `-NoLink` 构建均已通过。
7. 用户关闭 Unreal Editor 后，Codex 完成一次无文件锁的完整 DLL 链接构建。已完成，`Result: Succeeded`。
8. 用户确认审查结果后自行刷新 Rider 项目文件。
9. Codex 覆盖 `Current_UE_Manual_Steps.md`，用户完成 UE 配置和单人/Listen Server 验证。已完成，全部通过。
10. Phase 1A 已形成代码审查、完整构建、单人 PIE 和两人 Listen Server 的完整闭环。
11. 下一步讨论 Phase 1B 的 AbilitySet、InputTag 与最小 Ability；未确认设计前不实施。

## 本批不做

- AbilitySet、InputTag、输入绑定和 GameplayTag。
- Health 死亡语义、Death Ability、Respawn、Shield、EvolutionProgress。
- 相机、人物 Mesh、动画、武器、UI 或其他 UE 资产。
- Dedicated Server 最终验收将在 Phase 1 完整闭环时进行。

## Phase 1A 成功标准

- `ApecoxEditor Win64 Development` 编译通过。
- 单人 PIE 中 PlayerState 是 ASC Owner，当前 Character 是 Avatar。
- 2 人 Listen Server 中主机与客户端都能建立正确的 Owner/Avatar，不存在 Character 上的第二个 ASC。
- 解除占有、销毁或更换 Pawn 时，只清理旧 Avatar，不破坏已绑定的新 Avatar。
- `Health/MaxHealth` 可以复制，但本批不要求伤害、死亡或 UI 表现。
- 实施不包含未批准 GameplayTag、资产或额外系统。

## 当前待确认

无新的类名或 GameplayTag。修复不改变已批准的七类职责，只纠正 ASC 解绑、旧 Avatar 接管、网络更新频率和 Attribute 不变量。

## 当前下一步

Phase 1A 已通过。进入 Phase 1B 设计讨论：明确 `AbilitySet` 的资产职责、输入 Tag 命名与路由边界，以及最小 Ability 的激活、取消和结束验证口径。
