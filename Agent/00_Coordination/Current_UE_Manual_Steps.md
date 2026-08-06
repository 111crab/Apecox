# 当前 UE 手工操作清单

更新日期：2026-08-06

适用任务：Phase 1A 玩家、ASC 与 Pawn Avatar 生命周期验证。

验证状态：**已完成并全部通过。** 单人 PIE 与两人 Listen Server 均确认 Owner/Avatar 关系正确，未发现本清单所列异常。

## 开始前状态

- Codex 已完成 `ApecoxEditor Win64 Development` 完整构建和 DLL 链接，结果为 `Succeeded`。
- 本阶段不需要创建蓝图、输入资产、GameplayEffect、GameplayTag、相机或人物 Mesh。
- `AApecoxPlayerCharacter` 当前没有相机、输入和可见角色资产，因此进入 PIE 后画面静止或没有人物表现是正常现象，不属于本阶段失败。
- `Health/MaxHealth` 当前默认是 `0/0`。出生值以后由初始化 GE 或英雄配置负责；不要在编辑器里临时改成 100。

## 一、刷新 Rider

1. 按你当前使用的 Rider 流程刷新 Unreal 项目/C++ 文件。
2. 确认可以看到：
   - `Source/Apecox/Public/...`
   - `Source/Apecox/Private/...`
3. 不需要重新生成由 Codex 管理的 `.sln`，也不要移动 C++ 文件。

## 二、让开发地图使用 Apecox GameMode

1. 启动 `D:/UnrealProject/Apecox/Apecox.uproject`。
2. 打开 `/Game/Blueprints/Maps/L_Apecox_DevGym`。
3. 在编辑器顶部菜单打开 `Window -> World Settings`。
4. 在 World Settings 的 Details 搜索：`GameMode Override`。
5. 把它设置为 C++ 类 `ApecoxGameMode`。
   - 不需要创建 `BP_ApecoxGameMode`。
   - 该 C++ GameMode 已把 GameState、PlayerController、PlayerState 和 Default Pawn 指向 Apecox 自有类。
6. 保存地图。

如果下拉列表看不到 `ApecoxGameMode`：关闭并重新打开编辑器。完整 DLL 已经编译，不要新建同名蓝图或重复 C++ 类。

## 三、单人 PIE 验证

1. PIE 玩家数设为 `1`。
2. 启动 PIE。
3. 在 PIE 窗口按控制台键，执行：

```text
ShowDebug AbilitySystem
```

4. 查看屏幕上的 GAS 调试标题。它应同时包含类似内容：

```text
for avatar ApecoxPlayerCharacter_...
for owner ApecoxPlayerState_...
```

这证明：

- ASC 的逻辑 Owner 是 `AApecoxPlayerState`。
- 当前物理 Avatar 是 `AApecoxPlayerCharacter`。
- Character 的 `PossessedBy` 路径完成了 ActorInfo 初始化。

5. 在 PIE 运行期间查看 World Outliner：
   - 应存在 `ApecoxPlayerController_...`。
   - 应存在 `ApecoxPlayerState_...`。
   - 应存在 `ApecoxPlayerCharacter_...`。
6. 选择运行时 `ApecoxPlayerState`，在 Details 中确认有：
   - `AbilitySystemComponent`，类型为 `ApecoxAbilitySystemComponent`。
   - `VitalAttributeSet`，类型为 `ApecoxVitalAttributeSet`。
7. 选择运行时 `ApecoxPlayerCharacter`：
   - Character 自身不能出现另一个作为默认子对象创建的 ASC。
   - Capsule、Mesh、CharacterMovement 等 ACharacter 原生组件存在是正常的。
8. `ShowDebug AbilitySystem` 的 Attribute 页面可以显示 `Health/MaxHealth=0`；这是预期结果。
9. 打开 Output Log，确认没有：
   - C++ `ensure` 或崩溃。
   - `[Apecox] ASC ... has unexpected AvatarActor ...`。
   - 重复 ASC、无效 OwnerActor 或无效 AvatarActor 相关警告。
10. 停止 PIE。

## 四、两人 Listen Server 验证

1. Play 设置：
   - `Number of Players = 2`
   - `Net Mode = Play As Listen Server`
2. 建议使用两个独立 PIE 窗口，方便分别打开控制台。
3. 启动 PIE，等待主机和客户端都进入 `L_Apecox_DevGym`。
4. 在主机窗口执行：

```text
ShowDebug AbilitySystem
```

5. 在客户端窗口执行同一命令。
6. 两个窗口的 GAS 调试标题都必须满足：
   - Avatar 名称是各自的 `ApecoxPlayerCharacter_...`。
   - Owner 名称是对应的 `ApecoxPlayerState_...`。
   - Owner 与 Avatar 不是同一个 Actor。
7. 服务器 PIE World Outliner 中应有两组 PlayerState/Character：
   - 每个 PlayerState 各自拥有一个 ASC。
   - 每个 Character 都不创建第二个 ASC。
   - 不同玩家的 Avatar/Owner 不能串到另一位玩家。
8. 查看 Output Log，确认没有：
   - `unexpected AvatarActor` ensure。
   - ActorInfo 无效、重复初始化或组件复制警告。
   - 客户端断开或加入失败。
9. 停止 PIE。

## 五、本阶段暂不人工验证的内容

- 当前尚未实现 Death/Respawn，不要求为了测试旧 Avatar 清退而临时创建关卡蓝图或 Cheat 流程。
- “新 Pawn 先到、旧 Pawn 后清理”的乱序防护已经完成源码审查；在 Phase 1C 有真实 Respawn 后进行运行时验证。
- 当前没有初始化 GE，因此不验证 Health 数值变化，只验证 AttributeSet 存在和复制声明没有错误。
- 当前没有 Ability、Cue 或输入缓存，因此“不残留 Ability/Cue/Input”的真实行为在 Phase 1B/1C 闭环后验证。

## 六、通过标准

只有以下全部满足，Phase 1A 的 UE 验证才通过：

- 地图实际使用 `ApecoxGameMode`。
- 单人 `ShowDebug AbilitySystem` 同时显示正确 PlayerState Owner 和 Character Avatar。
- 两人 Listen Server 的主机与客户端分别显示自己的正确 Owner/Avatar。
- ASC 只由 PlayerState 创建，Character 没有第二个 ASC。
- Output Log 没有 Apecox ActorInfo ensure、复制错误或崩溃。
- `Health/MaxHealth=0/0` 被视为当前正确结果，没有通过编辑器临时写入出生值。

验证完成后，把单人和多人结果以及任何日志异常告诉 Codex。
