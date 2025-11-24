// Copyright 2024 Iraj Mohtasham aurelion.net 


#include "TitanFunctionLibrary.h"

#include "AudioDevice.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/InputSettings.h"

#include "Slate/SlateBrushAsset.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Text/SlateEditableTextLayout.h"




UInputComponent* UTitanFunctionLibrary::CreateAndRegisterNewInputComponent(const UObject* WorldContextObject,APlayerController* InController, bool StopAction, const int32 Priority)
{
	
	if(!InController)
	{
		InController=WorldContextObject->GetWorld()->GetFirstPlayerController();
	}
	if ( InController )
	{
		UInputComponent* InputComponent = NewObject< UInputComponent >( InController, UInputSettings::GetDefaultInputComponentClass(), NAME_None, RF_Transient );
		InputComponent->bBlockInput = StopAction;
		InputComponent->Priority = Priority;
		InController->PushInputComponent( InputComponent );
		return InputComponent;
	}
	
	return nullptr;
}

void UTitanFunctionLibrary::UnregisterInputComponent(const UObject* WorldContextObject,APlayerController* InController,
	UInputComponent* InputComponent)
{
	if(!InController)
	{
		InController=WorldContextObject->GetWorld()->GetFirstPlayerController();
	}
	if(InController&&InputComponent)
	{
		InController->PopInputComponent(InputComponent);
	}
}

void UTitanFunctionLibrary::AddKeyBinding(const UObject* WorldContextObject,FKey Key,TEnumAsByte<EInputEvent>InputEvent, UInputComponent* InputComponent, FOnInputActionWithKey Event)
{
	FInputKeyBinding KB(Key, InputEvent);
	
	KB.KeyDelegate.GetDelegateWithKeyForManualSet().BindUFunction(Event.GetUObject(),Event.GetFunctionName());
	
	InputComponent->KeyBindings.Emplace(MoveTemp(KB));
	
}







 void UTitanFunctionLibrary::GetActionMappings(TArray<FGroupedActionMapping>& GroupedActionMappings)
{
	const UInputSettings* MyInputSettings = UInputSettings::GetInputSettings();
	
	auto Mappings=  MyInputSettings->GetActionMappings();


	for (int32 I=0;I!=Mappings.Num();++I)
	{
		bool Found=false;
		for (int32 J=0;J!=GroupedActionMappings.Num();++J)
		{
			if(GroupedActionMappings[J].ActionName==Mappings[I].ActionName)
			{
				GroupedActionMappings[J].Mappings.Add(Mappings[I]);
				Found=true;
			}
			
		}
		if(!Found)
		{
			GroupedActionMappings.Add(FGroupedActionMapping(Mappings[I].ActionName,Mappings[I]));
		}
	}

	GroupedActionMappings.Sort();
	
}

void UTitanFunctionLibrary::GetAxisMappings(TArray<FGroupedAxisMapping>& GroupedActionMappings)
{
	const UInputSettings* MyInputSettings = UInputSettings::GetInputSettings();
	
	auto Mappings=  MyInputSettings->GetAxisMappings();


	for (int32 I=0;I!=Mappings.Num();++I)
	{
		bool Found=false;
		for (int32 J=0;J!=GroupedActionMappings.Num();++J)
		{
			if(GroupedActionMappings[J].AxisName==Mappings[I].AxisName)
			{
				GroupedActionMappings[J].Mappings.Add(Mappings[I]);
				Found=true;
			}
			
		}
		if(!Found)
		{
			GroupedActionMappings.Add(FGroupedAxisMapping(Mappings[I].AxisName,Mappings[I]));
		}
	}

	GroupedActionMappings.Sort();
}


void UTitanFunctionLibrary::UpdateActionMapping(FName ActName, FKey OldKey, FKey NewKey)
{
	 UInputSettings* Settings = UInputSettings::GetInputSettings();
	if (!Settings) { return; }

	if(OldKey.IsValid())
	{
		Settings->RemoveActionMapping(FInputActionKeyMapping(ActName, OldKey));
	}
	if(NewKey.IsValid())
	{
		Settings->AddActionMapping(FInputActionKeyMapping(ActName,NewKey));
	}else
	{
		Settings->AddActionMapping(FInputActionKeyMapping(ActName));
	}
	
	Settings->SaveKeyMappings();
}



FSlateBrush* UTitanFunctionLibrary::StaticLoadBrush(const FString& InPath)
{
	const auto Object =StaticLoadObject(USlateBrushAsset::StaticClass(), nullptr, *InPath);
	if(Object)
	{
		return &Cast<USlateBrushAsset>(Object)->Brush;
	}
	UE_LOG(LogTemp,Error,TEXT("Static load brush failed %s"),*InPath);
	return FCoreStyle::Get().GetDefaultBrush();
}







