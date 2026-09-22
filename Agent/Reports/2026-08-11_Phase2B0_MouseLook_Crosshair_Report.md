# Phase 2B-0 鼠标灵敏度与简单准星 — 实施报告

**日期**: 2026-08-11  
**构建结果**: ✅ 成功（ApecoxEditor Win64 Development）  
**构建耗时**: 16.44 秒（UBA 8.02 秒）  
**编译警告**: 0（新增代码无警告）

---

## 一、文件清单

### 1.1 新增文件（2 个）

| # | 文件 | 职责 |
|---|------|------|
| 1 | `Public/UI/ApecoxHUD.h` | 项目级 HUD——十字准星声明与配置 |
| 2 | `Private/UI/ApecoxHUD.cpp` | DrawHUD 实现——屏幕中心四段线准星 |

### 1.2 修改文件（4 个）

| # | 文件 | 修改内容 |
|---|------|---------|
| 1 | `Public/Player/ApecoxPlayerController.h` | 新增 `AddMouseLookInput`、`MouseLookSensitivity` |
| 2 | `Private/Player/ApecoxPlayerController.cpp` | 实现 `AddMouseLookInput` |
| 3 | `Private/Character/ApecoxPlayerCharacter.cpp` | `HandleLookInput` 改为通过 Controller 路由 |
| 4 | `Private/Game/ApecoxGameMode.cpp` | 构造函数设置 `HUDClass = AApecoxHUD::StaticClass()` |

### 1.3 是否超出 Prompt

否。所有修改均在批准范围内。未修改 Build.cs、.uasset/.umap、GameplayTag、GAS、网络逻辑或 Phase 2A 代码。

---

## 二、AApecoxHUD

- **父类**: `AHUD`
- **文件**: `Public/UI/ApecoxHUD.h` / `Private/UI/ApecoxHUD.cpp`

### 成员变量

| 变量 | 类型 | UPROPERTY | 默认值 | 作用 |
|------|------|-----------|--------|------|
| `CrosshairColor` | `FLinearColor` | `EditDefaultsOnly` | 白色 | 准星四条线段的颜色 |
| `CrosshairLineLength` | `float` | `EditDefaultsOnly, ClampMin=0` | 8.0 | 每条线段长度（像素） |
| `CrosshairLineThickness` | `float` | `EditDefaultsOnly, ClampMin=0` | 2.0 | 线宽（像素） |
| `CrosshairGap` | `float` | `EditDefaultsOnly, ClampMin=0` | 4.0 | 中心点与线段起点间隔（像素） |

### DrawHUD() 行为

```
1. Super::DrawHUD()
2. Canvas == nullptr → 返回
3. PlayerOwner == nullptr → 返回
4. PlayerOwner->GetPawn() == nullptr → 返回
5. 计算屏幕中心 (Canvas->ClipX * 0.5, Canvas->ClipY * 0.5)
6. 使用 FCanvasLineItem 绘制四段线：
   - 上: (CX, CY-Gap) → (CX, CY-Gap-Length)
   - 下: (CX, CY+Gap) → (CX, CY+Gap+Length)
   - 左: (CX-Gap, CY) → (CX-Gap-Length, CY)
   - 右: (CX+Gap, CY) → (CX+Gap+Length, CY)
```

- 不创建 Tick、纹理、材质、Widget、动态 UObject 或网络状态
- 不读取 WeaponInstance、ASC 或 HealthComponent
- 准星当前只表达统一屏幕中心，与武器、扩散、ADS 无关

---

## 三、AApecoxPlayerController 新增 API

### AddMouseLookInput

```cpp
void AddMouseLookInput(const FVector2D& LookInput);
```

- **可见性**: C++ public，无 UFUNCTION（纯 C++ 调用口，非蓝图入口）
- **职责**: 本地鼠标观察入口——应用 Controller 拥有的 `MouseLookSensitivity` 后调用原生 `AddYawInput` / `AddPitchInput`
- **守卫**: `IsLocalPlayerController()` 为非本地返回——不操作远端 ControlRotation
- **实现**: 对 `LookInput * MouseLookSensitivity` 的 X/Y 分别检查零分量，跳过零分量调用
- **不乘 DeltaTime**——鼠标增量已是每帧值
- **不发送 RPC、不复制灵敏度、不手动同步 ControlRotation**
- **位置**: ApecoxPlayerController.cpp:19-40

### MouseLookSensitivity

```cpp
float MouseLookSensitivity = 0.5f;
```

- **可见性**: private，`EditDefaultsOnly, BlueprintReadOnly`
- **元数据**: `ClampMin=0.01, UIMin=0.01, UIMax=2.0`
- **作用**: 本地鼠标观察基础倍率——统一缩放 X/Y
- **为什么归属 PlayerController**: 灵敏度是本地玩家偏好，应跨 Pawn 重生和角色切换保持一致。后续可由本地设置覆盖该基础值，不改变 Character 调用口
- **本轮不加入**: 水平/垂直拆分、ADS 倍率、FOV 缩放、运行时 Setter、用户设置类

---

## 四、Character 输入链路变化

**之前** (`HandleLookInput`):
```cpp
AddControllerYawInput(LookVector.X);
AddControllerPitchInput(LookVector.Y);
```

**之后**:
```cpp
AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(GetController());
if (PC)
{
    PC->AddMouseLookInput(LookVector);
}
```

**职责变化**:
- Character 不再直接调用 `AddControllerYawInput` / `AddControllerPitchInput`
- Character 只转交二维输入意图——不在此处理灵敏度、正负约定或网络逻辑
- Controller 统一管理本地灵敏度倍率

**未修改**:
- IA_Look_Mouse、IMC、BindNativeAction 绑定方式
- 上下反转约定（由用户在 IMC Modifier 中配置）
- HandleMoveInput、HandleJumpStarted/Completed

---

## 五、GameMode 接入

在 `AApecoxGameMode` 构造函数中新增一行：

```cpp
HUDClass = AApecoxHUD::StaticClass();
```

与现有 `GameStateClass`、`PlayerControllerClass`、`PlayerStateClass`、`DefaultPawnClass` 并列。不修改重生、清库存和其他逻辑。

---

## 六、构建验证

```text
构建命令:
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
  -Project="D:\UnrealProject\Apecox\Apecox.uproject"
  -WaitMutex -NoHotReloadFromIDE -architecture=x64

结果: 成功
耗时: 16.44 秒（UBA 8.02 秒）
操作数: 9（6 个编译 + 1 个 .lib + 1 个 .dll + 1 个 WriteMetadata）
新增代码编译警告: 0
```

---

## 七、未执行项与边界

| 项 | 状态 |
|----|------|
| Build.cs 修改 | ❌ 未修改——AHUD/UCanvas 来自 Engine 模块，无需新依赖 |
| GameplayTag | ❌ 未新增 |
| Attribute / GA / GE / GameplayCue | ❌ 未新增 |
| UMG / Widget Blueprint | ❌ 未创建 |
| 纹理 / 材质 / 动态 UObject | ❌ 未创建 |
| 网络 RPC / 复制 | ❌ 未新增 |
| .uasset / .umap 修改 | ❌ 未操作 |
| MCP 调用 | ❌ 未使用 |
| Git 操作 | ❌ 未执行 |
| 灵敏度拆分（水平/垂直/ADS） | ❌ 本轮不实现 |
| 动态扩散准星 | ❌ 本轮不实现 |

---

## 八、与 Prompt 的偏差

无偏差。所有实现严格按 Prompt 规格执行。

**注意**: `-NoHotReloadFromIDE` 参数在构建命令中使用（Prompt 指定），UBT 报告"not currently supported"并忽略——对构建结果无影响。

---

## 九、风险与 UE 验证建议

### 风险
1. **FCanvasLineItem API 稳定性**: `UCanvas::DrawItem` 在 UE 5.8 中正常工作——如果未来版本 API 变化，准星绘制是独立函数，修改范围极小
2. **准星与武器扩散无关**: 当前准星是静态中心十字——Phase 2B 引入动态扩散时需要额外设计准星状态

### UE 验证点（用户完成）
1. **灵敏度**: PIE 单人模式，鼠标观察手感是否流畅；`MouseLookSensitivity=0.5` 是建议起点，可在 PC Blueprint Defaults 中微调
2. **准星**: 确认屏幕中心有白色十字线，四段对称、中心有间隔
3. **Listen Server**: 主机和客户端分别确认准星只在本地显示，远端屏幕不重复绘制
4. **死亡/重生**: 确认重生的新 Pawn 灵敏度不变、准星正常
5. **GameMode Blueprint**: 确认 `BP_ApecoxGameMode` 继承到新的 `HUDClass`（C++ 构造函数应已生效）

---

## 十、声明

- ✅ 未使用 MCP 工具
- ✅ 未修改任何 `.uasset`、`.umap` 资产文件
- ✅ 未执行 Git add/commit/push 操作
- ✅ 未操作 UE 编辑器
- ✅ 未修改 Build.cs、Config、GameplayTag、GAS 或网络逻辑
- ✅ 所有新增和修改均在 Prompt 批准范围内
- ✅ ApecoxEditor Win64 Development 构建通过
