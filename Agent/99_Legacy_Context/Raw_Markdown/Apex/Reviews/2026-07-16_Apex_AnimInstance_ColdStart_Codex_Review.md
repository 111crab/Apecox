# Apex AnimInstance 冷启动审查

生成日期：2026-07-16
审查对象：`Agent/Reports/2026-07-16_Apex_AnimInstance_ColdStart_Report.md` 及对应 C++ 改动。
当前结论：复审通过；等待用户确认后生成 Rider 项目文件并进入 UE 人工配置。

## 1. 审查发现

### P2：MovementComponent 失效后不会重新缓存

位置：`Source/Apex/Private/Animation/ApexAnimInstance.cpp:20`

`NativeUpdateAnimation()` 当前只在 `OwningCharacter` 无效或 Pawn 已切换时调用 `CacheOwnerReferences()`。如果 `OwningCharacter` 仍有效、但缓存的 `CharacterMovementComponent` 已失效，代码会在后续检查中每帧重置动画状态并返回，却不会尝试重新获取 MovementComponent。

这在当前标准 `ACharacter` 生命周期中不容易触发，但与本批已经确认的“任一缓存引用无效时重新缓存”规则不一致，也会削弱该通用 AnimInstance 面对组件重建或未来扩展时的恢复能力。

建议将重新缓存条件改为同时检查：

```cpp
if (!::IsValid(OwningCharacter.Get())
	|| !::IsValid(CharacterMovementComponent.Get())
	|| OwningCharacter != TryGetPawnOwner())
{
	CacheOwnerReferences();
}
```

仅修改上述条件及相邻注释，不改变类、成员、函数或其他逻辑。

## 2. 已通过项目

- `UApexAnimInstance : UAnimInstance` 的职责和命名符合事前设计。
- `UCLASS(Transient, Blueprintable)` 与 `APEX_API` 正确。
- Public/Private 文件布局正确。
- `GroundSpeed`、`VerticalSpeed`、`bHasAcceleration`、`bIsFalling` 的类型、可见性和计算口径符合设计。
- 使用通用 `ACharacter`，没有把动画层绑定到 `AApexPlayerCharacter`。
- `NativeInitializeAnimation()` 与 `NativeUpdateAnimation()` 均先调用 `Super`。
- 编辑器预览无 Pawn 时能够使用安全默认值，不会直接解引用空 Owner。
- `AApexPlayerCharacter::SetupPlayerInputComponent()` 已补充父类调用，且不会引入重复的项目自定义绑定。
- 未修改 `Apex.Build.cs`、配置、蓝图、地图或 GAS 代码，实施范围合规。
- 子代理报告记录的 `ApexEditor Win64 Development` 最终编译成功。
- `git diff --check` 通过；仅出现现有换行符转换提示，没有空白错误。

## 3. 复审门槛

子代理完成上述单点修复后：

1. 重新执行 `ApexEditor Win64 Development` 编译。
2. 报告实际修改行和编译结果，不创建新类或新资产。
3. Codex 复审通过后，再运行 `Scripts/RegenerateProjectFiles.ps1` 并覆盖 UE 人工操作清单。

## 4. 复审结果

复审日期：2026-07-16

- `NativeUpdateAnimation()` 已在重新缓存条件中加入 `CharacterMovementComponent` 有效性检查。
- 相邻注释已同步为“任一缓存引用无效”。
- 未修改类名、成员变量、函数签名或其他动画逻辑。
- 子代理报告记录 `ApexEditor Win64 Development` 增量编译成功。
- 本轮 P2 问题关闭，当前没有阻塞 Rider 项目文件生成或 UE 人工配置的代码问题。
