// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "ApecoxBotCharacter.generated.h"

/** 可受伤、死亡和重生的 Apecox 人机；战斗属性与玩家角色走同一套 GAS 链路。 */
UCLASS()
class APECOX_API AApecoxBotCharacter : public AApecoxPlayerCharacter
{
	GENERATED_BODY()

public:
	AApecoxBotCharacter();

	/** Server AI input bridge. It reuses the same weapon ability and reload transaction as players. */
	void SetAIWeaponFireActive(bool bActive);
	bool TryStartAIWeaponReload();
	bool IsAIWeaponMagazineEmpty() const;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void RequestRespawnFromGameMode(AController* RespawnController) override;
	void TryEquipBotStartingWeapon();

private:
	/** Authority 记录地图放置/本次生成位置，死亡后在同一点生成新实例。 */
	FTransform BotSpawnTransform = FTransform::Identity;
	bool bAIWeaponFireActive = false;
};
