# Phase 1A 玩家与 ASC 生命周期首轮审查

日期：2026-08-06

## 结论

**暂不通过。**

七个类、Public/Private 对称路径、PlayerState 持有 ASC、Mixed 复制、Character 不创建第二个 ASC 等主体方向正确，源码已通过编译步骤。但存在两个会破坏换 Pawn 生命周期的高优先级问题，必须修复后再进入 UE 配置和 PIE 验证。

其中 `ClearActorInfo()` 问题来自原 Codex Prompt 的约束过强，不应归咎于子代理自行偏离；审查后已修正设计口径。

## Findings

### P1 - 新 Pawn 绑定前没有清退 ASC 上已有的旧 Avatar

文件：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp:42`

当前初始化只处理“当前 Character 缓存了另一个 ASC”，没有处理“即将取得的 ASC 仍绑定另一个 Character”。客户端可能先收到并绑定新 Pawn，旧 Pawn 稍后才解除；当前代码会直接用新 Pawn 覆盖 ActorInfo，旧 Pawn 上的活动 Ability 没有先取消，可能把旧执行状态带到新 Avatar。

修复：读取 `ASC->GetAvatarActor()`；若为另一个 `AApecoxPlayerCharacter`，先调用该旧 Character 的 `UninitializeAbilitySystem()`，确认旧 Avatar 解绑后再绑定新 Pawn。

### P1 - 正常换 Pawn 使用 `ClearActorInfo()`，连持久 PlayerState Owner 也被清空

文件：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp:73`

架构要求 ASC 的 OwnerActor 是跨 Pawn 存活的 PlayerState。正常解绑只应移除 Avatar；`ClearActorInfo()` 同时清空 Owner 与 Avatar，使 ASC 在重生间隙失去逻辑 Owner，与 PlayerState 持有 ASC 的目的冲突。

修复：当前 Character 仍是 Avatar 时，先取消活动 Ability；若 OwnerActor 有效，调用 `SetAvatarActor(nullptr)`；只有 OwnerActor 已无效时才调用 `ClearActorInfo()`。

### P2 - PlayerState 保留引擎默认 1 Hz 网络更新频率

文件：`Source/Apecox/Private/Player/ApecoxPlayerState.cpp:7`

UE 5.8 的 `APlayerState` 构造函数默认 `NetUpdateFrequency=1`。ASC 和公开 Attribute 位于 PlayerState 时，这会让基础状态复制出现明显延迟。Lyra 的同类所有权方案将其设置为 `100.0f`。

修复：PlayerState 构造函数加入 `SetNetUpdateFrequency(100.0f)`，并解释这是 ASC/Attribute 复制基线而非玩法数值。

### P2 - `MaxHealth` 降低后可能留下 `Health > MaxHealth`

文件：`Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp:59`

当前 MaxHealth 分支只保证 MaxHealth 非负，没有把现有 Health 压回新上限。瞬时 GE 或持续 MaxHealth Modifier 移除后都可能破坏生命值不变量。

修复：增加 `PreAttributeBaseChange`、`PostAttributeChange` 和私有 `ClampAttribute`。基础值和最终值使用同一钳制规则；MaxHealth 改变后保证现有 Health 不大于新上限。

### P2 - AttributeSet 硬编码了 `100/100` 出生值

文件：`Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp:7`

该行为明确违反已批准的“出生值由初始化 GE 或英雄配置负责”。所谓“避免 GAS 零值警告”没有对应的引擎要求，会让 AttributeSet 同时承担属性结构和英雄初始化数据两种职责。

修复：移除构造函数中的 `InitHealth/InitMaxHealth`，本阶段允许默认零值。

### P2 - ActorInfo 生命周期修改函数被放在 public

文件：`Source/Apecox/Public/Character/ApecoxPlayerCharacter.h:38`

`InitializeAbilitySystem/UninitializeAbilitySystem` 可以取消全部 Ability 并改变 ActorInfo，不应成为任意外部调用者可访问的普通 public API。

修复：保留 ASC 查询函数为 public，把 Init/Uninit 与 Pawn 生命周期函数移动到 protected。相同类内部仍可让新 Character 调用旧 Character 的 protected 解绑函数。

### P3 - 报告对 Attribute 的陈述自相矛盾

文件：`Agent/Reports/2026-08-06_Phase1A_Player_ASC_Lifecycle_Report.md`

报告第 6 节声称没有新增 Attribute，但本批实际新增了 `Health/MaxHealth`。修复报告必须明确列出这两个 Attribute，不修改原报告历史。

## 验证缺口

- UHT 和所有 `.cpp` 编译成功，但最终 DLL 链接被正在运行的 Unreal Editor 文件锁阻止，因此尚不能称为完整构建通过。
- 当前 Unreal Editor 仍在运行；修复后关闭编辑器，再执行完整 `ApecoxEditor Win64 Development` 构建。
- 代码复审和完整构建通过前，不开始 UE 手工配置与 PIE。

## 审查后的公开设计调整

- 不增加新类、Tag 或资产。
- `AApecoxPlayerState` 增加 `SetNetUpdateFrequency(100.0f)` 构造设置。
- `AApecoxPlayerCharacter` 的 Init/Uninit 改为 protected，并采用“先清退旧 Avatar、保留有效 OwnerActor”的轻量协议。
- `UApecoxVitalAttributeSet` 增加 `PreAttributeBaseChange`、`PostAttributeChange`、私有 `ClampAttribute`；移除硬编码出生值。

## 2026-08-06 修复后复审追加

首轮六项 Finding 中，两个 P1 和其余基础问题均已正确修复。复审发现一个剩余 P2：

- `ApecoxVitalAttributeSet.cpp` 的 `PostGameplayEffectExecute` 在 MaxHealth 分支调用 `SetMaxHealth(GetMaxHealth())`。`GetMaxHealth()` 是包含持续 Modifier 的当前聚合值，把它写回 Base Value 可能在持续 MaxHealth Buff 存在时重复计入 Modifier。
- `PreAttributeBaseChange/PreAttributeChange/PostAttributeChange` 已经负责 MaxHealth 钳制和 Health 跟随，因此该分支不应再次重写 MaxHealth Base；保留 Health 上限修正即可，或直接依赖 `PostAttributeChange`。

修复后可使用 UBT `-NoLink` 在编辑器运行期间验证编译；完整 DLL 链接仍需关闭编辑器。

## 2026-08-06 最终静态复审

- MaxHealth 当前聚合值回写 Base Value 的问题已经移除。
- 旧 Avatar 清退、PlayerState Owner 保留、Mixed 复制、100 Hz PlayerState 更新、生命周期 API 可见性和 Vital Attribute 不变量均符合当前设计。
- Codex 独立执行 `ApecoxEditor Win64 Development -NoLink`，UBT 返回 `Result: Succeeded`。
- 当前没有剩余代码 Finding。唯一未关闭的工程门禁是：关闭正在运行的 Unreal Editor 后完成一次真实 DLL 链接。

## 完整构建结果

用户关闭 Unreal Editor 后，Codex 执行：

```text
Build.bat ApecoxEditor Win64 Development -Project=D:/UnrealProject/Apecox/Apecox.uproject -WaitMutex -NoHotReloadFromIDE
```

`UnrealEditor-Apecox.lib`、`UnrealEditor-Apecox.dll` 和 Target Metadata 均成功生成，UBT 返回 `Result: Succeeded`。Phase 1A 已通过代码审查和完整构建，可以进入 UE 手工配置与 PIE 验证。

## 运行时验证结论

用户已按照 `Current_UE_Manual_Steps.md` 完成单人 PIE 与两人 Listen Server 验证，全部通过：

- ASC Owner 为对应的 `AApecoxPlayerState`。
- ASC Avatar 为各自当前的 `AApecoxPlayerCharacter`。
- Character 未创建第二个 ASC，两个玩家之间没有 Owner/Avatar 串联。
- 未发现 ActorInfo、复制、重复初始化或崩溃相关异常。
- `Health/MaxHealth=0/0` 符合本阶段无初始化 GE 的预期。

最终结论：**Phase 1A 通过。**
