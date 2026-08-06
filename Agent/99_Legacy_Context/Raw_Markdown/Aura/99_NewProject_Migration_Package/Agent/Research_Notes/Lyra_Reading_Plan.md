# Lyra 阅读计划

生成日期：2026-07-14

## 阅读目标

阅读 Lyra 的目的不是照搬，而是理解成熟 UE / GAS 项目的组织方式，再设计适合本项目规模的轻量版本。

## 优先问题

1. Lyra 如何初始化 ASC？
2. AbilitySet 如何授予 Ability、Effect、Attribute？
3. 输入如何映射到 Ability？
4. GameplayTag 如何声明、注册和分类？
5. PawnData、Experience、GameFeature 的职责边界是什么？
6. GameplayCue 如何组织？
7. AbilityTask 在技能流程中扮演什么角色？
8. 多人预测和表现同步在哪里处理？

## 输出要求

阅读笔记不要写成源码摘抄。

每个主题记录：

- Lyra 的做法。
- 为什么这样做。
- 对我们是否有价值。
- 如果采用，轻量版如何实现。
- 如果不采用，原因是什么。

## 第一批阅读主题

- 模块结构。
- GameFeature / Experience。
- PawnData / HeroData。
- AbilitySet。
- Enhanced Input 到 Ability 的路径。
- GameplayTags。
- GameplayCue。
- AttributeSet 和初始化。

