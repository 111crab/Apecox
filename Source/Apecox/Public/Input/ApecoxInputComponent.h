// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "ApecoxInputConfig.h"
#include "InputActionValue.h"
#include "ApecoxInputComponent.generated.h"

/**
 * UApecoxInputComponent
 * - 项目级 EnhancedInputComponent，提供模板化的 Ability 输入绑定
 * - Ability 回调签名：void(const FInputActionValue&, FGameplayTag)
 *   InputTag 由 Enhanced Input BindAction 的额外参数机制在 ActionValue 之后传入
 * - Ability IA 与对应 IMC Mapping 的 Trigger 均应为空；C++ 使用
 *   Started（物理按下边沿）+ Completed/Canceled（物理松开边沿）。
 *   Started 精确表示 Digital Action 从未激活到开始激活的一次边沿；
 *   AbilityInputTagPressed 只调用一次，但同时将 Spec 放入 Pressed 与 Held，
 *   直到 Completed/Canceled 才移出 Held。
 * - 瞬发、按住、蓄力、松发等玩法策略属于 GA 流程或每次授予/Spec 级配置，
 *   不固化在 IA 或本组件的触发事件选择中。
 */
UCLASS()
class APECOX_API UApecoxInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	template<class UserClass>
	void BindNativeAction(const UApecoxInputConfig* InputConfig, const FGameplayTag& Tag,
		ETriggerEvent TriggerEvent, UserClass* Object,
		typename FEnhancedInputActionHandlerValueSignature::template TMethodPtr<UserClass> Func)
	{
		if (!InputConfig)
		{
			return;
		}

		if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(Tag))
		{
			BindAction(IA, TriggerEvent, Object, Func);
		}
	}

	/**
	 * 绑定 Ability InputAction。
	 * 回调签名：void(const FInputActionValue&, FGameplayTag)
	 * InputTag 由 BindAction 的额外参数在 ActionValue 之后传入回调。
	 */
	template<class UserClass>
	void BindAbilityActions(const UApecoxInputConfig* InputConfig, UserClass* Object,
		void (UserClass::*PressedFunc)(const FInputActionValue&, FGameplayTag),
		void (UserClass::*ReleasedFunc)(const FInputActionValue&, FGameplayTag),
		TArray<uint32>& BindHandles)
	{
		if (!InputConfig)
		{
			return;
		}

		for (const FApecoxInputAction& Action : InputConfig->AbilityInputActions)
		{
			if (!Action.InputAction || !Action.InputTag.IsValid())
			{
				continue;
			}

			uint32 PressedHandle = BindAction(Action.InputAction, ETriggerEvent::Started,
				Object, PressedFunc, Action.InputTag).GetHandle();
			BindHandles.Add(PressedHandle);

			uint32 ReleasedHandle = BindAction(Action.InputAction, ETriggerEvent::Completed,
				Object, ReleasedFunc, Action.InputTag).GetHandle();
			BindHandles.Add(ReleasedHandle);

			ReleasedHandle = BindAction(Action.InputAction, ETriggerEvent::Canceled,
				Object, ReleasedFunc, Action.InputTag).GetHandle();
			BindHandles.Add(ReleasedHandle);
		}
	}

	void RemoveBinds(TArray<uint32>& BindHandles);
};
