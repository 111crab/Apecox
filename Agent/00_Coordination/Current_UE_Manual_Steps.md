# 当前 UE 编辑器人工操作清单

更新日期：2026-08-06  
用途：Phase 0 首次验证后的三项收口修正。本文件已覆盖上一轮完整操作。

## 一、彻底关闭 Hardware Ray Tracing

当前配置是：

```text
r.RayTracing=True
r.Lumen.HardwareRayTracing=False
```

这表示只关闭了 Lumen 使用硬件光追，项目的 Ray Tracing 支持仍然开启。

打开 `Edit > Project Settings > Engine - Rendering`。按 UE 5.8 当前界面检查：

1. 在 `Lumen` 分组中，保持 `Use Hardware Ray Tracing when available` 关闭。
2. 在 `Direct Lighting` 分组中，保持 `Ray Traced Shadows` 关闭。
3. 在 `Hardware Ray Tracing` 分组中，关闭当前仍被勾选的 `Support Hardware Ray Tracing`。
4. 同一分组中的 `Generate Ray Tracing Proxies` 和 `Path Tracing` 本项目也不使用，可以一并关闭。
5. 在 `Software Ray Tracing` 分组中，保持 `Generate Mesh Distance Fields` 开启；Lumen 软件追踪需要它。
6. 保持 Lumen、Virtual Shadow Maps、DX12 和 SM6 不变。
7. 按提示重启编辑器。

成功后 `Config/DefaultEngine.ini` 应出现 `r.RayTracing=False`，或不再保存开启值。

## 二、显式启用 Gameplay Abilities

UE 5.8 的 `GameplayAbilities` 插件默认并不启用。当前 `.uproject` 已移除 `GASToolsets`，但没有正式 GAS 插件条目。

打开 `Edit > Plugins`：

1. 搜索友好名称 `Gameplay Abilities`。
2. 启用 Epic 官方插件。
3. 确认 `GASToolsets` 仍为关闭状态。
4. 重启编辑器。

成功后 `Apecox.uproject` 应出现：

```json
{
  "Name": "GameplayAbilities",
  "Enabled": true
}
```

`Enhanced Input` 是 UE 5.8 默认启用插件，且 Apecox 模块已经依赖 `EnhancedInput`，本轮不必强行在 `.uproject` 中增加重复条目。

## 三、确认 DevGym 路径并清理重定向

路径名称说明：UE 编辑器里的 `Content` 就是代码和配置资源路径中的 `/Game` 虚拟挂载点，并不存在一个需要创建的 `Game` 文件夹。

```text
编辑器：Content/Blueprints/Maps/L_Apecox_DevGym
磁盘：  D:/UnrealProject/Apecox/Content/Blueprints/Maps/L_Apecox_DevGym.umap
资源：  /Game/Blueprints/Maps/L_Apecox_DevGym
```

当前正式地图资源路径已经接受为：

```text
/Game/Blueprints/Maps/L_Apecox_DevGym
```

这里把 `/Game/Blueprints` 作为 Apecox 项目自有 Gameplay 内容根，不要求其中只能存 Blueprint。地图无需移动。

1. 打开 `Project Settings > Project > Maps & Modes`，确认两个默认地图仍指向 `/Game/Blueprints/Maps/L_Apecox_DevGym`。
2. 在 Content Browser 设置中启用 `Show Redirectors`。
3. 查看 `/Game` 根目录的 1.3 KB 同名资产。
4. 如果它显示为 Redirector，对 `/Game` 根目录执行 `Fix Up Redirectors in Folder`，修复后它应自动消失。
5. 如果它显示为真正的 Level，先告诉 Codex，不要直接删除。
6. 对 `/Game/Blueprints` 执行一次 `Fix Up Redirectors in Folder`。
7. 执行 `Save All`。

## 四、最终验证

1. 关闭并重新打开编辑器。
2. 项目默认进入 `/Game/Blueprints/Maps/L_Apecox_DevGym`。
3. 单人 PIE 正常。
4. `Play As Listen Server`、2 Players 正常。
5. Output Log 没有插件、地图重定向或 Shader 平台错误。

完成后告诉 Codex“修正完成”。Codex 将复核并执行首个 commit 与 push。
