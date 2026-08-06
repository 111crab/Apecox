# Phase 1A 修复报告：玩家与 ASC 生命周期

日期：2026-08-06  
执行者：ClaudeCode（子代理）  
范围：Codex 审查后的五项定向修复，不涉及 Phase 1B

---

## 一、修复总览

| # | 修复项 | 严重程度 | 涉及文件 | 状态 |
| --- | --- | --- | --- | --- |
| 1 | 新 Pawn 绑定前清退 ASC 已有旧 Avatar | P1 | Character .h/.cpp | ✅ |
| 2 | 正常解绑保留 PlayerState OwnerActor | P1 | Character .cpp | ✅ |
| 3 | 收紧生命周期 API（Init/Uninit → protected） | P2 | Character .h | ✅ |
| 4 | PlayerState 网络更新频率 100 Hz | P2 | PlayerState .cpp | ✅ |
| 5 | Vital Attribute 不变量与移除硬编码出生值 | P2 | VitalAttributeSet .h/.cpp | ✅ |

---

## 二、逐项修复说明

### 修复 1：新 Pawn 绑定前清退 ASC 已有旧 Avatar

**问题**：`InitializeAbilitySystem()` 只处理"本 Character 缓存了另一个 ASC"，没有检查目标 ASC 上是否仍绑定着旧 Avatar。客户端复制乱序时，新 Pawn 可能先到达并直接覆盖 ActorInfo，旧 Pawn 的活动 Ability 未被取消。

**修复**：在 `AApecoxPlayerCharacter::InitializeAbilitySystem()` 中，调用 `InitAbilityActorInfo` 之前：

1. 读取 `ASC->GetAvatarActor()` 获取现有 Avatar
2. 若 ExistingAvatar 非空且不是 `this`：
   - 若为 `AApecoxPlayerCharacter`：调用 `OldCharacter->UninitializeAbilitySystem()`，让旧 Character 对称解绑（同类 protected 函数可互相访问）
   - 若不是 Apecox Character：`ensureMsgf` 暴露异常，调用 `ASC->SetAvatarActor(nullptr)` 做最小安全清理（保留 OwnerActor）
3. 确认旧 Avatar 已处理后，再执行 `InitAbilityActorInfo(ApecoxPS, this)`

**同时修复**：处理本 Character 缓存了另一个 ASC 的情况时，从手写清理代码改为复用 `UninitializeAbilitySystem()`。

### 修复 2：正常解绑保留 PlayerState OwnerActor

**问题**：`UninitializeAbilitySystem()` 在正常换 Pawn 路径调用 `ClearActorInfo()`，同时清空 Owner 和 Avatar。但 PlayerState 才是 ASC 的持久 Owner——它必须在重生间隙保持逻辑归属。

**修复**：`UninitializeAbilitySystem()` 在确认 `GetAvatarActor() == this` 后：

```text
CancelAllAbilities()
if GetOwnerActor() != nullptr:
    SetAvatarActor(nullptr)      ← 保留 OwnerActor，只解除 Avatar
else:
    ClearActorInfo()             ← OwnerActor 已失效时才全部清空
最后无条件清空本 Character 的缓存引用
```

- 旧 Character 延迟清理时，若 ASC 已绑定新 Avatar（`GetAvatarActor() != this`），只清空自己的缓存，不影响新 Avatar。
- 不调用 `RemoveAllGameplayCues()`。
- 与修复 1 协同：新 Pawn 初始化时主动清退旧 Avatar，旧 Pawn 之后的延迟清理成为安全的无操作。

### 修复 3：收紧生命周期 API

**问题**：`InitializeAbilitySystem()` 和 `UninitializeAbilitySystem()` 可以取消全部 Ability 并改变 ActorInfo，却暴露在 `public` 中。

**修复**：

| 函数 | 旧可见性 | 新可见性 | 理由 |
| --- | --- | --- | --- |
| `GetAbilitySystemComponent()` | public | public（不变） | 外部系统需查询 ASC |
| `GetApecoxAbilitySystemComponent()` | public | public（不变） | 项目强类型访问 |
| `InitializeAbilitySystem()` | public | **protected** | ActorInfo 变更不应成为任意外部 API |
| `UninitializeAbilitySystem()` | public | **protected** | 同上 |
| `PossessedBy` / `OnRep_PlayerState` / `UnPossessed` / `EndPlay` | protected | protected（不变） | Pawn 生命周期覆盖 |

同一 `AApecoxPlayerCharacter` 的成员函数可调用另一实例的 protected 生命周期函数——此规则用于修复 1 中新 Pawn 清退旧 Avatar。

### 修复 4：PlayerState 网络更新频率

**问题**：`APlayerState` 引擎默认 `NetUpdateFrequency = 1 Hz`，ASC 和公开 Attribute 放在 PlayerState 时复制延迟明显。

**修复**：在 `AApecoxPlayerState` 构造函数末尾加入：

```cpp
SetNetUpdateFrequency(100.0f);
```

附中文注释说明这是 ASC/Attribute 复制基线而非玩法属性。未增加自定义复制、RPC 或新成员。

### 修复 5：Vital Attribute 不变量与初始化职责

**问题**：
- 构造函数硬编码 `InitHealth(100.0f)` / `InitMaxHealth(100.0f)` 违反"出生值由初始化 GE 或英雄配置负责"
- 缺少 `PreAttributeBaseChange`：基础值可以绕过 `PreAttributeChange` 的钳制
- 缺少 `PostAttributeChange`：MaxHealth 降低后 Health 可能 > 新上限
- `PostGameplayEffectExecute` MaxHealth 分支没有保证 Health ≤ MaxHealth

**修复**：

| 变更 | 说明 |
| --- | --- |
| 移除构造函数中的 `InitHealth(100.0f)` 和 `InitMaxHealth(100.0f)` | 构造函数改为 `= default`；允许默认零值 |
| 新增 `PreAttributeBaseChange` override | 与 `PreAttributeChange` 使用同一 `ClampAttribute` |
| 新增 `PostAttributeChange` override | MaxHealth 降低且 Health > NewValue 时，通过 `ASC->SetNumericAttributeBase` 把 Health 压回新上限 |
| 新增 `private ClampAttribute` | 统一钳制：MaxHealth ≥ 0，Health ∈ [0, MaxHealth] |
| `PostGameplayEffectExecute` MaxHealth 分支增强 | 除了钳制 MaxHealth ≥ 0，新增 Health ≤ MaxHealth 检查和压回 |
| `PostGameplayEffectExecute` Health 分支 | 保持原有钳制 |

`PostAttributeChange` 中通过 `ASC->SetNumericAttributeBase` 写入而非直接操作 `FGameplayAttributeData` 内部字段，满足"使用 ASC 标准属性修改入口"的要求。

---

## 三、实际修改文件清单

| 文件 | 修改内容 | 是否超出允许范围 |
| --- | --- | --- |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | Init/Uninit 从 public → protected；注释更新 | 否 |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 修复 1+2：旧 Avatar 清退逻辑、OwnerActor 保留、复用 UninitializeAbilitySystem | 否 |
| `Source/Apecox/Private/Player/ApecoxPlayerState.cpp` | 修复 4：`SetNetUpdateFrequency(100.0f)` | 否 |
| `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h` | 修复 5：新增 PreAttributeBaseChange/PostAttributeChange/ClampAttribute 声明；构造函数 → `= default` | 否 |
| `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | 修复 5：移除硬编码出生值；实现新增函数；增强 PostGameplayEffectExecute；Include `AbilitySystemComponent.h` | 否（Include 为编译必须的最小修正） |

**未修改的文件**：`AApecoxGameMode`、`AApecoxGameState`、`AApecoxPlayerController`、`UApecoxAbilitySystemComponent`、`Apecox.Build.cs`——全部保持首轮实现不变。

**Codex 的 Markdown 文件**：`Current_Code_Design.md`、`Current_Phase.md` 等均未触碰。

---

## 四、成员与函数可见性变化

### AApecoxPlayerCharacter

| 成员/函数 | 旧可见性 | 新可见性 |
| --- | --- | --- |
| `InitializeAbilitySystem()` | public | protected |
| `UninitializeAbilitySystem()` | public | protected |

其他所有函数可见性不变。

### UApecoxVitalAttributeSet

| 成员/函数 | 状态 |
| --- | --- |
| `UApecoxVitalAttributeSet()` | 声明从显式定义改为 `= default`；.cpp 定义已移除 |
| `PreAttributeBaseChange(...)` | **新增** public override |
| `PostAttributeChange(...)` | **新增** public override |
| `ClampAttribute(...)` | **新增** private const |
| `PreAttributeChange(...)` | 保持不变（实现改为委托 `ClampAttribute`） |
| `PostGameplayEffectExecute(...)` | 保持不变（MaxHealth 分支增强） |

---

## 五、本批新增内容确认

| 类别 | 是否新增 | 说明 |
| --- | --- | --- |
| Attribute | **否**（仅修改） | `Health`/`MaxHealth` 为首轮已创建；本批只修改约束逻辑 |
| GameplayTag | 否 | — |
| GameplayAbility | 否 | — |
| GameplayEffect | 否 | — |
| GameplayCue | 否 | — |
| DataAsset | 否 | — |
| AbilityTask | 否 | — |
| 蓝图 / .uasset | 否 | — |
| 新 C++ 类 | 否 | — |

---

## 六、ClearActorInfo 正常路径要求纠正说明

原 Phase 1A 实施 Prompt 第 6.5 节第 6 条要求：

> `UninitializeAbilitySystem` 只有在 `CachedAbilitySystemComponent->GetAvatarActor() == this` 时，才能对当前 Avatar 调用 `CancelAllAbilities()` 和 `ClearActorInfo()`

此项约束来自 Codex 最初的设计，但审查发现 `ClearActorInfo()` 同时清空 Owner 和 Avatar，会破坏 PlayerState 作为跨 Pawn 持久 Owner 的架构前提。Codex 已在审查中修正设计口径——本次修复将正常换 Pawn 路径改为 `SetAvatarActor(nullptr)` 保留 OwnerActor，仅 OwnerActor 已失效时回退到 `ClearActorInfo()`。

---

## 七、编译结果

**未执行完整编译。**

原因：Unreal Editor（PID 47416）仍在运行，持有 `UnrealEditor-Apecox.dll` 文件锁。按 Prompt 要求不强制终止进程，也不反复执行注定因 DLL 文件锁失败的链接。

- 首轮编译已验证所有 `.cpp` 和 UHT 生成代码零错误零警告通过。
- 本轮修改仅涉及 3 个 `.cpp` 文件（Character / PlayerState / VitalAttributeSet）和 2 个 `.h` 文件（Character / VitalAttributeSet）——均为纯逻辑修改，不引入新依赖或新 UHT 反射声明。
- 关闭 UE Editor 后执行相同编译命令即可完成完整链接。

---

## 八、未验证项与残余风险

| 事项 | 状态 | 建议 |
| --- | --- | --- |
| 完整 DLL 链接 | 被运行中的 Unreal Editor 阻塞 | 关闭编辑器后重新编译 |
| 单人 PIE | 需完整构建 + UE Editor | 用户按 Codex 后续 `Current_UE_Manual_Steps.md` 执行 |
| 2 人 Listen Server | 需 PIE 基础通过 | 验证 Owner/Avatar 关系、旧 Pawn 清退、属性复制 |
| 旧 Avatar 清退的复制乱序场景 | 代码已实现但未经网络测试 | Listen Server 中可通过 Respawn/换 Pawn 验证 |
| PostAttributeChange 递归风险 | 逻辑上安全（仅 MaxHealth 分支触发，且 SetNumericAttributeBase 后 ClampAttribute 确保单一方向收敛） | 实际 GE 压力测试后确认 |

---

## 九、后续建议

1. **关闭 UE Editor** → 重新执行编译命令
2. **Codex 复审** → 确认修复满足审查要求
3. **Refresh Rider** → 用户刷新项目文件以识别 Visibility 变更
4. **PIE 验证** → 按 Codex 覆盖的 `Current_UE_Manual_Steps.md` 执行

---

**修复完成。** 未继续 Phase 1B，未提交 Git，未操作 UE 资产。

---

## 二次修复（2026-08-06，-NoLink 编译通过）

### 问题来源

Codex 修复后复审追加（见审查文件末尾）：`PostGameplayEffectExecute` 的 MaxHealth 分支调用 `SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f))`。`GetMaxHealth()` 返回的是包含活跃 Modifier 的当前聚合值，将其写回 Base Value 在持续 MaxHealth Buff 存在时会导致 Modifier 被重复计入（"bake in"效应），造成数值膨胀。

当前已具备的防护链：
- `PreAttributeBaseChange` 钳制 MaxHealth Base
- `PreAttributeChange` 钳制最终值
- `PostAttributeChange` 在 MaxHealth 下降后压低 Health

因此 `PostGameplayEffectExecute` 的 MaxHealth 分支不再需要重写 SetMaxHealth。

### 实际修改

| 文件 | 修改内容 |
| --- | --- |
| `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | 删除 `SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f))` 回写；Health 超出 MaxHealth 的防御检查改用 `ASC->SetNumericAttributeBase` 标准入口 |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` 第 16 行 | 注释从"清理自己的 ActorInfo"修正为"解除自己的 Avatar 关系"（仅注释，不改 API） |

修改后的 `PostGameplayEffectExecute` MaxHealth 分支：

```cpp
else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
{
    // 不调用 SetMaxHealth(GetMaxHealth())：
    // GetMaxHealth() 是包含活跃 Modifier 的当前聚合值，把它写回 Base Value
    // 会在持续 MaxHealth Buff 存在时重复计入 Modifier，造成数值膨胀。
    // MaxHealth Base 钳制和最终值约束已由 PreAttributeBaseChange/PreAttributeChange 负责，
    // MaxHealth 下降后 Health 跟随逻辑已由 PostAttributeChange 处理。

    // 防御检查：GE 执行后若 Health 仍超出当前 MaxHealth，通过 ASC 标准入口压低
    const float CurrentHealth = GetHealth();
    const float CurrentMaxHealth = GetMaxHealth();
    if (CurrentHealth > CurrentMaxHealth)
    {
        UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
        if (ensure(ASC))
        {
            ASC->SetNumericAttributeBase(GetHealthAttribute(), CurrentMaxHealth);
        }
    }
}
```

### -NoLink 编译结果

```text
命令: Build.bat ApecoxEditor Win64 Development -NoLink

[1/5] Compile [x64] ApecoxGameMode.cpp        ✓
[2/5] Compile [x64] ApecoxPlayerCharacter.cpp  ✓
[3/5] Compile [x64] ApecoxPlayerState.cpp      ✓
[4/5] Compile [x64] ApecoxVitalAttributeSet.cpp ✓
[5/5] Compile [x64] Module.Apecox.gen.cpp      ✓

Result: Succeeded
```

UHT 和全部 5 个 C++ 编译单元零错误零警告通过。`-NoLink` 被 Build.bat 正确接受，跳过了 DLL 链接步骤。完整链接需关闭 UE Editor 后执行。

> 注意：编译期间 UBA 因系统内存紧张（~58GB/60.9GB）多次 Kill 并自动重试编译进程，最终全部通过。这是本机资源问题，与代码无关。
