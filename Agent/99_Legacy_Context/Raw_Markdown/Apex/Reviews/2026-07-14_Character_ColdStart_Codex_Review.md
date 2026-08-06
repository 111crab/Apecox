# Character 冷启动 Codex 审查

生成日期：2026-07-14

## 审查结论

未发现阻塞问题。子代理的角色冷启动代码符合本阶段范围：只建立角色基础类、玩家相机和 Move / Look / Jump 输入，不接 GAS、不播放 Montage、不新增 GameplayTag、不自动修改蓝图或地图。

Codex 已追加完成目录规范化：

| 原意 | 最终路径 |
| --- | --- |
| 基础角色头文件 | `Source/Apex/Public/Character/ApexCharacterBase.h` |
| 基础角色实现 | `Source/Apex/Private/Character/ApexCharacterBase.cpp` |
| 玩家角色头文件 | `Source/Apex/Public/Character/ApexPlayerCharacter.h` |
| 玩家角色实现 | `Source/Apex/Private/Character/ApexPlayerCharacter.cpp` |

## Codex 修正

- 将子代理新增的 `.h` / `.cpp` 从同一 `Character/` 目录拆分到 `Public/Character` 与 `Private/Character`。
- 将 `.cpp` 自身头文件 include 改为 `Character/ApexCharacterBase.h` / `Character/ApexPlayerCharacter.h`。
- 给公共 `UCLASS` 增加 `APEX_API` 导出宏。
- 更新 Agent 文档中关于 `Task_Backlog`、`Current_Phase`、`Current_Code_Design`、`Reviews` 的职责边界。

## 命名审阅表

| 名称 | 类型 | 作用 | Codex 意见 |
| --- | --- | --- | --- |
| `AApexCharacterBase` | `ACharacter` 子类 | 角色身体基类，放 Capsule / Movement 默认配置，不放 Camera / Input。 | 合理，建议保留。 |
| `AApexPlayerCharacter` | `AApexCharacterBase` 子类 | 玩家可控角色，放 SpringArm / Camera / Enhanced Input 绑定。 | 合理，建议保留。 |
| `CameraSpringArm` | `USpringArmComponent` | 第三人称相机臂。 | 按用户意见由 `CameraBoom` 改名，更直观，建议保留。 |
| `FollowCamera` | `UCameraComponent` | 跟随相机。 | UE 模板常用命名，建议保留。 |
| `JumpAction` / `MoveAction` / `LookAction` / `MouseLookAction` | `UInputAction` | 蓝图配置的输入动作引用。 | 符合 Enhanced Input 语义，建议保留。 |
| `DoMove` / `DoLook` / `DoJumpStart` / `DoJumpEnd` | `UFUNCTION` | 蓝图可调用的输入处理入口。 | 沿用模板风格，建议保留。 |

## 编译验证

命令：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

结果：成功。

备注：`CameraSpringArm` 改名后已再次编译成功。构建过程中 Unreal Build Accelerator 因系统内存压力重试了若干编译进程，但最终 `Result: Succeeded`。

## 仍需用户人工验证

1. 在 UE 编辑器创建 `BP_Hero_Phase`，父类选 `AApexPlayerCharacter`。
2. 配置 Mesh、Anim Class、`JumpAction`、`MoveAction`、`LookAction`、`MouseLookAction`。
3. 在 GameMode 或关卡 World Settings 指定默认 Pawn。
4. PIE 验证 WASD、鼠标视角和 Jump。

## 剩余风险

- `MoveAction` / `LookAction` / `MouseLookAction` 需要配置为 Axis2D 类型，否则运行时取 `FVector2D` 不符合预期。
- 本次只验证编译，尚未验证 UE 蓝图可创建、PIE 输入是否正确。
- `Content/ParagonPhase/` 等资产改动仍未分类审查，提交前需要单独看工作区。
