# Apex Vital AttributeSet 冷启动实施报告

*创建日期：2026-07-23*
*执行者：ClaudeCode*
*任务来源：Agent/Prompts/2026-07-23_Apex_Vital_AttributeSet_ColdStart_ClaudeCode_Prompt.md*

## 1. 实际文件清单

### 新增（4 个）

| 文件 | 路径 |
|------|------|
| `ApexAttributeSet.h` | `Source/Apex/Public/AbilitySystem/Attributes/ApexAttributeSet.h` |
| `ApexAttributeSet.cpp` | `Source/Apex/Private/AbilitySystem/Attributes/ApexAttributeSet.cpp` |
| `ApexVitalAttributeSet.h` | `Source/Apex/Public/AbilitySystem/Attributes/ApexVitalAttributeSet.h` |
| `ApexVitalAttributeSet.cpp` | `Source/Apex/Private/AbilitySystem/Attributes/ApexVitalAttributeSet.cpp` |

### 修改（2 个）

| 文件 | 变更 |
|------|------|
| `ApexPlayerState.h` | 新增 `UApexVitalAttributeSet` 前向声明、`VitalAttributeSet` 成员、`GetVitalAttributeSet()` |
| `ApexPlayerState.cpp` | 构造函数创建 `VitalAttributeSet`、新增 getter 实现 |

### 是否超出范围

**否。** 未修改 Build.cs、uproject、配置、蓝图或任何其他文件。

## 2. 新增类

| 类 | 父类 | 路径 | 职责 |
|----|------|------|------|
| `UApexAttributeSet` | `UAttributeSet` | `Public/AbilitySystem/Attributes/` | 项目公共基类：提供 `APEX_ATTRIBUTE_ACCESSORS` 宏 + `GetApexAbilitySystemComponent()`；不保存业务属性 |
| `UApexVitalAttributeSet` | `UApexAttributeSet` → `UAttributeSet` | `Public/AbilitySystem/Attributes/` | 生命与法力资源集：Health、MaxHealth、Mana、MaxMana 及其钳制、复制、最大值联动 |

## 3. 四个 Attribute

| 属性 | 类型 | 初始值 | 复制条件 | 作用 |
|------|------|--------|----------|------|
| `Health` | `FGameplayAttributeData` | 100 | `COND_None` | 当前生命，全端可见 |
| `MaxHealth` | `FGameplayAttributeData` | 100 | `COND_None` | 最大生命，全端可见 |
| `Mana` | `FGameplayAttributeData` | 100 | `COND_OwnerOnly` | 当前法力，仅拥有者可见 |
| `MaxMana` | `FGameplayAttributeData` | 100 | `COND_OwnerOnly` | 最大法力，仅拥有者可见 |

所有属性使用 `BlueprintReadOnly`、`ReplicatedUsing`、`Category="Apex|Vital"`、`AllowPrivateAccess`。

Health/MaxHealth 全端复制是因为所有玩家都需要看到他人的血量（UI、目标框等）。Mana/MaxMana 仅拥有者复制，因为法力值对其他玩家属于隐藏信息。

## 4. AApexPlayerState 变更

| 新增成员 | 类型 | 访问 | UPROPERTY |
|----------|------|------|-----------|
| `VitalAttributeSet` | `TObjectPtr<UApexVitalAttributeSet>` | private | `VisibleAnywhere, BlueprintReadOnly, AllowPrivateAccess` |

| 新增函数 | 返回类型 | 作用 |
|----------|----------|------|
| `GetVitalAttributeSet()` | `const UApexVitalAttributeSet*` | 只读暴露 Vital 属性集 |

## 5. 所有新增函数

| 函数 | 类 | 作用 |
|------|-----|------|
| `GetApexAbilitySystemComponent()` | `UApexAttributeSet` | 返回类型安全的 Apex ASC |
| `UApexVitalAttributeSet()` | `UApexVitalAttributeSet` | 构造函数 |
| `GetLifetimeReplicatedProps()` | `UApexVitalAttributeSet` | 注册四项属性复制 + RepNotify |
| `PreAttributeBaseChange()` | `UApexVitalAttributeSet` | BaseValue 变化前钳制 |
| `PreAttributeChange()` | `UApexVitalAttributeSet` | CurrentValue 变化前钳制 |
| `PostAttributeChange()` | `UApexVitalAttributeSet` | 最大值降低时压低当前值 |
| `ClampAttribute()` | `UApexVitalAttributeSet` | 统一钳制实现 |
| `OnRep_Health/MaxHealth/Mana/MaxMana()` | `UApexVitalAttributeSet` | RepNotify，使用 `GAMEPLAYATTRIBUTE_REPNOTIFY` |

## 6. 钳制及最大值联动规则

```
Health:    0 <= Health <= MaxHealth
MaxHealth: 1 <= MaxHealth (防止除以零)
Mana:      0 <= Mana <= MaxMana
MaxMana:   0 <= MaxMana
```

最大值降低联动：
- MaxHealth 降低且 Health > 新上限 → `ASC->SetNumericAttributeBase(Health, NewMaxHealth)`
- MaxMana 降低且 Mana > 新上限 → `ASC->SetNumericAttributeBase(Mana, NewMaxMana)`
- 通过 ASC 属性修改入口写入，保证 PreAttributeBaseChange 钳制再次生效
- 最大值提高时不自动补满，不保持比例

## 7. 未新增

- 无 GameplayTag ✅
- 无 GA、GE ✅
- 无 ExecCalc ✅
- 无 Meta Attribute（IncomingDamage、IncomingHealing） ✅
- 无 DataAsset ✅
- 无 GameplayCue ✅
- 无蓝图或 UE 资产 ✅
- 无 Build.cs / uproject 修改 ✅

## 8. 编译结果

```
Result: Succeeded
Total execution time: 18.95 seconds
Exit code: 0
0 errors, 0 warnings
```

## 9. 未执行的验证

- PIE 验证（需用户）
- Listen Server + Client 多人验证（需用户）
- 蓝图检查

## 10. Codex 审查修复（2026-07-23）

| # | 问题 | 修复 |
|---|------|------|
| P1 | 四个 Attribute 未初始化为 100（构造函数空 + 头文件无默认值） | 构造函数初始化列表显式初始化 Health=100, MaxHealth=100, Mana=100, MaxMana=100；删除错误注释 |
| P2 | PlayerState 注释错误声称 VitalAttributeSet "作为 ASC 的子对象创建" | 改为 "Vital AttributeSet 与 ASC 都由 PlayerState 持有；ASC 初始化时会发现并注册该默认子对象" |
