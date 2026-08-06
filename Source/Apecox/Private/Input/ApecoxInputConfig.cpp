// Copyright Apecox. All Rights Reserved.

#include "Input/ApecoxInputConfig.h"

const UInputAction* UApecoxInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FApecoxInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}

const UInputAction* UApecoxInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FApecoxInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}
