# 当前 UE 人工操作：第三人称准星、朝向与四向移动

更新时间：2026-07-30

## 一、本轮目标

完成以下 UE 资产配置并验证：

1. 本地玩家屏幕中心显示静态准星。
2. 角色水平朝向持续跟随鼠标控制的视角。
3. W/A/S/D 分别播放 Phase 的前进、后退、左移、右移动画。
4. Energy Bolt 从手部 Socket 朝准星指向的位置飞行。
5. Listen Server 与 Client 都能正确显示对方的朝向、Montage 和投射物。

本轮不修改 GameplayTag、GE、GA、技能数值或 Montage Notify。

## 二、启动前

1. 使用 Rider 重新加载项目文件或等待索引完成。
2. 启动 UE 编辑器。
3. 如果编辑器提示旧类 `ApexAbilityTask_WaitAimTargetData` 丢失，不要创建替代类；先关闭编辑器并确认最新 C++ 已编译。
4. 打开 Output Log，确认没有 UHT、类加载或 Blueprint 编译错误。

## 三、切换为 Apex 自有框架蓝图

当前项目默认使用：

```text
/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode
/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController
```

它们虽然已经继承 Apex 的 C++ 类，但仍是官方 ThirdPerson 示例目录中的资产。现在创建项目自有蓝图，后续不再把示例蓝图作为正式入口。

### 3.1 创建 BP_ApexPlayerController

创建目录：

```text
/Game/Blueprints/GameFramework
```

右键选择 `Blueprint Class > All Classes`，搜索并选择 C++ 类：

```text
ApexPlayerController
```

命名：

```text
BP_ApexPlayerController
```

打开 `Class Defaults`，设置：

```text
Default Mapping Contexts
  [0] = /Game/Blueprints/Input/IMC_Apex_BaseMove
```

`Gameplay HUD Widget Class` 暂时保持 `None`，等第五节创建 HUD 后再回来配置。

Compile 并 Save。

### 3.2 创建 BP_ApexGameMode

再次选择 `Blueprint Class > All Classes`，搜索并选择：

```text
ApexGameMode
```

命名：

```text
BP_ApexGameMode
```

打开 `Class Defaults`，设置：

| 属性 | 值 |
|---|---|
| `Default Pawn Class` | `/Game/Blueprints/Characters/Phase/BP_Hero_Phase` |
| `Player Controller Class` | `BP_ApexPlayerController` |
| `Player State Class` | 保持继承的 `ApexPlayerState` |

其他 GameState、HUD 和 Spectator 配置暂时保持默认值。

Compile 并 Save。

### 3.3 切换项目入口

打开：

```text
Edit > Project Settings > Project > Maps & Modes
```

设置：

```text
Default GameMode = BP_ApexGameMode
```

再打开当前验证地图的：

```text
Window > World Settings
```

检查 `GameMode Override`：

- 如果当前引用 `BP_ThirdPersonGameMode`，清除为 `None`，让地图继承项目默认 GameMode。
- 如果该地图确实需要显式模式，也可以直接设置为 `BP_ApexGameMode`。

旧的 `BP_ThirdPersonGameMode` 和 `BP_ThirdPersonPlayerController` 暂时不要删除；确认新入口验证通过后，再决定是否清理示例资产。

### 3.4 理解资产引用关系

GameMode、PlayerController 和 BlendSpace 不是同一层资产。当前正确关系是：

```text
BP_ApexGameMode
-> BP_ApexPlayerController
-> BP_Hero_Phase
-> Mesh.AnimClass = ABP_Apex_Phase
-> Grounded 状态使用 BS_Apex_Phase_Locomotion
```

因此 GameMode 或 PlayerController 中不会出现 `BS_Apex_Phase_Locomotion` 引用。

## 四、确认玩家角色朝向配置

打开：

```text
/Game/Blueprints/Characters/Phase/BP_Hero_Phase
```

在 `Class Defaults` 和 `Character Movement` 中确认：

| 属性 | 值 |
|---|---|
| `Use Controller Rotation Yaw` | `true` |
| `Use Controller Rotation Pitch` | `false` |
| `Use Controller Rotation Roll` | `false` |
| `Orient Rotation to Movement` | `false` |
| `Use Controller Desired Rotation` | `false` |

如果某个属性右侧出现黄色“恢复默认值”箭头，点击箭头，使蓝图重新继承 C++ 默认值。

Compile 并 Save。

## 五、创建中心准星

### 5.1 创建 WBP_Apex_Crosshair

在以下目录创建 `Widget Blueprint`：

```text
/Game/Blueprints/UI/HUD/WBP_Apex_Crosshair
```

在 Designer 中：

1. 保留一个 `Canvas Panel` 作为根节点。
2. 添加 5 个 `Border`，分别命名：

```text
CenterDot
TopLine
BottomLine
LeftLine
RightLine
```

3. 所有 Border 使用白色或浅灰色，透明度建议 `0.9`。
4. 将它们按一个 `32 x 32` 的局部区域摆放：

| 控件 | Position X/Y | Size X/Y |
|---|---:|---:|
| `CenterDot` | `15, 15` | `2, 2` |
| `TopLine` | `15, 4` | `2, 7` |
| `BottomLine` | `15, 21` | `2, 7` |
| `LeftLine` | `4, 15` | `7, 2` |
| `RightLine` | `21, 15` | `7, 2` |

5. 根节点的 `Visibility` 设为 `Hit Test Invisible`，避免准星拦截游戏输入。
6. Compile 并 Save。

### 5.2 创建 WBP_Apex_GameplayHUD

在同一目录创建：

```text
/Game/Blueprints/UI/HUD/WBP_Apex_GameplayHUD
```

配置：

1. 使用 `Canvas Panel` 作为全屏根节点。
2. 从 Palette 的 `User Created` 分类拖入 `WBP_Apex_Crosshair`。
3. 选中 Crosshair 的 Canvas Slot：

| 属性 | 值 |
|---|---|
| `Anchors` | 正中心 |
| `Alignment` | `0.5, 0.5` |
| `Position` | `0, 0` |
| `Size` | `32, 32` |
| `ZOrder` | `10` |

4. 根节点和 Crosshair 都设为 `Hit Test Invisible`。
5. Compile 并 Save。

### 5.3 配置 PlayerController

打开：

```text
/Game/Blueprints/GameFramework/BP_ApexPlayerController
```

在 `Class Defaults > UI > HUD` 中设置：

```text
Gameplay HUD Widget Class = WBP_Apex_GameplayHUD
```

Compile 并 Save。

如果找不到该属性，先确认蓝图父类是 `AApexPlayerController`，并重新编译 C++ 后重启编辑器。

## 六、配置四向 BlendSpace

打开：

```text
/Game/Blueprints/Characters/Phase/Animation/BS_Apex_Phase_Locomotion
```

### 6.1 Axis Settings

| 轴 | Name | Minimum | Maximum | Grid Divisions | Wrap Input |
|---|---|---:|---:|---:|---|
| Horizontal | `MovementDirection` | `-180` | `180` | `4` | `true` |
| Vertical | `GroundSpeed` | `0` | `500` | `1` | `false` |

### 6.2 Sample Graph

在速度 `0` 的底部一行放置 `Idle`：

```text
(-180, 0) Idle
(-90,  0) Idle
(0,    0) Idle
(90,   0) Idle
(180,  0) Idle
```

在速度 `500` 的顶部一行放置：

```text
(-180, 500) Jog_Bwd
(-90,  500) Jog_Left
(0,    500) Jog_Fwd
(90,   500) Jog_Right
(180,  500) Jog_Bwd
```

`-180` 和 `180` 两端都放 `Jog_Bwd`，用于消除方向角跨越边界时的动画跳变。

在 BlendSpace 预览中拖动绿色预览点，分别检查前、后、左、右动画是否正确，再 Save。

## 七、连接 AnimBP

打开：

```text
/Game/Blueprints/Characters/Phase/Animation/ABP_Apex_Phase
```

进入 Locomotion 状态机中的 `Grounded` 状态：

1. 选中现有的 `BS_Apex_Phase_Locomotion` BlendSpace Player。
2. 将继承变量 `GroundSpeed` 连接到同名输入。
3. 将继承变量 `MovementDirection` 连接到同名输入。
4. 如果节点没有显示两个输入，右键节点选择 `Refresh Nodes`；仍不显示则删除并重新添加该 BlendSpace Player。
5. Compile 并 Save。

不要修改 Falling、Jump、Land 或 Montage Slot 链路。

## 八、单人验证

### 8.1 HUD 与朝向

1. 使用单人 PIE。
2. 准星应始终位于游戏窗口中心，调整窗口大小后也不能漂移。
3. 原地水平移动鼠标，角色应跟随镜头的水平 Yaw 转向。
4. 上下移动鼠标只改变视角 Pitch，角色胶囊和身体不能向前后倾倒。

当前没有 Turn In Place，因此原地快速转动镜头时脚步可能滑动；这不是本阶段失败条件。

### 8.2 四向移动

保持镜头方向不变，逐项检查：

| 输入 | 期望动画 |
|---|---|
| `W` | `Jog_Fwd` |
| `S` | `Jog_Bwd` |
| `A` | `Jog_Left` |
| `D` | `Jog_Right` |

如果 A/D 反了，先检查 BlendSpace 中 `-90 = Left`、`90 = Right`，不要修改 C++ 计算公式。

### 8.3 Energy Bolt

1. 将准星对准远处墙面，按 Q。
2. 角色应面向准星的水平方向播放 Montage。
3. 投射物仍从手部 Socket 生成。
4. 投射物应朝准星所指的墙面飞行，并在命中位置触发 Impact Cue。
5. 再对近距离目标测试一次。

第三人称过肩相机与手部 Socket 不在同一点，因此投射物刚生成时不会始终与准星重合；正确标准是它从手部飞向准星射线的命中点。

## 九、多人验证

1. `Number of Players = 2`。
2. Net Mode 使用 `Play As Listen Server`。
3. Host 和 Client 的各自窗口都只能出现一个本地中心准星。
4. Host 转动视角和四向移动，Client 应看到 Host 的正确身体朝向和移动动画。
5. Client 转动视角和四向移动，Host 应看到 Client 的正确身体朝向和移动动画。
6. Host 与 Client 分别释放 Energy Bolt，对方应看到正确的 Montage、发射朝向、投射物和 Impact Cue。
7. 命中伤害、Mana 消耗和 Cooldown 仍按上一阶段标准工作，不能重复结算。

## 十、最终通过标准

- 中心准星在单人和两个本地玩家窗口中均正确显示。
- 角色水平朝向跟随鼠标视角，Pitch 不作用于角色身体。
- W/A/S/D 四向动画全部对应正确。
- Energy Bolt 从手部朝准星目标飞行。
- Host 与 Client 能正确观察彼此的朝向、动画、投射物和命中。
- 没有 Blueprint 编译错误、TargetData 错误或新增网络警告。

验证完成后，将结果直接回复给 Codex；通过后进行本阶段功能提交与 push。
