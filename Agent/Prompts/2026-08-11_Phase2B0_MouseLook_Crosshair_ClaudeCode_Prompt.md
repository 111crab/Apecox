# Claude Code 执行 Prompt：Phase 2B-0 鼠标灵敏度与简单准星

## 一、身份与协作边界

你是 Unreal Engine 5.8 C++ 项目 Apecox 的代码实施子代理。

- 用户负责审核公开命名、完成 UE 编辑器检查和最终 PIE 验证。
- Codex 负责架构设计、代码审查和修复意见。
- 你只负责严格按本 Prompt 实施 C++、执行构建并提交中文报告。

项目路径：`D:/UnrealProject/Apecox`

引擎路径：`E:/UE_5.8`

本轮禁止调用 UE MCP，禁止创建或修改 `.uasset/.umap`，禁止执行 Git add/commit/push，禁止重新生成 Rider/Visual Studio 工程文件。

## 二、开始前必须阅读

按顺序阅读：

1. `Agent/README.md`
2. `Agent/00_Coordination/Working_Agreement.md`
3. `Agent/00_Coordination/Current_Phase.md`
4. `Agent/00_Coordination/Current_Code_Design.md`
5. `Agent/00_Coordination/Subagent_Review_Checklist.md`
6. `.agents/ue-project-context.md`
7. 下列当前源码：
   - `Source/Apecox/Public/Player/ApecoxPlayerController.h`
   - `Source/Apecox/Private/Player/ApecoxPlayerController.cpp`
   - `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
   - `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
   - `Source/Apecox/Public/Game/ApecoxGameMode.h`
   - `Source/Apecox/Private/Game/ApecoxGameMode.cpp`
   - `Source/Apecox/Apecox.Build.cs`

可定向阅读本机 UE 5.8 的 `APlayerController`、`APawn::AddControllerYawInput`、`AHUD` 和 `UCanvas` 源码核对 API。不要为本轮复制 Lyra UI 框架。

`Current_Code_Design.md` 是已经批准的规格。若它与真实源码或 UE 5.8 API 冲突，停止冲突部分并在报告中写明证据，不得静默改名或扩大范围。

## 三、本轮目标

完成两个互不污染的本地表现入口：

```text
鼠标 Look 输入
-> Character 接收输入意图
-> PlayerController 应用 0.5 基础灵敏度
-> 引擎原生 Controller Rotation

本地 ApecoxHUD
-> 有有效 Pawn 时在屏幕中心绘制简单十字准星
```

本轮不实现开火、Hitscan、弹药、扩散、后坐力、ADS、命中标记、UMG、用户设置存档或任何 GameplayTag。

## 四、允许修改的文件

新增文件必须保持 Public/Private 镜像目录：

```text
Source/Apecox/Public/UI/ApecoxHUD.h
Source/Apecox/Private/UI/ApecoxHUD.cpp
```

允许修改：

```text
Source/Apecox/Public/Player/ApecoxPlayerController.h
Source/Apecox/Private/Player/ApecoxPlayerController.cpp
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Private/Game/ApecoxGameMode.cpp
```

除报告文件外，不修改其他文件。`Apecox.Build.cs` 预计不需要变化，因为 `AHUD`、`UCanvas` 和 PlayerController 都来自 Engine 模块；只有编译证明确有依赖问题时才能修改，并在报告中说明。

保持现有文件的 UTF-8 编码和用户代码，不重写无关注释，不格式化无关区域。

## 五、强制实现规格

### 5.1 PlayerController 灵敏度入口

在 `AApecoxPlayerController` 增加：

```cpp
void AddMouseLookInput(const FVector2D& LookInput);
```

该函数是 C++ 调用口，不需要 `UFUNCTION`。职责：

- 只处理本地 Controller 的鼠标观察输入；非本地 Controller 安全返回。
- 使用 `MouseLookSensitivity` 同时缩放 X/Y。
- 调用 `APlayerController::AddYawInput` 和 `AddPitchInput`，零分量可以跳过。
- 不乘 `DeltaTime`。
- 不发送 RPC、不复制灵敏度、不手动同步 ControlRotation。

增加私有配置：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Input|Mouse Look",
    meta = (AllowPrivateAccess = "true", ClampMin = "0.01", UIMin = "0.01", UIMax = "2.0"))
float MouseLookSensitivity = 0.5f;
```

保留它为统一基础倍率，不预先加入水平/垂直拆分、ADS 倍率、FOV 缩放、运行时 Setter 或用户设置类。

### 5.2 Character 观察输入

修改 `AApecoxPlayerCharacter::HandleLookInput(...)`：

- 读取现有 `FVector2D`。
- 获取自己的 `AApecoxPlayerController`。
- 成功时只调用 `AddMouseLookInput(LookVector)`。
- 没有 Controller 或类型不匹配时安全返回；不要在此添加网络逻辑。
- 删除该函数内直接调用 `AddControllerYawInput/AddControllerPitchInput` 的旧路径。
- 不修改 IA/IMC、上下反转约定、移动函数和输入绑定方式。

### 5.3 `AApecoxHUD`

新增：

```cpp
UCLASS()
class APECOX_API AApecoxHUD : public AHUD
```

覆写：

```cpp
virtual void DrawHUD() override;
```

增加 `EditDefaultsOnly` 配置：

```text
CrosshairColor          FLinearColor，默认白色
CrosshairLineLength     float，建议默认 8.0
CrosshairLineThickness  float，建议默认 2.0
CrosshairGap            float，建议默认 4.0
```

浮点值使用合理的非负 Clamp 元数据。成员放在 `Apecox|HUD|Crosshair` 分类。

`DrawHUD()` 要求：

1. 先调用 `Super::DrawHUD()`。
2. `Canvas`、`PlayerOwner` 或 `PlayerOwner->GetPawn()` 无效时返回。
3. 使用 `Canvas->ClipX/ClipY` 计算准确屏幕中心。
4. 使用 `UCanvas` 绘制上、下、左、右四段线；中心保留 `CrosshairGap`，每段长度为 `CrosshairLineLength`。
5. 不创建 Tick、纹理、材质、Widget、动态 UObject 或网络状态。
6. 不读取 WeaponInstance、ASC 或 HealthComponent；准星当前只表达统一屏幕中心。

如果 UE 5.8 的 Canvas API 与预期不同，使用引擎实际存在的等价线段绘制 API，并在报告中说明。

### 5.4 GameMode 接入

在 `AApecoxGameMode` 构造函数中：

- 引入 `UI/ApecoxHUD.h`。
- 设置 `HUDClass = AApecoxHUD::StaticClass();`。
- 不修改现有 GameState、PlayerController、PlayerState、DefaultPawn 和重生逻辑。

不要创建 `BP_ApecoxHUD`，不要修改 `BP_ApecoxGameMode` 或地图。代码审查通过后由用户检查 Blueprint 是否继承到新的 HUDClass。

## 六、代码质量要求

- 新增注释必须使用中文，同时解释“做什么”和“为什么归属于该类”。
- 不引入新的组件、Subsystem、Interface、GameplayTag、委托或抽象层。
- 不把灵敏度放入 Character、WeaponDefinition、IMC 或 Config。
- 不改动 Phase 2A 的 Inventory、Equipment、Weapon、Health、Death、Respawn 和 GAS 输入处理。
- 不隐藏编译警告，不使用无意义 Cast、Tick 或网络 RPC。

## 七、构建验证

关闭 Live Coding；如果 UE 编辑器正在占用构建产物，不要强制结束用户进程，报告阻塞即可。

执行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

编译成功不等于 PIE 验证通过。你不进入 UE 编辑器，也不声称完成运行验证。

## 八、完成报告

写入：

```text
Agent/Reports/2026-08-11_Phase2B0_MouseLook_Crosshair_Report.md
```

报告必须使用中文并包含：

- 实际修改和新增的全部文件。
- `AApecoxHUD` 的父类、职责、成员和 `DrawHUD()` 行为。
- `AApecoxPlayerController` 新增函数和成员的完整签名、可见性、反射暴露与职责。
- Character 输入链路如何变化。
- 是否修改 Build.cs、Config、蓝图、资产、GameplayTag、GAS 或网络逻辑。
- 构建命令和完整结果。
- 与 Prompt 的任何偏差、剩余风险和建议的 UE 验证点。

完成后停止，等待 Codex 审查。不要继续设计或实现 Phase 2B Hitscan。
