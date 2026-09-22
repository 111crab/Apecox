// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ApecoxPlayerController.generated.h"

class UApecoxInventoryComponent;
class UApecoxWeaponStateComponent;

/**
 * AApecoxPlayerController
 * - Apecox 项目级 PlayerController
 * - 拥有 InventoryComponent 管理跨 Pawn 的私有库存。
 * - 拥有 WeaponStateComponent 管理射击确认和命中标记。
 * - 每帧在 PostProcessInput 中统一调度 ASC 输入处理。
 *
 * 为什么 Inventory 在 Controller 而非 Character：
 * - 库存是玩家持久持有状态，不应随 Pawn 销毁而丢失（即使本轮死亡清空）。
 * - PlayerController 跨 Pawn 存活，PlayerState/ASC 也跨 Pawn 存活，
 *   物品持有与技能身份放在同一生命周期层。
 *
 * 为什么 WeaponStateComponent 在 Controller：
 * - 射击确认是玩家级别的反馈——无论当前持有哪个 Pawn/武器，
 *   未确认的 Shot ID 和命中标记都属于该玩家。
 * - 同样跨 Pawn 存活，随 Controller 销毁清理。
 */
UCLASS()
class APECOX_API AApecoxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AApecoxPlayerController();

	/** 只读访问私有库存组件 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Inventory")
	UApecoxInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** 只读访问武器状态组件——射击确认与命中标记入口 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Weapon")
	UApecoxWeaponStateComponent* GetWeaponStateComponent() const { return WeaponStateComponent; }

	// ========================================================================
	// 鼠标观察（本地表现）
	// ========================================================================

	/**
	 * 本地鼠标观察入口——应用 PlayerController 拥有的灵敏度后驱动 Controller Rotation。
	 *
	 * 为什么 MouseLookSensitivity 归属 PlayerController 而非 Character：
	 * - 灵敏度是本地玩家偏好，应跨 Pawn 重生和角色切换保持一致。
	 * - Character 只转交二维输入意图，Controller 负责本地表现倍率。
	 * - 本轮不建立完整用户设置系统；后续可由本地设置覆盖该基础值。
	 */
	void AddMouseLookInput(const FVector2D& LookInput,
		const FVector2D& ContextSensitivityMultiplier = FVector2D(1.0, 1.0));

protected:
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Inventory",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxInventoryComponent> InventoryComponent;

	/** 武器状态组件——管理未确认射击和命中反馈，默认复制 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Weapon",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxWeaponStateComponent> WeaponStateComponent;

	/** 本地鼠标观察基础倍率——统一缩放 X/Y，不乘 DeltaTime。
	 *  后续可被玩家设置覆盖，不改变 Character 调用口。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Input|Mouse Look",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", UIMin = "0.01", UIMax = "2.0"))
	float MouseLookSensitivity = 0.25f;
};
