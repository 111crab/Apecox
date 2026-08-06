# Lyra 阅读计划

生成日期：2026-07-15

## 阅读目标

阅读 Lyra 的目的不是照搬，而是理解成熟 UE / GAS 项目的组织方式，再设计适合本项目规模的轻量版本。

## 当前源码

- 本机根目录：`D:\UnrealProject\LyraStarterGame`
- 2026-07-15 已确认目录存在并可读取。

## 当前启动条件

- Codex 先确认 Lyra 版本、模块结构与源码完整性。
- 第一轮只读分析，不修改 Apex C++、蓝图或配置。

## 当前状态

- 第一轮定向源码研究已完成。
- 研究报告：`Agent/Research_Notes/2026-07-15_Lyra_GAS_Architecture_Study.md`。
- 当前等待用户审阅“采用 / 轻量化 / 暂不采用”结论，再进入 Apex GAS RFC。

## 第一轮核心问题

1. Lyra 为什么把玩家 ASC 放在 PlayerState，Apex 是否也需要这样做？
2. Pawn、PlayerState 与 ASC 如何完成初始化、重生重绑和解绑？
3. AbilitySet 如何批量授予 Ability、GameplayEffect 和 AttributeSet？
4. InputAction 如何经过 InputTag 路由到 ASC？
5. GameplayAbility 的 ActivationPolicy、ActivationGroup 与取消关系解决什么问题？
6. HealthSet、CombatSet 与 HealthComponent 如何划分属性、伤害和死亡职责？
7. GameplayTag 如何声明、注册和用于输入、状态、阻塞及技能关系？
8. 哪些设计依赖 Experience / GameFeature，哪些可以提炼为 Apex 的轻量运行时？

## 输出要求

阅读笔记不要写成源码摘抄。

每个主题记录：

- Lyra 的做法。
- 为什么这样做。
- 对我们是否有价值。
- 如果采用，轻量版如何实现。
- 如果不采用，原因是什么。

## 第一批源码搜索入口

- `ULyraAbilitySystemComponent`
- `ULyraGameplayAbility`
- `ULyraAbilitySet`
- `ALyraPlayerState`
- `ULyraPawnExtensionComponent`
- `ULyraPawnData`
- `ULyraHeroComponent`
- `ULyraInputConfig` / `ULyraInputComponent`
- `ULyraHealthSet` / `ULyraCombatSet` / `ULyraHealthComponent`
- Lyra 原生 GameplayTag 声明与注册入口

实际文件路径以用户提供的 Lyra 版本为准，不预先假定目录。

## 第一轮明确不展开

- 不完整复刻 Experience 加载链。
- 不完整复刻 GameFeature 插件化。
- 不研究与当前 GAS 基础无关的 UI、武器表现和在线服务。
- 不因 Lyra 使用某种模式就默认 Apex 必须采用。

## 第一轮交付物

生成一份中文研究报告，按下面的固定口径组织：

```text
Lyra 怎么做
为什么这样做
它解决了什么规模或生命周期问题
Apex 是否需要
Apex 采用完整版、轻量版，还是拒绝
对首批 GAS 类、成员变量、函数与 Tag 命名有什么影响
```

研究报告经用户确认后，才进入 Apex GAS RFC 和 C++ 类设计。
