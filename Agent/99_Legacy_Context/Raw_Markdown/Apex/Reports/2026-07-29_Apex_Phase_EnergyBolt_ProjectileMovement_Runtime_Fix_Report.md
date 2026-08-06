# Apex Phase EnergyBolt 投射物移动运行时修复报告

*日期：2026-07-29*

## 1. 根因

`AApexProjectile` 构造函数将 `ProjectileMovement->bAutoActivate` 设为 `false`，但 `SpawnProjectileActorOnAuthority()` 在 `FinishSpawning()` 之前调用 `InitializeProjectile()`（其中调用 `ProjectileMovement->Activate()`）。UE 5.8 的 `UMovementComponent` 在组件注册阶段依据 `bAutoActivate` 重置 Tick 状态，导致过早执行的 `Activate()` 被覆盖，移动组件无 Tick。

此外，`ProjectileMovement` 未显式绑定 `CollisionSphere` 作为 `UpdatedComponent`，依赖注册阶段自动查找根组件，增加了时序不确定性。

## 2. 修改文件

| 文件 | 变更 |
|------|------|
| `D:\UnrealProject\Apex\Source\Apex\Private\CombatEntities\Projectile\ApexProjectile.cpp` | 构造函数添加 `SetUpdatedComponent(CollisionSphere)`（第 25 行）；`InitializeProjectile` 添加成功日志（第 66 行） |
| `D:\UnrealProject\Apex\Source\Apex\Private\AbilitySystem\Abilities\Projectile\ApexProjectileCastAbility.cpp` | `SpawnProjectileActorOnAuthority` 调整 Deferred Spawn 顺序：`FinishSpawning` → `IsValid`/`IsActorBeingDestroyed` → `InitializeProjectile`（第 156-171 行） |

## 3. FinishSpawning 与 InitializeProjectile 修改后的调用顺序

**修改前：**
```
SpawnActorDeferred → InitializeProjectile → FinishSpawning
```

**修改后：**
```cpp
SpawnActorDeferred
→ FinishSpawning
→ if (!IsValid(Proj) || Proj->IsActorBeingDestroyed()) return nullptr;
→ if (!Proj->InitializeProjectile(...)) { Proj->Destroy(); return nullptr; }
→ return Proj;
```

即：先完成 Actor/组件注册，再配置并激活 `ProjectileMovement`。

## 4. SetUpdatedComponent 和新增日志所在行

- `ApexProjectile.cpp` 第 25 行：`ProjectileMovement->SetUpdatedComponent(CollisionSphere);`
- `ApexProjectile.cpp` 第 66 行：`UE_LOG(LogApex, Log, TEXT("[ApexProjectile] 初始化完成：Location=%s, Velocity=%s, Active=%d, UpdatedComponent=%s"), ...)`

## 5. 编译结果

```
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development
      -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex

Result: Succeeded
Total execution time: 5.73 seconds
Exit code: 0
0 errors, 0 warnings
```

## 6. 未完成事项 / 风险

- PIE 验证由用户在 Codex 审查通过后执行
- 客户端投射物表现（粒子拖尾随飞行）依赖 `bReplicates=true` + `SetReplicatingMovement(true)`——本轮未修改

## 7. 用户 PIE 验证步骤与通过标准

1. 按 Q 后 Montage 正常播放
2. GameplayEvent 关键帧在手部 Socket 生成投射物
3. Output Log 显示：`[ApexProjectile] 初始化完成：Location=..., Velocity=..., Active=true, UpdatedComponent=CollisionSphere`
4. 投射物核心特效与拖尾沿屏幕中心方向飞行
5. 投射物撞墙后销毁并触发 Impact GameplayCue
6. 未命中时在 MaxLifetime 后销毁
7. Mana 与 Cooldown 行为不发生回归
