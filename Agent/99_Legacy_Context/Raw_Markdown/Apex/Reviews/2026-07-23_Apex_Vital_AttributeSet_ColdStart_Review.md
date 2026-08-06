# Apex Vital AttributeSet 冷启动审查

审查日期：2026-07-23
状态：复审通过。

## 范围结论

- 新增 4 个 AttributeSet 文件、修改 2 个 PlayerState 文件，符合 Prompt 范围。
- 未新增 GameplayTag、GA、GE、ExecCalc、Meta Attribute、DataAsset、GameplayCue 或 UE 资产。
- 子代理报告记录编译为 0 error、0 warning。

## P1 - 四个 Attribute 未真正初始化为 100

位置：

```text
Source/Apex/Private/AbilitySystem/Attributes/ApexVitalAttributeSet.cpp
Source/Apex/Public/AbilitySystem/Attributes/ApexVitalAttributeSet.h
```

当前构造函数为空，并注释“默认值在头文件初始化”，但头文件中的四个 `FGameplayAttributeData` 没有默认初始化器。实际初始值会是 0：

```cpp
UApexVitalAttributeSet::UApexVitalAttributeSet()
{
}
```

这会导致：

- `Health`、`MaxHealth`、`Mana`、`MaxMana` 初始值不是已确认的 100。
- 初始 `MaxHealth=0` 违反 `MaxHealth >= 1` 的设计不变量。
- `showdebug abilitysystem` 的人工验收无法通过。
- 实施报告中“四项初始值为 100”的结论与代码不一致。

修复要求：在构造函数初始化列表中显式初始化四项属性：

```cpp
UApexVitalAttributeSet::UApexVitalAttributeSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Mana(100.0f)
	, MaxMana(100.0f)
{
}
```

删除“默认值在头文件初始化”的错误注释。

## P2 - PlayerState 注释错误描述对象归属

位置：

```text
Source/Apex/Private/Player/ApexPlayerState.cpp
```

当前注释称 `VitalAttributeSet` “作为 ASC 的子对象创建”，但调用发生在 `AApexPlayerState` 构造函数中：

```cpp
VitalAttributeSet = CreateDefaultSubobject<UApexVitalAttributeSet>(TEXT("VitalAttributeSet"));
```

它与 ASC 一样是 PlayerState 的默认子对象，随后由 ASC 发现并注册。建议改为：

```cpp
// Vital AttributeSet 与 ASC 都由 PlayerState 持有；ASC 初始化时会发现并注册该默认子对象。
```

这是学习型注释，归属关系必须准确。

## 其余审查结果

- `UApexAttributeSet` 的无业务属性基类边界正确。
- `APEX_ATTRIBUTE_ACCESSORS` 展开正确。
- Health / MaxHealth 使用 `COND_None`，Mana / MaxMana 使用 `COND_OwnerOnly`，符合设计。
- 四个 RepNotify 正确调用 `GAMEPLAYATTRIBUTE_REPNOTIFY`。
- BaseValue / CurrentValue 两条钳制路径均已覆盖。
- MaxHealth 下限 1、MaxMana 下限 0，规则正确。
- 最大值降低时经 ASC 修改当前值，符合当前阶段设计。
- `AApexPlayerState` 的成员名、类型、Getter 和默认子对象创建位置正确。

## 修复后验收

1. 只修改上述两个 `.cpp` 文件和原实施报告。
2. 重新执行 `ApexEditor Win64 Development` 编译。
3. 报告中明确更正初始值实现方式。
4. Codex 复审通过后再由用户重新生成 Rider 项目文件并进入 PIE。

## 复审结果

2026-07-23 复审通过：

- `UApexVitalAttributeSet` 已在构造函数初始化列表中将四项 Attribute 显式初始化为 100。
- `AApexPlayerState` 注释已正确说明 ASC 与 Vital AttributeSet 均由 PlayerState 持有。
- 实施报告已补充两项修复记录。
- 修复后 `ApexVitalAttributeSet.cpp.obj`、`ApexPlayerState.cpp.obj` 和 `UnrealEditor-Apex.dll` 均重新生成，时间晚于源码修改，确认修复版本已经完成编译和链接。

当前没有遗留代码审查问题，可以进入 Rider 项目文件刷新和 UE 单人/多人 PIE 验证。

## 运行验证

用户已于 2026-07-23 完成：

- 单人 PIE 验证。
- Listen Server + Client 多人验证。
- Vital AttributeSet 挂载和四项初始值验证。
- 移动、镜头和动画回归验证。

运行结果通过，本批正式完成。
