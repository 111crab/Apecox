# Apecox Unreal Engine Project Context

更新日期：2026-08-06

## 项目事实

- 项目路径：`D:/UnrealProject/Apecox`
- 项目文件：`Apecox.uproject`
- Unreal Engine：5.8，安装路径 `E:/UE_5.8`
- 主运行时模块：`Apecox`
- Phase 1A 已建立最小 Gameplay Framework、PlayerState ASC 所有权、Pawn Avatar Init/Uninit 和 `Health/MaxHealth` Vital Attribute；代码审查及完整 `ApecoxEditor` 构建已通过，等待 PIE 验证。
- `Apecox.Build.cs` 当前依赖：`Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`GameplayAbilities`、`GameplayTags`、`GameplayTasks`。
- Git 已初始化，当前分支为 `main`，远端 `origin` 指向 `https://github.com/111crab/Apecox.git`；Phase 0 基线提交 `e115f8f` 已推送。
- 项目自建 `.uasset/.umap/.ubulk/.uexp` 使用 Git LFS；第三方商业资产默认不提交。
- 当前默认地图是 `/Game/Blueprints/Maps/L_Apecox_DevGym`；编辑器中的 `Content` 对应资源挂载点 `/Game`。
- Epic Experimental、EditorOnly 的 `GASToolsets` 已关闭；官方 `GameplayAbilities` 已在 `.uproject` 中显式启用。
- 玩家 ASC 由 `AApecoxPlayerState` 持有并使用 Mixed 复制，`AApecoxPlayerCharacter` 作为当前 Avatar；PlayerState 网络更新频率为 100 Hz。

## 本机参考来源

- 旧 Aura 项目：`D:/UnrealProject/Aura`
- 旧 Apex 项目：`D:/UnrealProject/Apex`
- Lyra Starter Game 源码：`D:/UnrealProject/LyraStarterGame`
- 待迁移的武器、动画和课程资产：`D:/UE_Resource`

## 当前渲染事实

- 默认 RHI 为 DX12，目标 Shader Model 为 SM6。
- Lumen、Virtual Shadow Maps 与 Mesh Distance Fields 已开启。
- Hardware Ray Tracing 和 Substrate 已关闭；Lumen 使用软件追踪并保留 Mesh Distance Fields。
- 用户曾遇到 UE DX12 显存压力和驱动异常；回退显卡驱动后恢复稳定，`-d3d11` 仅作为故障排查手段。

## 产品目标

- 面向 UE 游戏客户端求职作品，优先展示工程质量而不是内容数量。
- 目标方向是小规模多人英雄射击垂直切片，不以完整大逃杀或商业内容量为目标。
- 重点包括 GAS 深度、武器和技能共存、服务器权威、客户端预测、复制、可扩展配置、第一/第三人称表现分层和可复现验证。
- Lyra 是定向学习来源，不直接复制完整 Experience、GameFeature 或 ModularGameplay 体系。

## 已继承的架构原则

- 不以一个万能 GA 或一个万能配置资产覆盖所有技能为目标。
- 使用 GA 基类、少量稳定流程模板、AbilityTask、受控虚函数钩子和必要的专用 GA 形成扩展阶梯。
- AbilitySet 是可撤销授予包，不是单技能完整定义。
- SkillDefinition 是技能组装入口，不是“所有逻辑都字段化”。
- 施法目标、战斗衍生物生成和衍生物检测是不同职责。
- AttributeSet、GameplayEffect、GameplayCue、GameplayEvent、AbilityTask 各自遵守 GAS 原生边界。
- 稳定 C++ GameplayTag 使用 Native Gameplay Tags 命名空间，不创建项目 Tag 单例。
- 玩家 ASC 倾向由 PlayerState 拥有、Pawn 作为 Avatar；必须有对称初始化和解绑流程。
- 多人技能从第一批原型开始进行 Listen Server + Client 验证。

## 协作约定

- Codex 负责高层规划、架构分析、Prompt、代码审查、验证方案和 Git 收口。
- ClaudeCode/子代理负责经批准范围内的具体代码实施并提交中文报告。
- 用户审阅关键类名、成员变量、函数、GameplayTag、配置资产和 UE 手工配置。
- 非批量 UE 编辑器操作默认由用户手工完成；MCP 不作为单资产操作的默认手段。
- 用户当前自行处理 Rider 项目文件刷新，不要求 Codex 自动重新生成解决方案。
- 用户可见、需要审核的文档尽量使用中文。
- 新增业务 C++ 使用模块级 `Public/Private` 结构：头文件进入 `Public`、实现文件进入 `Private`，两边采用一致的领域目录层级和命名。

## 开始任务前的最小阅读

- `Agent/README.md`
- `Agent/00_Coordination/Current_Phase.md`
- 当前任务确实需要时，再读 `Current_Code_Design.md` 或相关 RFC。
