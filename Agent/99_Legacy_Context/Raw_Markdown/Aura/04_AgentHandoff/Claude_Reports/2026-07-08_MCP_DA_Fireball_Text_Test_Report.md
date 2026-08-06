# MCP DA_Fireball_Text 创建与替换测试报告

*创建日期：2026-07-08*
*执行者：Claude*
*任务：通过 MCP 在 UE 编辑器中创建 DA_Fireball_Text 并替换 BP_AuraCharacter 的引用*

## 测试结果摘要

| 步骤 | 操作 | 结果 |
|------|------|------|
| 1 | MCP 连接 | ✅ 成功 — `unreal-mcp v0.6.0`，端口 3000 |
| 2 | 查找现有资产 | ✅ `DA_Fireball`、`BP_AuraCharacter` 均可定位 |
| 3 | 创建 DA_Fireball_Text | ✅ 通过 `duplicate_asset` 复制 |
| 4 | 替换 BP_AuraCharacter | ✅ `set_property` 成功 |
| 5 | 验证引用 | ✅ CDO 确认只有 `DA_Fireball_Text` |
| 6 | 保存资产 | ✅ 2 个包已保存，0 失败 |
| 7 | 字段验证 | ⚠️ MCP 无法读取 DataAsset 的 UPROPERTY 字段 |

## 详细记录

### 1. MCP 连接

```
POST http://127.0.0.1:3000/mcp
→ {"jsonrpc":"2.0","result":{"serverInfo":{"name":"unreal-mcp","version":"0.6.0"}}}
→ Session: 575B8E9E43B99F151A05F1BFB552ED24
```

UE 编辑器 PID 60616，Native MCP 端口 3000，连接正常。

### 2. 资产创建方式

**复制旧资产**（非新建后填写）。

```
工具：manage_asset
动作：duplicate_asset
参数：
  sourcePath: /Game/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball
  destinationPath: /Game/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball_Text

响应：
  success: true
  assetPath: /Game/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball_Text
  assetClass: AuraSkillDefinition
  existsAfter: true
```

因为是通过复制创建，`DA_Fireball_Text` 继承了 `DA_Fireball` 的所有字段值。如果 `DA_Fireball` 配置正确，新资产也应该是等价的。

### 3. 新资产路径

```
/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball_Text
类: AuraSkillDefinition
```

### 4. 修改的 UE 资产

| 资产 | 操作 |
|------|------|
| `DA_Fireball_Text` | 新建（从 `DA_Fireball` 复制） |
| `BP_AuraCharacter` | `StartupSkillDefinitions` 属性修改 |

### 5. BP_AuraCharacter 替换确认

**替换前**：
```
StartupSkillDefinitions: [
  "/Script/Aura.AuraSkillDefinition'.../DA_Fireball.DA_Fireball'"
]
```

**替换后**：
```
StartupSkillDefinitions: [
  "/Script/Aura.AuraSkillDefinition'.../DA_Fireball_Text.DA_Fireball_Text'"
]
```

- ✅ 新引用仅包含 `DA_Fireball_Text`
- ✅ 旧 `DA_Fireball` 已被替换（不是追加）
- ✅ 不存在 `InputTag.1` 双触发风险

### 6. 字段验证情况

**已验证成功**（间接）：
- `SkillTag = Ability.Fire.Fireball`（在 `DA_Fireball` 上已知正确，复制保留）
- `InputTag = InputTag.1`（同上）
- 所有其他字段均从 `DA_Fireball` 复制

**MCP 无法直接读取**：
- 尝试 `get_property` → `PROPERTY_NOT_FOUND`
- 尝试 `find_by_class` 查找 `AuraSkillDefinition` → 0 结果（可能仅搜索已加载 UObject）
- 尝试 `find_by_tag` → 0 结果（搜索 Actor Tag，非 GameplayTag）
- `manage_gas` 的 `get_gas_info` 仅返回资产名和类名，不返回字段值

**MCP 无法直接写入**：
- `set_property` 对 DataAsset 对象不支持（`PROPERTY_NOT_FOUND`）
- `manage_gas` 无设置 DataAsset 字段值的动作

**结论**：MCP 当前版本对自定义 `UPrimaryDataAsset` 类型的细粒度字段（如 `TMap<FGameplayTag, FScalableFloat>`、`TSoftObjectPtr`、`FGameplayTag`）的读写支持有限。复制是一种有效的绕行方案，但字段级别的修改（如改 `DamageTypes` 数值）无法通过 MCP 完成。

### 7. 保存状态

```
Saved 0 world and 2 content packages (skipped 0 transient/temp)
success: true
savedCount: 2
failedCount: 0
```

两包为：`DA_Fireball_Text` 和 `BP_AuraCharacter`。

## MCP 能力总结

| 操作 | 支持程度 |
|------|----------|
| 连接/初始会话 | ✅ 正常 |
| 工具枚举 (tools/list) | ✅ 23 个工具 |
| 复制资产 (duplicate_asset) | ✅ 成功 |
| 保存资产 (save_all) | ✅ 成功 |
| 打开资产 (open_asset) | ✅ 成功 |
| 检查蓝图 CDO (inspect_cdo) | ✅ 可读取基本类型和资产引用数组 |
| 设置蓝图 CDO 属性 (set_property) | ✅ 数组属性替换成功 |
| 读取 DataAsset UPROPERTY | ❌ `PROPERTY_NOT_FOUND` |
| 写入 DataAsset UPROPERTY | ❌ `PROPERTY_NOT_FOUND` |
| 按类查找 DataAsset (find_by_class) | ❌ 0 结果 |
| 按 GameplayTag 查找 | ❌ 0 结果 |
| `TMap<FGameplayTag, FScalableFloat>` 读写 | ❌ 无支持 |
| `TSoftObjectPtr` 字段读写 | ❌ 无支持 |

## 建议手动检查项

建议在 UE 编辑器中进行以下手动验证：

1. **打开 `DA_Fireball_Text`**：
   - 路径：`Content/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball_Text`
   - 确认 `SkillTag` 为 `Ability.Fire.Fireball`
   - 确认 `InputTag` 为 `InputTag.1`
   - 确认 `ProjectileClass` 为 `BP_FireBolt`
   - 确认 `DamageEffectClass` 为 `GE_Damage`
   - 确认 `DamageTypes` 包含 `Damage.Fire` 且数值与 `DA_Fireball` 一致
   - 确认 `CastMontage` 为 `AM_Cast_FireBolt`
   - 确认 `MontageSpawnEventTag` 为 `Event.Montage.FireBolt`

2. **打开 `BP_AuraCharacter`**：
   - 确认 `StartupSkillDefinitions` 数组中只有 `DA_Fireball_Text`
   - 确认 `StartupAbilities` 中的 `GA_FireBolt` 仍保留（旧路径不受影响）

3. **单人 PIE 验证**：
   - 按 `1` 键触发 `DA_Fireball_Text` 路径
   - 验证火球正常施放、发射、命中、造成伤害
   - 按左键触发 `GA_FireBolt` 旧路径，确认仍然可用
