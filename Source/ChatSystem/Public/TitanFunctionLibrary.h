// Copyright 2024 Iraj Mohtasham aurelion.net

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerInput.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TitanFunctionLibrary.generated.h"

/**
 *
 */

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnInputActionWithKey, FKey, Key);
USTRUCT(BlueprintType)
struct FGroupedActionMapping
{
public:
	GENERATED_BODY()
	/** Friendly name of action, e.g "jump" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName ActionName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<FInputActionKeyMapping> Mappings;

	bool operator==(const FName &Other) const
	{
		return (ActionName == Other);
	}
	bool operator<(const FGroupedActionMapping &Other) const
	{
		return ActionName.LexicalLess(Other.ActionName);
	}

	FGroupedActionMapping(FName InActionName, FInputActionKeyMapping Mapping)
	{
		ActionName = InActionName;
		Mappings.Add(Mapping);
	}
	FGroupedActionMapping()
	{
		ActionName = NAME_None;
	}
};

USTRUCT(BlueprintType)
struct FGroupedAxisMapping
{
public:
	GENERATED_BODY()
	/** Friendly name of action, e.g "jump" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName AxisName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<FInputAxisKeyMapping> Mappings;

	bool operator==(const FName &Other) const
	{
		return (AxisName == Other);
	}
	bool operator<(const FGroupedAxisMapping &Other) const
	{
		return AxisName.LexicalLess(Other.AxisName);
	}

	FGroupedAxisMapping(FName InActionName, FInputAxisKeyMapping Mapping)
	{
		AxisName = InActionName;
		Mappings.Add(Mapping);
	}
	FGroupedAxisMapping()
	{
		AxisName = NAME_None;
	}
};
class FOnInputAction;
UCLASS()
class CHATSYSTEM_API UTitanFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	

	/**
		Create and register an input component
	 *StopAction: Whether any components lower on the input stack should be allowed to receive input.
	 *Priority: input priority*/
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "TitanUMG|CustomInput")
	static UInputComponent *CreateAndRegisterNewInputComponent(const UObject *WorldContextObject, APlayerController *InController, bool StopAction, int32 Priority);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "TitanUMG|CustomInput")
	static void UnregisterInputComponent(const UObject *WorldContextObject, APlayerController *InController, UInputComponent *InputComponent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "TitanUMG|CustomInput")
	static void AddKeyBinding(const UObject *WorldContextObject, FKey Key, TEnumAsByte<EInputEvent> InputEvent, UInputComponent *InputComponent, FOnInputActionWithKey Event);








	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TitanUMG|Input")
	static void GetActionMappings(TArray<FGroupedActionMapping> &GroupedActionMappings);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TitanUMG|Input")
	static void GetAxisMappings(TArray<FGroupedAxisMapping> &GroupedActionMappings);



	UFUNCTION(BlueprintCallable, Category = "TitanUMG|Input")
	static void UpdateActionMapping(FName ActName, FKey OldKey, FKey NewKey);



	static FSlateBrush *StaticLoadBrush(const FString &InPath);

	
};
