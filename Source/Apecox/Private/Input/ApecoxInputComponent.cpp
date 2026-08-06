// Copyright Apecox. All Rights Reserved.

#include "Input/ApecoxInputComponent.h"

void UApecoxInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (const uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Empty();
}
