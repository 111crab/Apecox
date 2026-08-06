# 外部参考索引

生成日期：2026-07-14

本目录保存可供 Apex 后续研究使用的外部参考索引。外部文章、学习包、Lyra 和第三方实现不是默认阅读材料，只有具体问题需要时才读取。

## 已迁移到 Apex 的参考

| 文件 | 说明 |
| --- | --- |
| `UE_GAS_SkillGraph_Design_Document.md` | Aura 阶段已有的 GAS SkillGraph 参考方案，不能照搬。 |
| `UE_GAS_SkillGraph_Design_Review.md` | Codex 对上述参考方案的评审。 |
| `Lyra AbilitySystem 深度解析：GAS的正确打开方式 - 知乎.mhtml` | Lyra GAS 代码职责的二次分析；关键结论需与本地 Lyra 源码交叉核对。 |
| `(7 封私信 _ 68 条消息) UF2025(Shanghai)——《漫威争锋》战斗系统架构实战：从GAS框架到客户端预测的完整落地方案 - 知乎.mhtml` | 漫威争锋技术分享的文章整理；用于借鉴配置分层、模板/Task、Timeline、Combat Entity 和编辑器思想，不复制 AAA 规模。 |

## Aura 原始外部文章

这些 mhtml 已完整复制到 `Agent/Aura_Legacy_Archive/` 根目录，同时可从旧 Aura 路径读取：

| 来源文件 |
| --- |
| `D:\UnrealProject\Aura\Agent\(7 封私信 _ 68 条消息) 【游戏设计】如何搭建技能实现框架，让你少走弯路【干货】 - 知乎.mhtml` |
| `D:\UnrealProject\Aura\Agent\(7 封私信 _ 68 条消息) 如何设计一个易扩展的游戏技能系统？ - 知乎.mhtml` |
| `D:\UnrealProject\Aura\Agent\(7 封私信 _ 68 条消息) UF2025(Shanghai)——《漫威争锋》战斗系统架构实战：从GAS框架到客户端预测的完整落地方案 - 知乎.mhtml` |

## 使用规则

- 读取外部参考前先说明目的。
- 读取后要写“借鉴什么 / 不借鉴什么”。
- 不因为参考项目复杂就照搬完整框架。
- Apex 的当前决策以 `Agent/00_Coordination/Decision_Log.md` 和用户最新确认优先。
