# 静止人机、固定一倍镜与命中表现实施报告

日期：2026-09-14

## 结果

- `AApecoxWanderAIController`新增默认关闭的`bEnableWandering`；OnPossess会清理移动和定时器，只有显式开启时才会排入随机MoveTo。机器人继续保留PlayerState、ASC、受伤、死亡和原位置重生。
- `AApecoxHUD`根据实际水平速度和当前最大速度计算移动比例。准星基础间距4像素，满移动比例额外增加10像素，以12的插值速度平滑张开和回收；ADS隐藏规则保持不变。
- 首轮地面准星通过后增加独立腾空项14像素；原地跳跃会张开，向前跳跃会与最多10像素的移动项叠加。
- 新增`UApecoxWeaponImpactGameplayCue`。它读取已有Projectile Cue携带的HitResult，在准确命中点播放可配置Niagara和声音，随机选择贴花并附着到命中组件，6秒后开始用3秒淡出。
- RAR核对确认`Scope_01`无需Render Target；使用相机FOV倍率0.7、第一人称模型FOV 100、Yaw/Pitch灵敏度0.5。具体资产迁移、固定装配和Cue Blueprint配置见当前人工清单。
- 当前项目已有`NS_IG_MuzzleFlash`，内部同时包含火焰与硝烟Emitter。RAR蓝图实际以Yaw 90度附着生成，Apecox首轮使用零旋转；现已新增`MuzzleEffectRotation`并默认匹配RAR的`Pitch 0/Yaw 90/Roll 0`。
- 首轮Impact资产没有序列化`GameplayCue.Weapon.Impact`，所以粒子、声音和弹孔一起不执行。原生父类现不再预填标签，派生`GCN_Weapon_Impact`必须明确选择并保存标签；扫描目录已限定到`/Game/Blueprints/GameplayCues`。

## 验证

- `ApecoxEditor Win64 Development`完整构建成功。
- 修复后全量`Apecox.*`自动化发现并执行62项：62 Success、0 Warning、0 Failed、0 Not Run。既有GameplayCue扫描路径警告已消失。
- 报告：`Saved/Diagnostics/M1D/ScopeImpactFix/Tests/index.json`。
- 日志：`Saved/Diagnostics/M1D/ScopeImpactFix/tests.log`。

## 人工验收结果

- 机器人静止与重生：通过。
- 固定一倍镜ADS及跳跃时红点稳定：通过。
- 地面移动准星与额外腾空扩散：通过。
- `GCN_Weapon_Impact`明确保存`GameplayCue.Weapon.Impact`后，Impact粒子、弹孔和命中声：通过。
- 枪口硝烟：未通过，仍然不可见。

硝烟已排除Data Asset空引用、MuzzlePoint缺失、Niagara材质依赖缺失和RAR生成旋转差异。下一次先检查`NS_IG_MuzzleFlash`中Smoke Emitter在UE 5.8的实际运行状态、Spawn Count/Lifetime、Bounds和Scalability，不把Impact链路重新打开。硝烟通过后，下一批接入RAR式镭射的枪身挂件、开关输入、持续Trace、光束和命中光点。
