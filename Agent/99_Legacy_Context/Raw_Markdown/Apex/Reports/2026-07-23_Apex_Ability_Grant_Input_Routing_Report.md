# Apex Ability 授予与输入路由实施报告

*创建日期：2026-07-23*
*执行者：ClaudeCode*
*任务来源：Agent/Prompts/2026-07-23_Apex_Ability_Grant_Input_Routing_ClaudeCode_Prompt.md*

## 1. 实际文件清单

### 新增（10 个）

| 文件 | 路径 |
|------|------|
| `ApexGameplayTags.h` | `Source/Apex/Public/GameplayTags/ApexGameplayTags.h` |
| `ApexGameplayTags.cpp` | `Source/Apex/Private/GameplayTags/ApexGameplayTags.cpp` |
| `ApexGameplayAbility.h` | `Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h` |
| `ApexGameplayAbility.cpp` | `Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp` |
| `ApexSkillDefinition.h` | `Source/Apex/Public/AbilitySystem/Data/ApexSkillDefinition.h` |
| `ApexSkillDefinition.cpp` | `Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp` |
| `ApexAbilitySet.h` | `Source/Apex/Public/AbilitySystem/ApexAbilitySet.h` |
| `ApexAbilitySet.cpp` | `Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp` |
| `ApexAbilityInputConfig.h` | `Source/Apex/Public/Input/ApexAbilityInputConfig.h` |
| `ApexAbilityInputConfig.cpp` | `Source/Apex/Private/Input/ApexAbilityInputConfig.cpp` |

### 修改（7 个）

| 文件 | 变更 |
|------|------|
| `ApexAbilitySystemComponent.h` | 新增输入队列 + 4 个公共函数 + 2 个 override |
| `ApexAbilitySystemComponent.cpp` | 完整实现输入路由、ProcessAbilityInput、AbilitySpecInputPressed/Released |
| `ApexPlayerCharacter.h` | 新增 CoreAbilitySet/AbilityInputConfig 资产成员、Handle、5 个函数 |
| `ApexPlayerCharacter.cpp` | 新增 Ability 授予/撤销、输入路由绑定、UnPossessed/EndPlay 清理 |
| `ApexPlayerController.h` | 新增 PostProcessInput、GetApexAbilitySystemComponent |
| `ApexPlayerController.cpp` | 实现 PostProcessInput 推进 ASC 输入队列 |
| `Apex.Build.cs` | GameplayTags 改为 PublicDependencyModuleNames |

### 是否超出范围

**否。**

## 2. 新增类、结构体、枚举

| 类型 | 名称 | 父类 | 职责 |
|------|------|------|------|
| namespace | `ApexGameplayTags` | — | 5 个 Native GameplayTag 声明与定义 |
| enum | `EApexAbilityActivationPolicy` | uint8 | OnInputTriggered / WhileInputActive |
| class | `UApexGameplayAbility` | `UGameplayAbility` | 所有 Apex GA 根基类：ActivationPolicy、ASC/Character/SkillDefinition 访问 |
| struct | `FApexAbilitySetAbilityGrant` | — | 单个授予条目：SkillDefinition + Level + InputTag |
| struct | `FApexAbilitySetGrantedHandles` | — | 运行时 Handle 容器：Add/TakeFromAbilitySystem/IsEmpty |
| class | `UApexAbilitySet` | `UPrimaryDataAsset` | 只读授予清单：GiveToAbilitySystem + Data Validation |
| struct | `FApexAbilityInputAction` | — | InputAction → InputTag 绑定 |
| class | `UApexAbilityInputConfig` | `UDataAsset` | Ability 输入配置：AbilityInputActions + Data Validation |

## 3. 所有新增成员和函数

### Five Native GameplayTag（namespace ApexGameplayTags）

| C++ 变量 | Tag 字符串 |
|----------|-----------|
| `InputTag_Ability` | `InputTag.Ability` |
| `InputTag_Ability_BasicAttack` | `InputTag.Ability.BasicAttack` |
| `InputTag_Ability_Skill1` | `InputTag.Ability.Skill1` |
| `InputTag_Ability_Skill2` | `InputTag.Ability.Skill2` |
| `InputTag_Ability_Ultimate` | `InputTag.Ability.Ultimate` |

### UApexGameplayAbility

| 成员/函数 | 类型 | UPROPERTY/UFUNCTION | 作用 |
|-----------|------|---------------------|------|
| `ActivationPolicy` | `EApexAbilityActivationPolicy` | EditDefaultsOnly, BlueprintReadOnly | OnInputTriggered / WhileInputActive |
| `GetActivationPolicy()` | `EApexAbilityActivationPolicy` | — | 返回策略 |
| `GetApexAbilitySystemComponentFromActorInfo()` | `UApexAbilitySystemComponent*` | BlueprintPure | 类型安全 ASC |
| `GetApexPlayerCharacterFromActorInfo()` | `AApexPlayerCharacter*` | BlueprintPure | 类型安全 Character |
| `GetSkillDefinition()` | `const UApexSkillDefinition*` | BlueprintPure | 从 SourceObject 取回 SkillDefinition |

### UApexSkillDefinition

| 成员 | 类型 | 作用 |
|------|------|------|
| `AbilityTemplateClass` | `TSubclassOf<UApexGameplayAbility>` | 该技能使用的 GA 模板类 |
| `GetAbilityTemplateClass()` | getter | 同上 |
| `IsDataValid()` | override | 校验 AbilityTemplateClass 非空 |

### FApexAbilitySetAbilityGrant

| 字段 | 类型 |
|------|------|
| `SkillDefinition` | `TObjectPtr<UApexSkillDefinition>` |
| `AbilityLevel` | `int32`（必须 > 0） |
| `InputTag` | `FGameplayTag`（可空） |

### FApexAbilitySetGrantedHandles

| 函数 | 作用 |
|------|------|
| `AddAbilitySpecHandle(Handle)` | 记录授予 Handle |
| `TakeFromAbilitySystem(ASC)` | 权威端 ClearAbility + 清空 |
| `IsEmpty()` | 是否无 Handle |

### UApexAbilitySet

| 成员/函数 | 作用 |
|-----------|------|
| `GrantedAbilities` | 授予条目列表 |
| `GiveToAbilitySystem(ASC, OutHandles)` | 遍历条目 → 验证 → 创建 Spec → GiveAbility |
| `IsDataValid()` | SkillDefinition 非空、Level>0、InputTag 不重复 |

### UApexAbilityInputConfig

| 成员/函数 | 作用 |
|-----------|------|
| `AbilityInputActions` | `TArray<FApexAbilityInputAction>` |
| `GetAbilityInputActions()` | 返回绑定列表 |
| `IsDataValid()` | InputAction 非空、InputTag 有效、无重复 |

### UApexAbilitySystemComponent（新增）

| 成员/函数 | 作用 |
|-----------|------|
| `InputPressedSpecHandles` | 本帧按下的 Handle |
| `InputHeldSpecHandles` | 当前按住中的 Handle |
| `InputReleasedSpecHandles` | 本帧松开的 Handle |
| `AbilityInputTagPressed(InputTag)` | 校验根域并匹配 Spec，只加入 Pressed/Held 队列 |
| `AbilityInputTagReleased(InputTag)` | 加入 Released → 从 Held 移除 |
| `ProcessAbilityInput(DeltaTime, bGamePaused)` | 处理 Held/Pressed → 统一激活 → 处理 Released → 清除本帧队列 |
| `ClearAbilityInput()` | 清空全部队列 |
| `AbilitySpecInputPressed(Spec)` | override → Super + InvokeReplicatedEvent(InputPressed) |
| `AbilitySpecInputReleased(Spec)` | override → Super + InvokeReplicatedEvent(InputReleased) |

### AApexPlayerCharacter（新增）

| 成员/函数 | 作用 |
|-----------|------|
| `CoreAbilitySet` | 核心能力集资产引用 |
| `AbilityInputConfig` | Ability 输入绑定配置引用 |
| `CoreAbilitySetHandles` | 运行时 Handle 容器 |
| `GrantCoreAbilitySet()` | 服务器 PossessedBy 时授予（先撤销再授予防重复） |
| `RemoveCoreAbilitySet()` | 服务器 UnPossessed/EndPlay 时撤销 |
| `InputAbilityTagPressed(Tag)` | 转发到 ASC::AbilityInputTagPressed |
| `InputAbilityTagReleased(Tag)` | 转发到 ASC::AbilityInputTagReleased |
| `UnPossessed()` override | 撤销 CoreAbilitySet |
| `EndPlay()` override | 撤销 CoreAbilitySet |

### AApexPlayerController（新增）

| 函数 | 作用 |
|------|------|
| `PostProcessInput(DeltaTime, bGamePaused)` | 仅本地 Controller 调用 ASC::ProcessAbilityInput |
| `GetApexAbilitySystemComponent()` | 从 PlayerState 取 ASC |

## 4. AbilitySet 授予 → SourceObject → Handle 回收 → 输入队列链路

```
服务器 PossessedBy
→ GrantCoreAbilitySet()
→ UApexAbilitySet::GiveToAbilitySystem()
    → 遍历 GrantedAbilities
    → 校验 SkillDefinition / AbilityTemplateClass / AbilityLevel / InputTag
    → FGameplayAbilitySpec(AbilityTemplateClass, AbilityLevel)
    → Spec.SourceObject = SkillDefinition
    → Spec.DynamicSourceTags += InputTag
    → ASC::GiveAbility(Spec)
    → OutGrantedHandles.AddAbilitySpecHandle(Handle)

输入按下
→ Character::SetupPlayerInputComponent
    → 遍历 AbilityInputConfig
    → BindAction(InputAction, Started, InputAbilityTagPressed, InputTag)
→ InputAbilityTagPressed(InputTag)
    → ASC::AbilityInputTagPressed(InputTag)
    → 校验 InputTag.Ability 根域
    → 匹配 DynamicSourceTags
    → 只写入 Pressed/Held 队列

每帧
→ PlayerController::PostProcessInput
    → ASC::ProcessAbilityInput
    → Held 收集 WhileInputActive
    → Pressed 设置 Spec.InputPressed、发送 Active Spec 输入事件并收集 OnInputTriggered
    → 统一 TryActivateAbility
    → Released 清除 Spec.InputPressed 并发送 Active Spec 输入事件
    → 清空 Pressed/Released

解锁/结束
→ UnPossessed/EndPlay
    → RemoveCoreAbilitySet()
    → ASC::ClearAbilityInput()
    → FApexAbilitySetGrantedHandles::TakeFromAbilitySystem()
    → ASC::ClearAbility(Handle)
```

## 5. Character 授予/撤销时序

- `PossessedBy`（服务器）→ InitActorInfo → GrantCoreAbilitySet
- `OnRep_PlayerState`（客户端）→ 仅 InitActorInfo（AbilitySpec 通过 GAS 复制）
- `UnPossessed`（权威端）→ RemoveCoreAbilitySet
- `EndPlay`（权威端）→ RemoveCoreAbilitySet
- GrantCoreAbilitySet 先撤销再授予，保证幂等

## 6. Data Validation 规则

| 类 | 规则 |
|----|------|
| `UApexSkillDefinition` | AbilityTemplateClass 非空 |
| `UApexAbilitySet` | SkillDefinition 非空、AbilityLevel > 0、有效 InputTag 必须是 InputTag.Ability 具体子 Tag、同一 AbilitySet 内 InputTag 不重复 |
| `UApexAbilityInputConfig` | InputAction 非空、InputTag 必须是 InputTag.Ability 具体子 Tag、InputAction 不重复、InputTag 不重复 |

## 7. 未新增

- 无 GA、GE、Cue、CombatEntity、AbilityTask、技能模板 ✅
- 无 UE 资产、蓝图、地图 ✅
- 无 OnSpawn、ActivationGroup、InputBlocked、TagRelationshipMapping ✅
- 无 StructUtils / FInstancedStruct ✅
- 无 MCP 操作 ✅

## 8. 最终编译结果

```
Result: Succeeded
Total execution time: 50.93 seconds
Exit code: 0
0 errors, 0 warnings
```

## 9. 未执行的验证

- 单人 PIE 验证（需用户创建 InputProbe 蓝图和配置资产后执行）
- Listen Server + Client 多人验证
- Blueprint 父类检查

## 10. 首次审查修复（2026-07-23）

### 修复清单

| # | 问题 | 修复 |
|---|------|------|
| 1 | 输入队列处理不完整：Pressed/Released 未处理，Spec.InputPressed 未更新，TryActivateAbility 在遍历中调用 | 完整重写 ProcessAbilityInput：Held 收集 WhileInputActive → Pressed 设 InputPressed + 发事件 + 收集 OnInputTriggered → 统一 TryActivateAbility → Released 清除 InputPressed + 发事件 → 清空本帧 Pressed/Released |
| 2 | PredictionKey 使用已弃用的 Spec.ActivationInfo | 优先从 Spec.GetPrimaryInstance()->GetCurrentActivationInfo() 获取；PRAGMA_DISABLE_DEPRECATION_WARNINGS 包围 legacy fallback。0 个 C4996 |
| 3 | SkillDefinition 错误使用 RequiredAssetDataTags RowStructure | 删除该 meta；增加 InputTag Categories="InputTag.Ability"；修正幂等注释 |
| 4 | InputTag 根域未落实 | 4 处检查：ASC Pressed/Released 入口拒绝、AbilitySet 运行时授予 + Data Validation、InputConfig Data Validation |
| 5 | 输入队列跨 Avatar 残留 | RemoveCoreAbilitySet 在客户端/服务端均调用 ASC->ClearAbilityInput()，再权威端 TakeFromAbilitySystem |
| 6 | 权威检查用 GetOwnerActor()->HasAuthority() | 改为 ASC->IsOwnerActorAuthoritative()（FApexAbilitySetGrantedHandles::TakeFromAbilitySystem + UApexAbilitySet::GiveToAbilitySystem） |

### 输入队列新顺序

```
InputTag 回调只入队（Pressed→Pressed+Held, Released→Released-Held）
ProcessAbilityInput:
  1. Held → 收集 WhileInputActive 未激活 → AbilitiesToActivate
  2. Pressed → Spec.InputPressed=true → 已激活发 AbilitySpecInputPressed → 未激活 OnInputTriggered → AbilitiesToActivate
  3. 统一 TryActivateAbility
  4. Released → Spec.InputPressed=false → 已激活发 AbilitySpecInputReleased
  5. Reset Pressed/Released
```

### 修改文件

| 文件 | 变更 |
|------|------|
| `ApexAbilitySystemComponent.cpp` | 完整重写输入队列 + PredictionKey 修复 |
| `ApexAbilitySet.h` | 元数据修正 + 注释修正 |
| `ApexAbilitySet.cpp` | Tag 根域验证 + 权威检查 + Data Validation |
| `ApexPlayerCharacter.cpp` | RemoveCoreAbilitySet 增加 ClearAbilityInput |
| `ApexAbilityInputConfig.cpp` | Data Validation 增加 Tag 根域检查 |
| `2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md` | 追加本章 |

### 编译结果

```
Result: Succeeded
Total execution time: 40.69 seconds
Exit code: 0
0 errors, 0 warnings（含不再出现 C4996 ActivationInfo）
```

## 11. 第二次审查修复（2026-07-23）

### 修复

| # | 问题 | 修复 |
|---|------|------|
| 1 | Pressed 循环中 `if (AbilitiesToActivate.Contains(Handle)) continue;` 导致 WhileInputActive Handle 的 InputPressed 和 AbilitySpecInputPressed 被跳过 | 删除该 guard；Pressed 循环对所有 Handle 设置 Spec.InputPressed 并发送输入事件；OnInputTriggered 分支仍通过 AddUnique 去重 |
| 2 | ApexAbilitySet.h 注释拼写 TOjectPtr | 修正为 TObjectPtr |
| 3 | 报告前半部分旧描述与最终代码矛盾 | 修正 §4 运行链路、§6 Data Validation、编译结论 |

### 编译结果

```
Result: Succeeded
Total execution time: 50.93 seconds
Exit code: 0
0 errors, 0 warnings
```

### 修改文件

- `ApexAbilitySystemComponent.cpp` — 删除 Pressed 循环的 AbilitiesToActivate.Contains guard
- `ApexAbilitySet.h` — 拼写修正
- 本报告 — 追加本章
