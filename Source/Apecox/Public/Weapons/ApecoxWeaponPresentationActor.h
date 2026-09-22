// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "ApecoxWeaponPresentationActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UDecalComponent;
class UPointLightComponent;
class UApecoxWeaponPresentationDefinition;

/**
 * AApecoxWeaponPresentationActor
 * - 纯表现 Actor：既可由 Blueprint 组装完整第一人称武器，也可作为第三人称镭射的轻量宿主。
 * - 不复制、仅在镭射开启时 Tick、无碰撞，只向 UApecoxEquipmentComponent 提供表现组件。
 *
 * 为什么表现与玩法真相分离：
 * - 战斗真相在服务器 WeaponInstance + EquippedWeaponState 摘要，本 Actor 不保存弹药、
 *   射速、伤害、命中、配件或任何复制状态；
 * - 表现层随本地观察视角自由解释，不参与同步与判定，避免把视觉状态当成战斗真相而引入作弊面。
 */
UCLASS(Blueprintable)
class APECOX_API AApecoxWeaponPresentationActor : public AActor
{
	GENERATED_BODY()

public:
	AApecoxWeaponPresentationActor();
	virtual void Tick(float DeltaSeconds) override;

	/** 武器 SkeletalMesh——枪械机械动作 Montage 的播放目标 */
	UFUNCTION(BlueprintPure, Category = "Apecox|Presentation")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	/** 项目显式定义的枪口锚点——FP 枪口 VFX/SFX 的附着位置，不依赖供应商 Socket 名 */
	UFUNCTION(BlueprintPure, Category = "Apecox|Presentation")
	USceneComponent* GetMuzzlePoint() const { return MuzzlePoint; }

	/** 左手握持锚点——FP AnimBP 的左手 IK 目标，由每把武器的表现 Blueprint 调整 */
	UFUNCTION(BlueprintPure, Category = "Apecox|Presentation")
	USceneComponent* GetLeftHandGripPoint() const { return LeftHandGripPoint; }

	/**
	 * 从 Blueprint 中名为 MagazineDefault 的固定弹匣复制外观，准备一只仅供换弹动画使用的备用弹匣。
	 * 备用弹匣附着到 RAR 武器骨架的 SOCKET_Magazine_Reserve，由武器 Animation Sequence 驱动。
	 */
	bool PrepareReloadMagazineVisuals();

	/** 切换枪槽固定弹匣与换弹备用弹匣的本地可见性。 */
	void SetReloadMagazineVisualState(bool bLoadedMagazineVisible, bool bReserveMagazineVisible);

	/** 回到正常持枪状态：固定弹匣可见、备用弹匣隐藏。 */
	void ResetReloadMagazineVisuals();

	/**
	 * 从武器表现 Data Asset 配置 RAR 镭射美术和 Trace 参数；缺必要资产时安全关闭。
	 * ExternalMountParent 为空时沿用第一人称 Prefab 内部 Socket 搜索；非空时挂到指定的
	 * 第三人称武器组件，ExternalMountTransform 用于把挂件从该 Socket 移到护木侧面。
	 * 完成初始摆放后可转挂到 ExternalStableAttachmentName，避免 Muzzle 机械动画抖动实体挂件。
	 */
	bool ConfigureLaser(const UApecoxWeaponPresentationDefinition* Presentation,
		USceneComponent* ExternalMountParent = nullptr,
		FName ExternalMountSocket = NAME_None,
		FName ExternalStableAttachmentName = NAME_None,
		const FTransform& ExternalMountTransform = FTransform::Identity,
		bool bConvergeToViewAim = true);

	/** 只控制当前实际显示；用户的开关意图由EquipmentComponent保存。 */
	void SetLaserEnabled(bool bEnabled);

	/** 立即隐藏并销毁所有镭射组件，再销毁纯表现 Actor；用于死亡/卸装的确定性清理。 */
	void ShutdownPresentation();

	bool IsLaserConfigured() const { return bLaserConfigured; }

protected:
	/** Prefab 根节点——完整枪械相对 Arms 的统一调整基准 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation")
	TObjectPtr<USceneComponent> PresentationRoot;

	/** RAR 枪体——Blueprint 子类可在此下添加弹匣、护木、机械瞄具等固定部件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** 显式枪口锚点——FP 枪口 VFX/SFX 的附着位置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation")
	TObjectPtr<USceneComponent> MuzzlePoint;

	/** 显式左手握持锚点——保持 hand_l 与护木接触，不参与玩法或复制 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation")
	TObjectPtr<USceneComponent> LeftHandGripPoint;

	/** RAR 换弹动画驱动的第二只弹匣；外观运行时取自 Blueprint 的 MagazineDefault。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation")
	TObjectPtr<UStaticMeshComponent> MagazineReserve;

	/** 固定安装到枪械SOCKET_Laser的镭射挂件外观。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation|Laser")
	TObjectPtr<UStaticMeshComponent> LaserAttachment;

	/** 从挂件发射口起射的光束；FP 沿挂件轴，TP 沿外部武器枪口轴。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation|Laser")
	TObjectPtr<UStaticMeshComponent> LaserBeam;

	/** 只在Visibility Trace命中时投射到世界表面的红点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation|Laser")
	TObjectPtr<UDecalComponent> LaserDot;

	/** RAR光点附带的小范围红色点光，用于近距离表面亮度。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Presentation|Laser")
	TObjectPtr<UPointLightComponent> LaserDotLight;

private:
	UStaticMeshComponent* FindLoadedMagazineComponent() const;
	void UpdateLaserTrace();

	FName LaserEmitterSocketName = NAME_None;
	TEnumAsByte<ECollisionChannel> LaserTraceChannel = ECC_Visibility;
	float LaserMaxDistance = 10000.0f;
	float LaserBeamMeshLength = 20.024246f;
	float LaserBeamThickness = 5.0f;
	float LaserDotSizeBase = 5.0f;
	float LaserDotSizeMultiplier = 0.002f;
	bool bLaserConfigured = false;
	bool bLaserEnabled = false;
	bool bLaserConvergesToViewAim = true;
	bool bDestroyWhenOwnerUnavailable = false;
	bool bShutdownStarted = false;
};
