// Copyright Apecox. All Rights Reserved.

#include "Inventory/ApecoxInventoryItemInstance.h"
#include "Inventory/ApecoxInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UApecoxInventoryItemInstance::Initialize(const UApecoxInventoryItemDefinition* InDefinition)
{
	// 只允许初始化一次——防御重复调用
	if (ItemDefinition)
	{
		return;
	}

	if (!ensureMsgf(InDefinition, TEXT("[Apecox] InventoryItemInstance: Initialize called with null definition.")))
	{
		return;
	}

	ItemDefinition = InDefinition;
}

UWorld* UApecoxInventoryItemInstance::GetWorld() const
{
	// CDO 没有 Outer，安全返回 nullptr
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Outer 必须是可以获取世界的 Actor（PlayerController）
		if (const AActor* OuterActor = Cast<AActor>(GetOuter()))
		{
			return OuterActor->GetWorld();
		}
	}
	return nullptr;
}

void UApecoxInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UApecoxInventoryItemInstance, ItemDefinition);
}
