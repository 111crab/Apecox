# Apecox 技能编辑器长期愿景

更新日期：2026-08-05
状态：长期方向，不进入当前里程碑。

## 定位

技能编辑器是技能开发入口和预览工具，界面可类比 Montage 编辑器：

- 左侧管理 SkillDefinition、Cue、CombatEntity 等关联资源。
- Details 面板编辑公共配置和当前类型化 ExecutionConfig。
- 中央视口使用默认或指定角色预览动画、生成点、投射物、范围和表现。
- Timeline 显示 Montage、GameplayEvent、Cue 与预览片段。
- 保存后形成可持久化配置资产，运行时直接读取，不生成每技能 C++ 文件。

## 能做什么

- 校验 AbilityClass 与 ExecutionConfig 是否匹配。
- 播放指定角色的 Montage。
- 可视化 Socket、目标方向、Trace、范围和 CombatEntity 生成。
- 预览 GameplayCue、Niagara、音效和部分镜头反馈。
- 检查缺失资产、非法参数和关键事件 Tag。
- 统一跳转并减少在多个编辑器之间寻找资产。

## 不能替代什么

- 服务器权威和客户端预测。
- Listen Server/Dedicated Server 复制验证。
- 完整 GameMode、PlayerState、装备、地图碰撞和敌我状态。
- 真正网络延迟、丢包、回滚和多人交互。
- 对所有特殊 GA 的任意逻辑模拟。

## 一致性原则

编辑器不复制一套“预览版技能逻辑”。预览应尽量调用运行时共享的：

- Definition/Config 解析。
- 目标与生成规则的纯数据部分。
- CombatEntity Preview Adapter。
- Cue 与 Presentation 资源。
- 动画和 Timeline 数据。

需要 World/ASC/Authority 的逻辑必须通过预览上下文适配，并明确标注“近似预览”。

## 启动条件

至少满足以下条件后再开始：

- 已有 2-3 个生命周期不同的技能或武器 Ability。
- SkillDefinition、AbilitySet、CombatEntity 和 Presentation 边界稳定。
- Montage GameplayEvent、Cue 和网络路径已在游戏中验证。
- 已经出现真实的内容制作重复成本。

第一阶段编辑器只做资产校验和局部预览，不立即实现任意节点图或完整技能 VM。
