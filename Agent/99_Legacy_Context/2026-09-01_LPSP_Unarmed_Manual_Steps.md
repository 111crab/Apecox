# 历史 UE 人工操作：LPSP 空手重定向

更新日期：2026-09-01  
适用版本：Unreal Engine 5.8  
当前目标：接入第一人称空手 `Idle / Move / Run`，并为现有 RAR 持枪 Locomotion 增加 `Run`。

## 使用规则

- 严格按章节顺序操作。每一章结尾都先 `Save All`，再进入下一章。
- 资产选择器出现同名项时，必须根据本清单给出的完整目录判断，不能只看资产名。
- 本轮只处理 5 个空手动画和 1 个 RAR Sprint，不选择 Crouch、Jump、Tactical Sprint 或供应商 AnimBP。
- Retarget 阶段出现红色 `pelvis near ground plane` 警告时，先按第四章说明处理；只要同时显示 `Success! The IK Retargeter is ready`，它就不是阻塞错误。
- 任意导出动画的 Skeleton 不是 `SKEL_IG_Mannequin` 时立即停止，不继续搭建 AnimGraph。

## 关键机制

- LPSP 空手动画使用 `SKEL_Character`，不能直接播放在当前 `SKEL_IG_Mannequin` 上。
- LPSP 提供源 `IK_LPSP` 和 `RTG_LPSP_UE5`；RAR 提供当前 IG Manny 对应的目标 `IK_Mannequin`。只需要替换 Retargeter 的 Target IK Rig，不手工创建 Retarget Chain。
- 空手移动动画是 Additive，必须以重定向后的空手 `Idle_Pose` 为 Base Pose；这个关系错误会导致手臂消失。
- 当前 CMC 只有默认 600 速度，没有 Shift Sprint。本轮 `Run` 是速度达到阈值后的第一人称视觉层，不改变移动速度和网络逻辑。

## 一、从 LPSP 迁移限定资源

1. 关闭 Apecox 编辑器。
2. 使用 UE 5.8 打开：

```text
D:\UE_Resource\Low Poly Shooter Pack V6.0\Low Poly Shooter Pack v6.0 5.7\Extracted\LowPolyShooterPackv6\LowPolyShooterPackv6.uproject
```

如果提示项目来自 UE 5.7，选择 `Open a Copy`。

3. 在 Content Browser 打开：

```text
/Game/InfimaGames/AnimatedLowPolyWeapons/Art/Characters/Animations/Unarmed
```

4. 只选择以下 5 个资产：

```text
A_FP_PCH_Unarmed_Idle_Pose
A_FP_PCH_Unarmed_Idle_Loop
A_FP_PCH_Unarmed_Walk_F
A_FP_PCH_Unarmed_Walk_B
A_FP_PCH_Unarmed_Run
```

5. 右键选择 `Asset Actions -> Migrate`，目标选择：

```text
D:\UnrealProject\Apecox\Content
```

依赖列表自动带入 `SKEL_Character` 等源骨架资产，这是正常的。

6. 再定位并迁移：

```text
/Game/InfimaGames/LowPolyShooterPack/Art/Characters/Default/RTG_LPSP_UE5
```

目标仍然选择 `D:\UnrealProject\Apecox\Content`。迁移会自动带入 `IK_LPSP` 和它的预览 Mesh。

7. 关闭 LPSP 工程。

## 二、从 RAR 迁移目标 IK Rig 与持枪 Run

1. 使用 UE 5.8 打开：

```text
D:\UE_Resource\Realistic Assault Rifle Template\UE5\RAR 5.8\RAR.uproject
```

2. 分别迁移以下两个资产到 `D:\UnrealProject\Apecox\Content`：

```text
/Game/InfimaGames/ArtCore/Mannequin/Rigs/IK_Mannequin

/Game/InfimaGames/RealisticAssaultRifle/Art/Character/Animations/Movement/A_RAR_FP_PCH_AssaultRifle_Sprint_F_Loop
```

如果出现已有 RAR 依赖是否覆盖的提示，勾选 `Apply to All`，选择 `No`。本轮只补入缺少的 IK Rig 和 Sprint 动画，不覆盖当前已经验证的 RAR 资产。

3. 关闭 RAR 工程，重新打开 Apecox。

## 三、创建并检查项目自己的 Retargeter

1. 创建目录：

```text
/Game/Blueprints/Characters/Animations/FirstPerson/Unarmed
```

2. 在 Content Browser 搜索 `RTG_LPSP_UE5`。
3. 右键该资产选择 `Duplicate`，把副本移动到上述目录，重命名为：

```text
RTG_Apecox_LPSP_To_IGMannequin
```

4. 双击打开 `RTG_Apecox_LPSP_To_IGMannequin`。
5. 在 Retargeter 编辑器右侧 Details 设置：

| 字段 | 值 |
| --- | --- |
| Source IK Rig | `IK_LPSP`，路径位于 `LowPolyShooterPack/Art/Characters/Default` |
| Target IK Rig | `IK_Mannequin`，路径位于 `ArtCore/Mannequin/Rigs` |

不要选择 LPSP `Utilities/.../IK_Mannequin`；目标必须是 RAR `ArtCore/.../IK_Mannequin`。

6. `Source IK Rig` 必须是 LPSP 路径：

```text
/Game/InfimaGames/LowPolyShooterPack/Art/Characters/Default/IK_LPSP
```

7. `Target IK Rig` 必须是 RAR ArtCore 路径：

```text
/Game/InfimaGames/ArtCore/Mannequin/Rigs/IK_Mannequin
```

8. 点击 `Auto Map Chains`，选择按名称精确匹配。
9. 在 Chain Mapping 列表确认至少以下链不是 `None`：

```text
Spine
LeftClavicle
LeftArm
RightClavicle
RightArm
LeftThumb / LeftIndex / LeftMiddle / LeftRing / LeftPinky
RightThumb / RightIndex / RightMiddle / RightRing / RightPinky
```

10. 不进入 Edit Retarget Pose，不修改骨骼旋转。
11. 点击 `Save`，关闭 Retargeter 编辑器。

## 四、使用 UE 5.8 的 Retarget Animations 导出 5 个动画

### 4.1 打开 UE 5.8 Retarget 窗口

1. 在 Content Browser 进入 LPSP 空手动画目录：

```text
/Game/InfimaGames/AnimatedLowPolyWeapons/Art/Characters/Animations/Unarmed
```

2. 在搜索框输入：

```text
A_FP_PCH_Unarmed_
```

3. 按住 `Ctrl`，只选择：

```text
A_FP_PCH_Unarmed_Idle_Pose
A_FP_PCH_Unarmed_Idle_Loop
A_FP_PCH_Unarmed_Walk_F
A_FP_PCH_Unarmed_Walk_B
A_FP_PCH_Unarmed_Run
```

4. 确认 Content Browser 左下角显示 `5 items (5 selected)`。
5. 右键任意已选动画，点击 UE 5.8 菜单：

```text
Retarget Animations
```

这里没有旧版 `Export Selected Animations` 菜单。

### 4.2 配置 Retarget Animations 窗口

在新窗口右上区域逐项设置：

1. `Source Skeletal Mesh` 不要使用自动带出的 `SK_Mannequin_Arms`，改为 LPSP 的 IK Rig 标准预览 Mesh：

```text
SK_FP_CH_Default
```

完整路径：

```text
/Game/InfimaGames/LowPolyShooterPack/Art/Characters/Default/SK_FP_CH_Default
```

2. `Target Skeletal Mesh` 不要选择 Arms-only 的 `SK_IG_FP_Mannequin`，选择 RAR Target IK Rig 使用的完整 Manny：

```text
SKM_Manny
```

完整目标路径应属于：

```text
/Game/InfimaGames/ArtCore/Mannequin/Meshes
```

不要选择 `SK_IG_FP_Mannequin`、皮肤手臂 `SK_IG_FP_Arms`，也不要选择 LPSP 自己的 Manny。Retarget 求解使用完整标准 Mesh；导出的动画仍然属于 `SKEL_IG_Mannequin`，运行时可以正常播放在 `SK_IG_FP_Mannequin` 上。

3. 取消勾选：

```text
Auto Generate Retargeter
```

4. `Retarget Asset` 选择：

```text
RTG_Apecox_LPSP_To_IGMannequin
```

5. `Override Set to Apply` 保持 `None`。

### 4.3 判断当前状态是否可以导出

左侧应能看到 Source 与完整 Target Manny。两者可以部分重叠，但目标不应出现手臂被拉成长尖锥的现象。

底部可能同时出现：

```text
The source pelvis bone is very near the ground plane...
Success! The IK Retargeter is ready to transfer animation...
```

使用 `SK_FP_CH_Default -> ArtCore/SKM_Manny` 后，不应再依靠忽略 Pelvis 警告继续导出。若仍显示 `pelvis near ground plane`，或者预览出现长尖锥拉伸，不要导出；重新核对 Source/Target Mesh 路径。只有预览骨架比例正常且显示 `Success!` 才能继续。

### 4.4 选择窗口内的动画

1. 在窗口右下方动画列表内单击任意一项。
2. 按 `Ctrl+A`。
3. 确认五行全部蓝色高亮。
4. 底部提示应从 `Select animations to export` 变为可导出状态。
5. 右下角 `Export Animations` 按钮应解除灰色。
6. 点击：

```text
Export Animations
```

不要点击左侧的 `Export Retarget Assets`。

### 4.5 选择输出目录和重命名

输出目录选择：

```text
/Game/Blueprints/Characters/Animations/FirstPerson/Unarmed
```

若导出窗口包含 Search/Replace，填写：

```text
Search:  A_FP_PCH_Unarmed_
Replace: A_Apecox_FP_Unarmed_
Prefix:  留空
Suffix:  留空
```

若 UE 5.8 只显示目录选择器，没有 Search/Replace：

1. 先保持原名导出。
2. 回到输出目录，选中五个新动画。
3. 右键选择 `Batch Rename`。
4. 使用相同的 Search/Replace 完成批量重命名。

最终必须得到：

```text
A_Apecox_FP_Unarmed_Idle_Pose
A_Apecox_FP_Unarmed_Idle_Loop
A_Apecox_FP_Unarmed_Walk_F
A_Apecox_FP_Unarmed_Walk_B
A_Apecox_FP_Unarmed_Run
```

### 4.6 验证目标 Skeleton

1. 双击打开 `A_Apecox_FP_Unarmed_Idle_Pose`。
2. 在左侧 `Asset Details` 搜索 `Skeleton`。
3. Skeleton 必须显示：

```text
SKEL_IG_Mannequin
```

4. 点击 Skeleton 字段旁的放大镜，Content Browser 应定位到：

```text
/Game/InfimaGames/ArtCore/Mannequin/Meshes/SKEL_IG_Mannequin
```

5. 依次打开另外四个动画，确认 Skeleton 相同。
6. 在动画编辑器顶部 `Preview Mesh` 选择 `SK_IG_FP_Mannequin`，确认动画播放在最终 FP Arms 上没有尖锥拉伸。
7. 任意一个 Skeleton 不相同或预览仍拉伸时，停止后续配置。

## 五、使用 UE 5.8 Asset Details 修正 Additive Base

### 5.1 Idle Pose

1. 打开 `A_Apecox_FP_Unarmed_Idle_Pose`。
2. 在左侧 `Asset Details` 搜索 `Additive`。
3. 设置：

```text
Additive Anim Type = No Additive
```

4. `Save` 后关闭。

### 5.2 四个 Additive 动画

对 `Idle_Loop / Walk_F / Walk_B / Run` 逐个执行：

1. 打开动画。
2. 在 `Asset Details` 搜索 `Additive`。
3. 设置：

```text
Additive Anim Type = Local Space
Base Pose Type     = Selected Animation Frame
Base Pose Animation / Ref Pose Sequence
                   = A_Apecox_FP_Unarmed_Idle_Pose
Ref Frame Index    = 0
```

4 个动画是：

```text
Idle_Loop
Walk_F
Walk_B
Run
```

4. `Base Pose Animation` 在部分 UE 5.8 布局中显示为 `Ref Pose Sequence`，两者指同一字段。
5. 设置后拖动时间轴，预览里应能看到相对 Idle Pose 的小幅位移，而不是整套手臂消失或飞离视口。
6. 每个都点击 `Save`。

## 六、创建空手移动 Blend Space

1. 回到目录：

```text
/Game/Blueprints/Characters/Animations/FirstPerson/Unarmed
```

2. 点击 Content Browser 左上角 `Add`。
3. 选择 `Animation -> Blend Space`。
4. 在 Skeleton 选择器搜索并选择：

```text
SKEL_IG_Mannequin
```

5. 命名：

```text
BS_Apecox_FP_Unarmed_Move
```

6. 双击打开 Blend Space。
7. 在右侧 Asset Details 展开 `Axis Settings`，设置：

| Axis | Min | Max |
| --- | ---: | ---: |
| Horizontal | -1.0 | 1.0 |
| Vertical | -1.0 | 1.0 |

8. 在右侧 Asset Browser 搜索相应动画，把动画拖到网格准确坐标：

| 坐标 | 动画 |
| --- | --- |
| `(0, 0)` | `A_Apecox_FP_Unarmed_Idle_Loop` |
| `(0, 1)` | `A_Apecox_FP_Unarmed_Walk_F` |
| `(0, -1)` | `A_Apecox_FP_Unarmed_Walk_B` |
| `(-1, 0)` | `A_Apecox_FP_Unarmed_Walk_F` |
| `(1, 0)` | `A_Apecox_FP_Unarmed_Walk_F` |

左右暂时复用前进摆臂，这是当前资产只有前后空手 Walk 的受控降级，不需要伪造不存在的侧移动画。

9. 逐个点击样本点，在 Sample Details 确认坐标没有被吸附到错误位置。
10. 将鼠标放在 Blend Space 网格中移动，确认预览手臂始终可见。
11. 点击 `Save`。

## 七、新增视觉 Run 条件

打开：

```text
ABP_Apecox_FirstPersonArms -> EventGraph
```

1. 在 My Blueprint 面板点击 Variables 右侧 `+`。
2. 新建 Bool 变量：

```text
bUseForwardRunVisual
```

3. 点击 `Compile`，使变量可用。
4. 打开 `EventGraph`，找到 `Event Blueprint Update Animation` 的现有白色执行线。
5. 在当前最后一个 Set 节点之后新增：

```text
Set bUseForwardRunVisual
```

6. 从 My Blueprint 把以下变量以 `Get` 方式拖入图中：

```text
GroundSpeed
MovementDirection
bIsFalling
bIsCrouching
```

7. 创建并连接以下节点：

```text
GroundSpeed -> >= (Float)，另一端填写 450.0

MovementDirection -> Abs (Float)
Abs Return Value -> <= (Float)，另一端填写 45.0

bIsFalling -> NOT Boolean
bIsCrouching -> NOT Boolean
```

8. 创建 `AND Boolean` 节点，点击 `Add Pin` 直到有四个输入。
9. 把四个条件输出连接到 AND：

```text
GroundSpeed >= 450.0
Abs(MovementDirection) <= 45.0
NOT bIsFalling
NOT bIsCrouching
```

10. 把 AND 输出连接到 `Set bUseForwardRunVisual` 的 Value。
11. 确认白色执行线也连接到该 Set 节点。
12. 点击 `Compile`、`Save`。

这个变量只选择动画，不修改 CMC 速度。

## 八、接入空手 AnimGraph

### 8.1 创建空手状态机

1. 打开 `AnimGraph`。
2. 在空白处右键，搜索 `State Machine`，选择 `Add New State Machine`。
3. 命名：

```text
SM_UnarmedLocomotion
```

4. 双击进入状态机。
5. 从 `Entry` 的白色 Pose 引脚拖出，松开后选择 `Add State`，命名 `Grounded`。
6. 双击 `Grounded` 进入状态内部。

### 8.2 配置 Grounded

1. 从 Asset Browser 拖入：

```text
BS_Apecox_FP_Unarmed_Move
A_Apecox_FP_Unarmed_Run
```

2. 从 My Blueprint 拖入以下变量并选择 `Get`：

```text
RifleMoveHorizontal
RifleMoveVertical
bUseForwardRunVisual
```

这里复用现有两个 `RifleMove` 数值，因为它们实际表示通用的局部移动方向；本轮不重复创建同值变量。

3. 连接 Blend Space 参数：

```text
RifleMoveHorizontal -> BS_Apecox_FP_Unarmed_Move.Horizontal
RifleMoveVertical   -> BS_Apecox_FP_Unarmed_Move.Vertical
```

4. 右键创建 `Blend Poses by Bool`，连接：

```text
False Pose = BS_Apecox_FP_Unarmed_Move
True Pose  = A_Apecox_FP_Unarmed_Run
Active Value = bUseForwardRunVisual
True Blend Time  = 0.12
False Blend Time = 0.12
```

5. 把 `Blend Poses by Bool` 输出连接到 `Output Animation Pose`。
6. 点击 `Compile`。预览在空手动画族下应能显示手臂。

### 8.3 接入 Animation Family

1. 回到主 AnimGraph。
2. 从 Asset Browser 拖入：

```text
A_Apecox_FP_Unarmed_Idle_Pose
Apply Additive
```

3. 右键创建 `Apply Additive`。
4. 连接：

```text
A_Apecox_FP_Unarmed_Idle_Pose -> Apply Additive.Base
SM_UnarmedLocomotion          -> Apply Additive.Additive
Apply Additive                -> Blend Poses by ApecoxCharacterAnimationFamily.空手 Pose
```

5. `Apply Additive.Alpha = 1.0`。
6. 确认连接的是枚举节点的 `空手 Pose`，不是 `Default Pose` 或 `步枪 Pose`。
7. 点击 `Compile`、`Save`。

## 九、给现有持枪 Grounded 增加 Run

打开：

```text
SM_RifleLocomotion -> Grounded
```

1. 双击进入 `SM_RifleLocomotion`。
2. 双击进入 `Grounded` 状态。
3. 找到当前直接连接 `Output Animation Pose` 的 RAR Movement Blend Space。
4. 按住 `Alt` 单击 Output 连线，断开该连线，不删除 Blend Space 节点。
5. 从 Asset Browser 拖入：

```text
A_RAR_FP_PCH_AssaultRifle_Sprint_F_Loop
```

6. 从 My Blueprint 拖入 `bUseForwardRunVisual`，选择 `Get`。
7. 新增 `Blend Poses by Bool`：

```text
False Pose = 当前 BS_RAR_FP_PCH_AssaultRifle_Movement
True Pose  = A_RAR_FP_PCH_AssaultRifle_Sprint_F_Loop
Active Value = bUseForwardRunVisual
True Blend Time  = 0.12
False Blend Time = 0.12
```

8. `Blend Poses by Bool` 输出连接该状态的 `Output Animation Pose`。
9. 点击 `Compile`，确认没有 Skeleton 或 Additive 报错。
10. 点击 `Save`。

主 AnimGraph 末端仍然是：

```text
AnimationFamily
    -> DefaultSlot
    -> Convert Local To Component Space
    -> FABRIK
    -> Convert Component To Local Space
    -> Output Pose
```

## 十、单人验证

1. PIE 使用单人模式启动。
2. 不拾取武器，原地观察两秒：Arms 正常显示，并播放空手 Idle。
3. 分别短按 `W / S / A / D`：出现空手移动摆臂，手臂不消失。
4. 持续按 `W`：加速到稳定速度后切入空手 Run。
5. 持续按 `S / A / D`：不应错误播放向前 Run。
6. 拾取步枪：Animation Family 切换为 Rifle，枪械和双手正常显示。
7. 持枪分别短按 `W / S / A / D`：仍使用原有方向 Movement Blend Space。
8. 持续按 `W`：稳定速度后切入 RAR Sprint。
9. 跳跃并落地：原有 JumpStart、Falling、Land 和左手 FABRIK 正常。
10. 连续开火：Montage、枪口 VFX、声音、扣弹和射击日志正常。

## 十一、Listen Server 验证

1. Host 与 Client 分别在自己的窗口验证空手和持枪的 Idle、移动、Run。
2. Client 拾取、开火、扣弹和服务器 Shot Confirmation 仍正常。
3. 本阶段不验收远端第三人称动画观感。

## 成功标准

- 空手与持枪拥有明显不同的第一人称姿势。
- 两种动画族都能在向前稳定移动时进入对应 Run。
- 切换动画不会导致 Arms 消失、Ref Pose、剧烈弹跳或 Additive 叠加翻倍。
- 持枪跳跃时左手仍受 FABRIK 约束。
- 射击逻辑、Montage、VFX、声音和多人权威链路没有回归。
