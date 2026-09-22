# 当前 UE 编辑器人工操作（2026-08-24 重构前快照）

更新日期：2026-08-21
当前任务：Phase 2B-2B，步枪双视角腰射表现基础设施。

## 当前状态

代码最终复审与 Development Editor 构建均已通过。现在执行下面步骤；下方“已完成记录”只保留历史，不再重复执行。

## 一、迁移本轮所需 RAR 原始资产

### 1.0 先弄清楚这一步在做什么

你现在不是在 Apecox 里“导入文件”，而是在 **RAR 项目的 UE 编辑器中**，让 UE 把选中的资产及其依赖复制到 Apecox。

```text
操作发生的位置：RAR 编辑器
复制来源：RAR/Content/InfimaGames/...
复制目标：Apecox/Content
RAR 原项目：不会被删除
Apecox：会得到一份可以引用的资产副本
```

UE 把这项功能叫做 `Migrate`。下面先完整演示第一个资产；成功后，其余资产只是重复相同操作。

### 1.1 关闭 Apecox，打开 RAR

1. 如果 Apecox 编辑器正在运行，先保存资产和地图，然后关闭编辑器。
2. 在 Windows 资源管理器中打开：

```text
D:/UE_Resource/Realistic Assault Rifle Template/UE5/RAR
```

3. 双击：

```text
RAR.uproject
```

4. 等待 UE 5.8 完全进入 RAR 项目。
5. 看内容浏览器左侧目录树，应该能看到：

```text
Content
`- InfimaGames
```

如果这里只有 Apecox 的 `Blueprints`、`ThirdParty` 等目录，说明打开错项目了，此时不要继续。

### 1.2 完整迁移第一个资产：SK_IG_FP_Mannequin

1. 在 RAR 编辑器底部点击 `Content Drawer`，展开内容浏览器。
2. 在内容浏览器左侧点击最上层的 `Content`。这样搜索范围是整个 RAR 项目，而不是某个小文件夹。
3. 点击右上方或资产列表上方的搜索框。
4. 输入完整名称：

```text
SK_IG_FP_Mannequin
```

5. 等待搜索结果出现。应该只选择名称完全相同的 `SK_IG_FP_Mannequin`，不要选择 `SKM_Manny`、Skeleton 或 Physics Asset。
6. 为避免选错，把鼠标悬停在资产上，确认 Path 包含：

```text
/Game/InfimaGames/ArtCore/Mannequin/Meshes
```

7. 右键 `SK_IG_FP_Mannequin`。
8. 在右键菜单中找到并展开 `Asset Actions`。
9. 点击 `Migrate`。不要点 `Export`、`Duplicate` 或 `Advanced Copy Here`。
10. UE 会弹出 `Asset Report`。这里列出的不只有 Arms，还会有 Skeleton、材质、纹理等依赖，这是正常的。
11. 不取消任何勾选，直接点击 Asset Report 右下角的 `OK` 或 `Migrate`。
12. Windows 文件夹选择窗口出现后，依次进入：

```text
This PC
`- D:
   `- UnrealProject
      `- Apecox
         `- Content
```

13. 单击选中 `Content` 文件夹本身。此时目标栏应显示：

```text
D:/UnrealProject/Apecox/Content
```

14. 点击 `Select Folder` / `选择文件夹`。
15. 等待 UE 显示迁移成功。不要在文件复制过程中关闭编辑器。

第一个资产完成后，Apecox 磁盘上应出现：

```text
D:/UnrealProject/Apecox/Content/InfimaGames/ArtCore/Mannequin/Meshes/SK_IG_FP_Mannequin.uasset
```

你不需要把它手工移动到 `Blueprints`；保留 `InfimaGames` 路径才是正确结果。

### 1.3 用完全相同的方法迁移另外 6 个资产

每完成一个资产：清空搜索框，输入下一个完整名称，然后重复上面的第 5 至第 15 步。

按这个顺序操作：

```text
1. A_RAR_FP_PCH_AssaultRifle_Idle_Loop
2. A_RAR_FP_PCH_AssaultRifle_Fire
3. SK_RAR_AssaultRifle
4. A_RAR_FP_WEP_AssaultRifle_Fire
5. NS_IG_MuzzleFlash
6. SC_RAR_AssaultRifle_Fire
```

搜索时尤其注意：

```text
A_RAR_...   = 原始 Animation Sequence，本轮要迁移
AM_RAR_...  = 厂商 Montage，本轮不要迁移
```

每次目标都选择同一个目录：

```text
D:/UnrealProject/Apecox/Content
```

不要随着源资产目录变化而在目标中选择不同子目录，UE 会自己建立正确的 `InfimaGames/...` 结构。

### 1.4 做完 7 次以后再打开 Apecox

1. 关闭 RAR 编辑器。
2. 打开：

```text
D:/UnrealProject/Apecox/Apecox.uproject
```

3. 在 Apecox 内容浏览器左侧展开：

```text
Content
`- InfimaGames
```

4. 逐个搜索刚才的 7 个名称。7 个都能搜索到，才算“迁移完成”。
5. 暂时不要创建 AnimBP 或 Montage；先确认迁移结果，再进入本文件第二节。

如果任意一步看到的按钮、弹窗或结果与上述不同，停在那一步并截图，不需要自行猜测。

---

### 以下是迁移原理与异常处理，需要时再看

### 附录 A：迁移前准备

1. 保存 Apecox 中尚未保存的地图和资产，然后关闭 Apecox 编辑器。
2. 本机此前出现过 UE 显存与内存压力，因此不要同时打开 RAR 与 Apecox 两个大型编辑器实例。
3. 不要通过 Windows 资源管理器直接复制 `.uasset`。直接复制不会替你检查 Skeleton、Material、Niagara、SoundWave 等依赖，也容易留下缺失引用。
4. 本轮使用 UE 的 `Migrate`，让资产注册表计算依赖并保留原始包路径。

打开已经通过 UE 5.8 验证的 RAR 副本：

```text
D:/UE_Resource/Realistic Assault Rifle Template/UE5/RAR/RAR.uproject
```

确认内容浏览器能看到：

```text
/Game/InfimaGames
```

如果看不到，先确认打开的是上面的 RAR 工程，而不是 Apecox，也不要启用 `Show Engine Content` 来寻找它。

### 附录 B：为什么目标必须选 Apecox 的 Content 根目录

每次弹出目标目录选择器时，始终选择：

```text
D:/UnrealProject/Apecox/Content
```

不要选择 `Content/Blueprints`、`Content/ThirdParty` 或更深的子目录。RAR 资产的包名本来是 `/Game/InfimaGames/...`；选择目标项目的 `Content` 根目录后，UE 会得到：

```text
D:/UnrealProject/Apecox/Content/InfimaGames/...
```

这不是目录失控，而是 UE 正确保留供应商命名空间。Apecox 的 `.gitignore` 已明确忽略 `/Content/InfimaGames/`，这些商业原始资产不会进入公共仓库。

### 附录 C：需要迁移的四组资产

建议逐个迁移，便于你观察每类资产带来的依赖。已经复制过的相同依赖不会因为后续迁移而生成第二份。

#### A. 第一人称 Manny Arms Mesh

```text
/Game/InfimaGames/ArtCore/Mannequin/Meshes/SK_IG_FP_Mannequin
```

它是第一人称专用的 Manny 外观视图模型，会带入 `SKEL_IG_Mannequin`、Physics Asset、材质和纹理等依赖。不要用完整第三人称身体 `SKM_Manny` 替代它。此前已经迁移的真人皮肤 `SK_IG_FP_Arms` 可以保留，但本阶段不再引用。

#### B. 第一人称角色 Idle 与 Fire

```text
/Game/InfimaGames/RealisticAssaultRifle/Art/Character/Animations/Movement/A_RAR_FP_PCH_AssaultRifle_Idle_Loop
/Game/InfimaGames/RealisticAssaultRifle/Art/Character/Animations/Actions/A_RAR_FP_PCH_AssaultRifle_Fire
```

二者都是原始 Animation Sequence。不要把旁边同名的 `AM_RAR_FP_PCH_AssaultRifle_Fire` 选进来，因为厂商 Montage 带有 Apecox 不使用的 `Update Tags` Notify 和双 Overlay Track。

#### C. RAR 武器 Mesh 与机械 Fire

```text
/Game/InfimaGames/RealisticAssaultRifle/Art/Weapon/Meshes/SK_RAR_AssaultRifle
/Game/InfimaGames/RealisticAssaultRifle/Art/Weapon/Animations/A_RAR_FP_WEP_AssaultRifle_Fire
```

武器动画使用自己独立的 `SKEL_RAR_AssaultRifle`，不会和 Arms 的 `SKEL_IG_Mannequin` 合并。这正是代码同时播放 Arms Montage 与 Weapon Montage 的原因。

#### D. 枪口 VFX 与开火 SFX

```text
/Game/InfimaGames/ArtCore/VFX/Systems/NS_IG_MuzzleFlash
/Game/InfimaGames/RealisticAssaultRifle/Audio/SC_RAR_AssaultRifle_Fire
```

Niagara 会带入使用的材质、纹理和 Niagara Script；Sound Cue 会带入对应 SoundWave。这些都是预期依赖。

### 附录 D：每个资产的通用迁移规则

对上面每个入口资产执行：

1. 在左侧目录树进入给出的目录。
2. 在右侧资产列表确认名称完全一致，特别区分 `A_` 与 `AM_`。
3. 右键资产，选择 `Asset Actions -> Migrate`。
4. UE 打开 `Asset Report` 后，保留它自动勾选的依赖，不要手动取消 Skeleton、Material、Texture、Niagara 或 SoundWave。
5. 在 Asset Report 点击 `Migrate` 或 `OK`，选择 `D:/UnrealProject/Apecox/Content`。
6. 等待出现迁移成功提示，再处理下一个入口资产。

不同 UE 5.8 布局里 `Asset Actions` 可能位于右键菜单的折叠分组中；使用的是 `Migrate`，不是 `Export`、`Duplicate` 或 `Advanced Copy Here`。

### 附录 E：如何判断 Asset Report 是否正常

正常依赖通常位于：

```text
/Game/InfimaGames/ArtCore/...
/Game/InfimaGames/RealisticAssaultRifle/Art/...
/Game/InfimaGames/RealisticAssaultRifle/Audio/...
```

数量大于 7 很正常，因为 Mesh、Niagara 和 Sound Cue 都引用底层资源。

如果 Asset Report 出现下面目录，先取消本次迁移并截图反馈：

```text
/Game/InfimaGames/RealisticAssaultRifle/Demo/...
/Game/InfimaGames/GameplayCore/...
任何 Map、Widget、Input、Character/Weapon Gameplay Blueprint
```

这些内容说明误选了厂商 Montage、Demo Blueprint 或更高层资产，不属于本轮需要的纯美术依赖。

### 附录 F：文件冲突怎么处理

- 第一次迁移通常不会有冲突。
- 如果只是本轮重复迁移，而你从未修改 Apecox 中的 `/Game/InfimaGames` 原始资产，可以允许覆盖相同供应商文件。
- 如果你已经在 Apecox 中修改过同名供应商资产，不要直接选择 `Yes All`；取消迁移并反馈具体文件名。
- 后续 Apecox 自己的 Montage 和 AnimBP 会放在 `/Game/Blueprints`，因此正常情况下不需要修改 `/Game/InfimaGames` 原始文件。

### 附录 G：迁移后的完整核对

全部迁移成功后关闭 RAR 编辑器，再打开 Apecox。依次确认：

1. `/Game/InfimaGames` 出现在内容浏览器中。
2. `SK_IG_FP_Mannequin` 能正常预览，Skeleton 显示为 `SKEL_IG_Mannequin`。
3. `A_RAR_FP_PCH_AssaultRifle_Idle_Loop` 与 `A_RAR_FP_PCH_AssaultRifle_Fire` 都能在 `SK_IG_FP_Mannequin` 上播放，没有错骨。
4. `SK_RAR_AssaultRifle` 材质正常，Skeleton 为 `SKEL_RAR_AssaultRifle`。
5. 在武器 Skeleton Tree 中能找到骨骼 `weapon_r_muzzle`。
6. `A_RAR_FP_WEP_AssaultRifle_Fire` 能驱动武器机械动作。
7. `NS_IG_MuzzleFlash` 预览不是空白；首次打开需要等待 Niagara 编译。
8. `SC_RAR_AssaultRifle_Fire` 能正常试听。
9. `/Game/InfimaGames/GameplayCore` 与 `/Game/InfimaGames/RealisticAssaultRifle/Demo` 没有因为本轮迁移被整套带入。

出现重定向器提示可以暂时忽略；不要对整个项目执行批量 `Fix Up Redirectors`。

上述 9 项确认后，再进入“二、创建项目适配目录”。

## 二、创建项目适配目录

在 Apecox 创建：

```text
/Game/Blueprints/Weapons/Rifle/Animations/FirstPerson
/Game/Blueprints/Weapons/Rifle/Animations/ThirdPerson
/Game/Blueprints/GameplayCues
```

供应商原始资产继续留在 `/Game/InfimaGames`，不要移动或重命名。下面新建的 AnimBP、Montage 和 GameplayCue 才进入 `/Game/Blueprints`。

## 三、创建第一人称 Arms AnimBP（Manny 外观）

1. 在内容浏览器找到 `SK_IG_FP_Mannequin` 并双击打开。如果资产尚不存在，先回到 1.2，只迁移这个 Mesh；不要继续使用真人皮肤的 `SK_IG_FP_Arms`。
2. 如果当前进入了 `Edit Skeleton` 模式，先点击底部 `Cancel` 退出，不要点击 `Apply to Asset`。在右侧 `Asset Details` 中搜索 `Skeleton`，确认它是 `SKEL_IG_Mannequin`，然后关闭资产编辑标签页。
3. 回到内容浏览器，右键 `SK_IG_FP_Mannequin`，选择 `Create -> Animation Blueprint`。不要创建 Control Rig，也不要直接修改供应商 Skeleton。
4. UE 5.8 从 Skeletal Mesh 创建 AnimBP 时可能不再显示 Parent Class / Target Skeleton 确认窗口，这是正常的：它会自动使用 `AnimInstance` 和该 Mesh 的 `SKEL_IG_Mannequin`。
5. 保存到：

```text
/Game/Blueprints/Weapons/Rifle/Animations/FirstPerson/ABP_Apecox_Rifle_FP_Arms
```

6. 打开 AnimGraph。RAR 的 `A_RAR_FP_PCH_AssaultRifle_Idle_Loop` 是 `Local Space Additive` 动画，不能直接连接 `Output Pose`；否则手臂会消失。
7. 添加第一个 `Sequence Player` 作为完整基础姿势，Sequence 设置为：

```text
A_RAR_FP_PCH_AssaultRifle_Idle_Pose
```

该资产通常会随 `Idle_Loop` 作为依赖自动迁移；如果内容浏览器中不存在，再从 RAR 原项目单独迁移它。

8. 添加第二个 `Sequence Player` 作为呼吸/晃动叠加姿势，Sequence 设置为：

```text
A_RAR_FP_PCH_AssaultRifle_Idle_Loop
```

9. 确认 `Idle_Loop` 的 Sequence Player 循环播放。
10. 添加 `Apply Additive` 节点：`Idle_Pose` 连接 `Base`，`Idle_Loop` 连接 `Additive`，Alpha 保持 `1.0`。
11. 添加 `Slot` 节点，Slot Name 使用 `DefaultGroup.DefaultSlot`。
12. 连接：

```text
Idle_Pose Sequence Player -> Apply Additive.Base
Idle_Loop Sequence Player -> Apply Additive.Additive
Apply Additive -> Slot(DefaultSlot) -> Output Pose
```

13. Preview Mesh 选择 `SK_IG_FP_Mannequin`，编译并保存。`Allow Different Skeletons` 不需要勾选。

本轮只要求 Idle 基线与 Fire Montage 正常叠加；RAR 四向移动 BlendSpace 留到专门的 FP Locomotion 阶段，不在这里扩张。

## 四、创建两个第一人称 Fire Montage

### 4.1 Arms Fire

1. 右键 `A_RAR_FP_PCH_AssaultRifle_Fire`。
2. 选择 `Create -> Create AnimMontage`。
3. 将新 Montage 移动并命名为：

```text
/Game/Blueprints/Weapons/Rifle/Animations/FirstPerson/AM_Apecox_Rifle_FP_Arms_Fire
```

4. Montage 只保留一个 `DefaultGroup.DefaultSlot` Track。
5. Notify 时间轴不得出现 `Update Tags`；本轮 Fire Cue 已经决定射击表现，不依赖动画通知。

### 4.2 Weapon Fire

1. 右键 `A_RAR_FP_WEP_AssaultRifle_Fire`。
2. 选择 `Create -> Create AnimMontage`。
3. 移动并命名为：

```text
/Game/Blueprints/Weapons/Rifle/Animations/FirstPerson/AM_Apecox_Rifle_FP_Weapon_Fire
```

4. 保持单个 `DefaultGroup.DefaultSlot` Track，编译并保存。

Weapon Mesh 没有 AnimBP，运行时会由已经审查通过的受限 Single Node 路径播放这个 Montage。

## 五、创建第三人称 Fire Montage 与 AnimBP 适配

本轮先用项目已有、与 Manny 兼容的：

```text
/Game/ThirdParty/Epic/UE58_FirstPerson/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire
```

不在本轮执行 Rifle Pro IK Retarget。

1. 若 `MM_Rifle_Fire` 是 Animation Sequence，右键选择 `Create AnimMontage`；若编辑器显示它已经是 AnimMontage，则直接 Duplicate。
2. 移动并命名为：

```text
/Game/Blueprints/Weapons/Rifle/Animations/ThirdPerson/AM_Apecox_Rifle_TP_Character_Fire
```

3. Montage Slot 使用 `DefaultGroup.DefaultSlot`。
4. 将现有第三人称 AnimBP：

```text
/Game/ThirdParty/Epic/UE58_FirstPerson/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed
```

Duplicate 到：

```text
/Game/Blueprints/Characters/Animations/ABP_Apecox_Manny
```

5. 打开 `ABP_Apecox_Manny` 的 AnimGraph，在原有最终姿态与 `Output Pose` 之间插入 `Slot` 节点，Slot Name 为 `DefaultGroup.DefaultSlot`。
6. 编译并保存。

本轮允许 TP Fire 短暂全身覆盖。移动开火时若脚部观感一般，记录为后续“Rifle UpperBody Anim Layer”工作，不算当前 GameplayCue 链路失败。

## 六、配置 BP_ApecoxPlayerCharacter

打开当前 GameMode 实际使用的 `BP_ApecoxPlayerCharacter`。

### FirstPersonMesh

```text
Skeletal Mesh Asset = SK_IG_FP_Mannequin
Anim Class          = ABP_Apecox_Rifle_FP_Arms
Relative Location   = 0, 0, -162（从旧 Manny 层级换算出的第一轮基线）
Relative Rotation   = Roll 0, Pitch 0, Yaw -90
Relative Scale      = 1, 1, 1
```

保留 C++ 给出的 `Only Owner See`、NoCollision 和 FirstPerson Primitive 设置。若视图中手臂偏移或裁切，只调整 `FirstPersonMesh` 的 Relative Transform，不移动 `FirstPersonCamera`，也不要重新附着到 `head` Socket。

### 第三人称 Mesh（继承的 Mesh）

```text
Skeletal Mesh Asset = 保持当前 SKM_Manny_Simple
Anim Class          = ABP_Apecox_Manny
```

编译并保存蓝图。

## 七、配置 DA_WeaponPresentation_Rifle

打开：

```text
/Game/Blueprints/Weapons/Rifle/DA_WeaponPresentation_Rifle
```

填写：

| 字段 | 值 |
| --- | --- |
| `First Person Weapon Mesh` | `SK_RAR_AssaultRifle` |
| `First Person Attach Socket` | `SOCKET_Weapon` |
| `First Person Attach Transform` | 先保持 Identity |
| `Third Person Weapon Mesh` | `SK_RAR_AssaultRifle` |
| `Third Person Attach Socket` | `hand_r` |
| `Third Person Attach Transform` | 先保留当前已验证值；若原先不是 RAR Mesh，稍后从观察端微调 |
| `World Pickup Mesh` | 保持当前可用值，不要求本轮替换 |
| `First Person Arms Fire Montage` | `AM_Apecox_Rifle_FP_Arms_Fire` |
| `First Person Weapon Fire Montage` | `AM_Apecox_Rifle_FP_Weapon_Fire` |
| `Third Person Character Fire Montage` | `AM_Apecox_Rifle_TP_Character_Fire` |
| `Third Person Weapon Fire Montage` | `AM_Apecox_Rifle_FP_Weapon_Fire` |
| `Muzzle Socket Name` | `weapon_r_muzzle` |
| `Muzzle Flash System` | `NS_IG_MuzzleFlash` |
| `Fire Sound` | `SC_RAR_AssaultRifle_Fire` |

`weapon_r_muzzle` 是 RAR 武器 Skeleton 上的枪口骨骼名；SkeletalMeshComponent 的 Socket API 可以使用骨骼名。打开 `SK_RAR_AssaultRifle`，在 Skeleton Tree 中确认该名称存在。若实际大小写或名称不同，以编辑器显示的真实名称为准并反馈，不要猜一个新名称。

## 八、创建 GCN_Weapon_Fire

1. 在 `/Game/Blueprints/GameplayCues` 右键 `Blueprint Class`。
2. 搜索并选择父类 `GameplayCueNotify_Burst`，命名：

```text
GCN_Weapon_Fire
```

3. 打开 Class Defaults，将 Gameplay Cue Tag 设置为：

```text
GameplayCue.Weapon.Fire
```

4. 在 Event Graph 添加 `Event On Burst`。
5. 按以下顺序连接：

```text
Event On Burst.Target
-> Cast To ApecoxPlayerCharacter
-> Get Equipment Component
-> Play Fire Presentation
```

6. `Parameters` 与 `Spawn Results` 本轮不连接；这个 Cue 只负责把目标角色转发到组件表现入口。
7. 不添加 Delay、Multicast、扣弹、Trace、伤害或动画 Notify 逻辑。
8. 编译并保存。

## 九、Standalone 验证

1. 运行 `L_Apecox_DevGym`，拾取步枪。
2. 第一人称确认 Arms 与 RAR 武器均可见，没有同时看到第三人称身体。
3. 单击及长按鼠标左键。
4. 每一发应同时出现：Arms Fire、Weapon Fire、枪口 Niagara、开火声音。
5. 动画、VFX、SFX 的节奏应与现有 RPM 一致，不应同一发明显播放两次。
6. 已有弹匣扣除、射速、命中伤害、Shot Confirmation 必须保持原有正确行为。
7. 若 `showdebug abilitysystem` 遮住准星或改变视觉判断，再输入一次同命令关闭 Debug 后观察；它不会删除准星 Widget。

若 Arms 或 Weapon 完全不播放，先检查：Anim Class、Montage Skeleton、Slot Name 和 DataAsset 字段，不要修改 GA。

## 十、Listen Server 双端验证

地图中至少放置两把步枪拾取物，让 Host 与 Client 都能装备。

### Host 开火

- Host 自己窗口：只看到 FP Arms/Weapon Fire。
- Client 观察 Host：看到 Host 的 TP Character/Weapon Fire 与 TP 枪口表现。

### Client 开火

- Client 自己窗口：只看到 FP Arms/Weapon Fire。
- Host 观察 Client：看到 Client 的 TP Character/Weapon Fire 与 TP 枪口表现。

两条路径都必须满足：

- 观察端看不到对方的 FP Arms。
- 开火者本地不显示自己的 TP 身体。
- 每发只播放一次声音、枪口特效和动画，没有预测 Cue 与权威 Cue 的肉眼可见双播。
- 服务器仍唯一扣弹和应用伤害；Rejected Shot 不产生权威命中伤害。
- 停止开火后没有持续残留的 Niagara 或音频组件。

## 十一、反馈格式

完成后直接回复：

```text
RAR 最小资产迁移：通过 / 异常
FP Arms Idle 与 Fire：通过 / 异常
FP Weapon Fire：通过 / 异常
Muzzle VFX / Fire SFX：通过 / 异常
Standalone：通过 / 异常
Host 自己 FP、Client 观察 Host TP：通过 / 异常
Client 自己 FP、Host 观察 Client TP：通过 / 异常
同一发是否双播：否 / 是
原有弹匣、伤害、Shot Confirmation：通过 / 异常
需要微调的 Arms 或 TP Weapon Transform：描述或截图
```

---

## 已完成记录：Phase 2B-2A RAR 资产验证

本轮只在第三方资源工程中检查资产，不修改 Apecox，不执行迁移。

## 一、打开可牺牲副本

两套 RAR 工程内容完全相同。保留 `RAR_5.4` 作为原始备份，本轮使用下面这份副本：

```text
D:/UE_Resource/Realistic Assault Rifle Template/UE5/RAR/RAR.uproject
```

用 `E:/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe` 打开它：

1. 若提示版本不一致，选择打开副本或就地转换这份 `UE5/RAR` 副本。
2. 不转换 `RAR_5.4/RAR` 原始备份。
3. 等待 Shader 与资产发现完成。
4. 若出现编译旧 Blueprint、材质或 Niagara 的警告，先记录，不批量修复或删除资产。

成功标准：工程能进入编辑器，内容浏览器能看到 `/Game/InfimaGames`，没有导致编辑器退出的错误。

## 二、运行原始演示

打开：

```text
/Game/InfimaGames/RealisticAssaultRifle/Demo/Maps/L_RAR_Tutorial_AssaultRifle
```

运行 PIE，按演示界面的现有提示操作。只确认：

- 第一人称手臂和步枪可见，没有明显错骨、手部严重脱离或材质丢失。
- Idle、移动和开火可正常播放。
- 枪口火焰与开火声音至少能正常出现一次。
- 停止 PIE 后编辑器保持稳定。

演示 Blueprint 是否支持多人不在本轮验证范围内。

## 三、检查 FP 角色开火资产

打开目录：

```text
/Game/InfimaGames/RealisticAssaultRifle/Art/Character/Animations/Actions
```

依次检查：

```text
A_RAR_FP_PCH_AssaultRifle_Fire
AM_RAR_FP_PCH_AssaultRifle_Fire
```

记录下面四项：

1. Animation Sequence 是否能正常预览。
2. 资产详情中的 Skeleton 名称。
3. Montage 使用的 Slot 名称。
4. Montage 时间轴上全部 Notify / Notify State 的名称。

不要删除或改写原有 Notify。我们稍后判断哪些属于演示逻辑、哪些需要复制成 Apecox 自有语义事件。

## 四、检查枪械自身动画

打开：

```text
/Game/InfimaGames/RealisticAssaultRifle/Art/Weapon/Animations
```

检查：

```text
A_RAR_FP_WEP_AssaultRifle_Fire
AM_RAR_FP_WEP_AssaultRifle_Fire
```

确认：

- 枪械骨骼动画可以预览。
- 开火时枪机或枪械部件有正常运动。
- 角色 Fire 与武器 Fire 的时长大致能够配套；无需完全相同。

## 五、检查 Mesh、VFX 与 SFX

依次打开并点击预览：

```text
/Game/InfimaGames/ArtCore/Mannequin/Meshes/SK_IG_FP_Mannequin
/Game/InfimaGames/RealisticAssaultRifle/Art/Weapon/Meshes/SK_RAR_AssaultRifle
/Game/InfimaGames/ArtCore/VFX/Systems/NS_IG_MuzzleFlash
/Game/InfimaGames/ArtCore/VFX/Systems/NS_IG_Impact_Bullet
/Game/InfimaGames/RealisticAssaultRifle/Audio/SC_RAR_AssaultRifle_Fire
```

确认：

- `SK_IG_FP_Mannequin` 能正确播放前述第一人称角色 Fire 动画。
- 步枪 Mesh 材质、弹匣和主要部件正常。
- 两个 Niagara System 的预览不是空白。
- Fire Sound Cue 能正常发声。

## 六、反馈格式

完成后直接回复：

```text
RAR UE 5.8 工程：通过 / 未通过
演示 Idle、移动、开火：通过 / 哪一项异常
FP Fire Skeleton：实际名称
FP Fire Montage Slot：实际名称
FP Fire Montage Notify：按时间顺序列出名称
Weapon Fire 动画：通过 / 异常
可用 FP Mesh：资产名
MuzzleFlash：通过 / 异常
Impact：通过 / 异常
Fire SFX：通过 / 异常
其他警告或截图：有则附上
```

本轮结束时不要从 RAR 工程向 Apecox 执行 `Migrate`。
