// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/ApecoxAnimationTypes.h"
#include "ApecoxWeaponPresentationDefinition.generated.h"

class USkeletalMesh;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;
class AApecoxWeaponPresentationActor;

/**
 * UApecoxWeaponPresentationDefinition
 * - 独立表现配置 DataAsset，把武器表现和附着参数从玩法定义中解耦。
 * - 第一人称使用完整武器表现 Prefab（AApecoxWeaponPresentationActor 的 Blueprint 子类）；
 *   第三人称仍使用单一 SkeletalMesh 的简化表现。
 * - 可以在不修改武器玩法定义的情况下替换 UE 5.8 官方、Stephen_FPS 或 Rifle Pro 资产。
 * - 首版使用直接资产引用；常驻装备一定需要资源，暂不引入异步流送状态机。
 */
UCLASS(BlueprintType, Const)
class APECOX_API UApecoxWeaponPresentationDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 第一人称——拥有客户端看到的完整武器表现 Prefab 类（AApecoxWeaponPresentationActor 的 Blueprint 子类） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson")
	TSubclassOf<AApecoxWeaponPresentationActor> FirstPersonWeaponPresentationClass;

	/** 第一人称附着 Socket 名称（如 "weapon_r"） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson")
	FName FirstPersonAttachSocket = NAME_None;

	/** 第一人称附着微调（位置、旋转、缩放） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson")
	FTransform FirstPersonAttachTransform = FTransform::Identity;

	/** 第三人称——其他客户端看到的武器 SkeletalMesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson")
	TObjectPtr<USkeletalMesh> ThirdPersonWeaponMesh;

	/** 第三人称附着 Socket 名称（如 "weapon_r"） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson")
	FName ThirdPersonAttachSocket = NAME_None;

	/** 第三人称附着微调（位置、旋转、缩放） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson")
	FTransform ThirdPersonAttachTransform = FTransform::Identity;

	/** 世界地面拾取物使用的武器 SkeletalMesh——为空时回退到 ThirdPersonWeaponMesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WorldPickup")
	TObjectPtr<USkeletalMesh> WorldPickupMesh;

	// ========================================================================
	// 开火表现
	// ========================================================================

	/**
	 * 第一人称 Arms 腰射开火 Montage——Owning Player 的 RAR Arms 播放。
	 * 通过 Arms 现有 AnimBP 的 Slot 系统播放，绝不切 Single Node。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Fire")
	TObjectPtr<UAnimMontage> FirstPersonArmsFireMontage;

	/** 第一人称枪械机械动作 Montage——Owning Player 的 FP Presentation Actor WeaponMesh 播放 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Fire")
	TObjectPtr<UAnimMontage> FirstPersonWeaponFireMontage;

	/** 第三人称角色开火 Montage——Simulated Proxy 的 Manny 全身播放 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Fire")
	TObjectPtr<UAnimMontage> ThirdPersonCharacterFireMontage;

	/** 第三人称枪械机械动作 Montage——可选，Simulated Proxy 的 TP Weapon Mesh 播放；V1 允许为空 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Fire")
	TObjectPtr<UAnimMontage> ThirdPersonWeaponFireMontage;

	/**
	 * 枪口表现 Socket 名——本轮仅服务第三人称单 Mesh 路径，在 TP 武器 Mesh 上生成枪口 VFX/SFX。
	 * 第一人称改用 Presentation Actor 的显式 MuzzlePoint，不使用本字段。
	 * 仅用于视觉/听觉表现：玩法发射源由 FApecoxRangedFireConfig::GameplayFireOriginOffset 决定，
	 * 不依赖视觉 Socket，因此本字段缺失也不影响命中判定。
	 * 默认 "Muzzle"，但资产可能不存在该 Socket——播放前会做存在性检查。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirePresentation")
	FName MuzzleSocketName = FName(TEXT("Muzzle"));

	/** 一次性枪口 Niagara——可为空，为空时跳过枪口特效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirePresentation")
	TObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	/**
	 * Niagara 相对枪口锚点的旋转。RAR 的 NS_IG_MuzzleFlash 以局部侧轴为发射方向，
	 * 原工程 SpawnSystemAttached 使用 Yaw=90；不能用零旋转替代。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirePresentation")
	FRotator MuzzleEffectRotation = FRotator(0.0f, 90.0f, 0.0f);

	/** 一次性开火声音——可为空，为空时跳过开火音效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirePresentation")
	TObjectPtr<USoundBase> FireSound;

	/** 本地第一人称武器表现成功创建并附着后播放一次；纯刷新或远端TP表现不播放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Equip")
	TObjectPtr<USoundBase> EquipSound;

	/** 第一人称 Arms 普通装备 Montage；与枪械 Unholster Montage 成对播放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Equip")
	TObjectPtr<UAnimMontage> FirstPersonArmsEquipMontage;

	/** 第一人称枪械 Unholster Montage；必须与 Arms Equip Montage 使用相同动作时长。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Equip")
	TObjectPtr<UAnimMontage> FirstPersonWeaponEquipMontage;

	/** 开火、ADS或姿态切换打断装备表现时的混合时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Equip",
		meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
	float EquipInterruptBlendOutTime = 0.1f;

	// ========================================================================
	// ADS 表现——默认值来自 RAR 的 Default 铁瞄设置
	// ========================================================================

	/** 该武器是否允许按住式 ADS。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim")
	bool bSupportsAim = true;

	/** ADS 相机 FOV = 进入游戏时实际相机 FOV * 此倍率。RAR Default 铁瞄为 0.75。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim",
		meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float AimCameraFieldOfViewMultiplier = 0.75f;

	/** UE 第一人称模型独立 FOV 的 ADS 目标。RAR Default 铁瞄为 100。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim",
		meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float AimFirstPersonFieldOfView = 100.0f;

	/** ADS 鼠标水平/垂直灵敏度倍率。RAR Default 铁瞄均为 0.5。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float AimYawSensitivityMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float AimPitchSensitivityMultiplier = 0.5f;

	/** 从腰射到 ADS、以及退出 ADS 的表现混合时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Aim",
		meta = (ClampMin = "0.0", ClampMax = "2.0", Units = "s"))
	float AimBlendTime = 0.2f;

	// ========================================================================
	// 第一人称检视表现
	// ========================================================================

	/** 第一人称 Arms 检视 Montage；与枪械检视 Montage 成对播放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Inspect")
	TObjectPtr<UAnimMontage> FirstPersonArmsInspectMontage;

	/** 第一人称枪械检视 Montage；必须与 Arms 检视 Montage 使用相同动作时长。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Inspect")
	TObjectPtr<UAnimMontage> FirstPersonWeaponInspectMontage;

	/** 检视开始时立即播放并附着到第一人称枪械；中断检视时同步停止。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Inspect")
	TObjectPtr<USoundBase> InspectSound;

	/** 被开火、瞄准或冲刺打断时的 Montage 停止混合时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Inspect",
		meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
	float InspectInterruptBlendOutTime = 0.1f;

	// ========================================================================
	// 第一人称换弹与空仓反馈
	// ========================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<UAnimMontage> FirstPersonArmsReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<UAnimMontage> FirstPersonWeaponReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<UAnimMontage> FirstPersonArmsEmptyReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<UAnimMontage> FirstPersonWeaponEmptyReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<USoundBase> EmptyReloadSound;

	/** 空仓扣扳机只有 Arms 动作；RAR 的 Weapon Montage 行为空。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<UAnimMontage> FirstPersonArmsEmptyFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload")
	TObjectPtr<USoundBase> EmptyFireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload",
		meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
	float ReloadInterruptBlendOutTime = 0.1f;

	/** 第三人称角色换弹 Montage；由公开复制的换弹表现状态驱动，不依赖 OwnerOnly 武器实例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Reload")
	TObjectPtr<UAnimMontage> ThirdPersonCharacterReloadMontage;

	/** 第三人称枪械换弹 Montage；必须与 ThirdPersonWeaponMesh 使用同一 Skeleton。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Reload")
	TObjectPtr<UAnimMontage> ThirdPersonWeaponReloadMontage;

	/** RAR 普通换弹中，旧固定弹匣完全离开枪槽的时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload|Magazine",
		meta = (ClampMin = "0.0", Units = "s"))
	float TacticalReloadHideLoadedMagazineTime = 1.816144f;

	/** RAR 普通换弹中，新弹匣完成插入、恢复固定弹匣并隐藏备用弹匣的时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload|Magazine",
		meta = (ClampMin = "0.0", Units = "s"))
	float TacticalReloadRestoreMagazineTime = 2.266666f;

	/** RAR 空仓换弹中，旧固定弹匣离开枪槽的时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload|Magazine",
		meta = (ClampMin = "0.0", Units = "s"))
	float EmptyReloadHideLoadedMagazineTime = 0.833333f;

	/** RAR 空仓换弹中，新弹匣完成插入、恢复固定弹匣并隐藏备用弹匣的时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Reload|Magazine",
		meta = (ClampMin = "0.0", Units = "s"))
	float EmptyReloadRestoreMagazineTime = 2.533333f;

	// ========================================================================
	// 镭射表现
	// ========================================================================

	/** 是否为该武器启用可切换镭射；资产不完整时运行时安全关闭。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Laser")
	bool bSupportsLaser = false;

	/** 安装在武器骨架 SOCKET_Laser 上的 RAR 镭射实体挂件。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	TObjectPtr<UStaticMesh> LaserAttachmentMesh;

	/** 从挂件自身 SOCKET_Laser 向前缩放的光束 Mesh。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	TObjectPtr<UStaticMesh> LaserBeamMesh;

	/** 命中世界表面时使用的光点 Decal 材质。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	TObjectPtr<UMaterialInterface> LaserDotMaterial;

	/** 用户成功切换开/关时播放一次；自动因冲刺等状态隐藏/恢复时不播放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	TObjectPtr<USoundBase> LaserToggleSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	FName LaserAttachmentSocketName = FName(TEXT("SOCKET_Laser"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	FName LaserEmitterSocketName = FName(TEXT("SOCKET_Laser"));

	/**
	 * 第三人称 Lyra 步枪没有 RAR 的 SOCKET_Laser；使用自身 Muzzle Socket 作为一次性定位锚点，
	 * 再用下面的相对变换把 RAR 镭射挂件移回护木侧面，随后转挂稳定 Root。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Laser")
	FName ThirdPersonLaserAttachmentSocketName = FName(TEXT("Muzzle"));

	/**
	 * 用 Muzzle 完成初始摆放后，重新挂到这个稳定骨骼，避免 Barrel/Muzzle 的开火机械动画抖动挂件。
	 * 当前 Lyra SK_Rifle 的 Root 不参与 Bolt/Barrel 机械动作。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Laser")
	FName ThirdPersonLaserStableAttachmentName = FName(TEXT("Root"));

	/** 第三人称镭射挂件相对上述 Socket 的微调；默认值适配当前 Lyra SK_Rifle。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ThirdPerson|Laser")
	FTransform ThirdPersonLaserAttachmentTransform = FTransform(
		FRotator(0.0f, -90.0f, 0.0f), FVector(-35.0f, 3.5f, -2.0f), FVector::OneVector);

	/** RAR Beam/Dot均使用10000厘米的Visibility Trace。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser",
		meta = (ClampMin = "1.0", Units = "cm"))
	float LaserMaxDistance = 10000.0f;

	/** SM_IG_Laser_Beam沿局部X轴的实际长度约20.024厘米，用于距离到Scale X换算。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser",
		meta = (ClampMin = "0.01", Units = "cm"))
	float LaserBeamMeshLength = 20.024246f;

	/** RAR步枪Lasersight子类把Beam Thickness覆盖为5。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser",
		meta = (ClampMin = "0.01"))
	float LaserBeamThickness = 5.0f;

	/** RAR步枪光点缩放：Base 5 + 命中距离 * 0.002。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser",
		meta = (ClampMin = "0.01"))
	float LaserDotSizeBase = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser",
		meta = (ClampMin = "0.0"))
	float LaserDotSizeMultiplier = 0.002f;

	/** RAR使用TraceTypeQuery1，即工程默认Visibility通道。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FirstPerson|Laser")
	TEnumAsByte<ECollisionChannel> LaserTraceChannel = ECC_Visibility;

	// ========================================================================
	// 动画
	// ========================================================================

	/**
	 * 装备后角色与 FP Arms 应使用的动画族。
	 * 默认 Unarmed 是安全中性值；后续用户在 DA_WeaponPresentation_Rifle 中手工设置为 Rifle。
	 * 这是表现层有限选择，不放入 WeaponInstance / FireConfig / EquipmentComponent / GameplayAbility。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	EApecoxCharacterAnimationFamily EquippedAnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;
};
