# RAR 左右探头实现与 Apecox 适配结论

更新日期：2026-09-17。参考工程：`D:\UE_Resource\Realistic Assault Rifle Template\RAR_5.4\RAR\RAR.uproject`。

## 最终结论

RAR的探头由两条独立运动叠加，并不是单纯平移：

1. `A_RAR_FP_PCH_AssaultRifle_Lean_Poses`以完整权重驱动上半身、手臂和枪械左右倾斜；
2. 相机同时侧移50 cm，并Roll 15°。

此前曾根据组件层级推断Camera会继承`SOCKET_Camera`在Lean Pose中的动画Delta。真实资产集成测量推翻了这个假设：从居中到完全右倾，`SOCKET_Camera`在Component Space中的位置和旋转增量均为0。因此RAR与Apecox的父子层级差异不是本次探头观感的根因，也不需要运行时Socket补偿或重构组件层级。

## RAR实测链路

1. `IA_Lean`为Axis1D。`IMC_Player`中Q使用`Negate`，因此Q=-1；E无Modifier，因此E=+1。
2. 输入值写入`Leaning Target Pressed`，允许探头时转交给`Leaning Target`。
3. `Leaning Alpha = FInterpTo(Current, Target, DeltaSeconds, 9)`。原蓝图注释记录曾尝试Spring Interp，但观感不合适，最终使用FInterpTo。
4. 默认相机侧移为`(X=0,Y=50,Z=0)`；旋转向量为`(X=15,Y=0,Z=0)`。RAR的Vector To Rotator把Vector X映射到Roll，所以这里是15° Roll。
5. 侧移前进行墙体Trace；`Leaning Loc Fix`再用VInterpTo平滑，避免镜头进入墙内。
6. `A_RAR_FP_PCH_AssaultRifle_Lean_Poses`为Mesh Space Additive，基准是`A_RAR_FP_PCH_AssaultRifle_Idle_Pose`，长度0.066667秒。
7. AnimBP把`Leaning Alpha [-1,1]`映射到Explicit Time `[0,0.07]`，通过Sequence Evaluator取Pose，再用`Apply Mesh Space Additive`、Alpha=1叠加。
8. `BS_RAR_FP_PCH_AssaultRifle_Leaning`负责探头状态下的移动方向，不负责左右倾斜本身。

## Apecox核对与最终设置

`ABP_Apecox_FirstPersonArms`原有AnimGraph接线正确：

- 使用`A_RAR_FP_PCH_AssaultRifle_Lean_Poses`；
- `LeanAmount -1~1 → 0~0.066667秒`；
- Mesh Space Additive；
- Additive Alpha=1；
- Q=-1、E=+1。

最终运行设置直接复现RAR的两条运动：

- `MaxLeanDistance=50 cm`；
- `MaxLeanAngle=15°`；
- `LeanInterpSpeed=9`；
- `FirstPersonLeanPoseScale=1.0`，保留完整上半身倾斜；
- 墙体Sphere Sweep按完整50 cm路径限制镜头；
- 第三人称继续读取完整`[-1,1]`探头值。

`14 cm / 7° / Pose 0.55`只缩小原有效果，用户确认观感没有改善，已废弃。`50 cm / 15° / Pose 0`只剩镜头平移与Roll，缺失上半身倾斜，也已废弃。

## 验证证据

- RAR层级与蓝图导出：`Saved/Diagnostics/M1D/RARLean/export/`
- Apecox AnimGraph与角色导出：`Saved/Diagnostics/M1D/RARLean/apecox_current/`
- Camera Socket测量：`Saved/Diagnostics/M1D/RARLean/socket_delta_measurement.json`
- 蓝图实际CDO：`Saved/Diagnostics/M1D/RARLean/relation_fix_defaults.json`
- 最终自动化：`Saved/Diagnostics/M1D/RARLean/FinalCameraTests/index.json`

RAR源资产仅作只读分析，没有保存或改写。最终`ApecoxEditor Win64 Development`完整构建成功；`Apecox.Camera` 12项全部Success，覆盖Look Sway、探头墙体限制、姿态高度、低顶、反向切换、移动和帧率。
