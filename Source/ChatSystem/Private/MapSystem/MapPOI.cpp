// Copyright 2024 Iraj Mohtasham aurelion.net 


#include "MapSystem/MapPOI.h"

#include "Engine/World.h"
#include "MapSystem/POIManager.h"

// Sets default values for this component's properties
UMapPOI::UMapPOI()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(false);
	// ...
}


// Called when the game starts
void UMapPOI::BeginPlay()
{
	Super::BeginPlay();

	POIManager = GetWorld()->GetSubsystem<UPOIManager>();
	if (POIManager)
	{
		POIManager->AddPoi(this);
	}
}

void UMapPOI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (POIManager)
	{
		POIManager->RemovePoi(this);
	}
	for (auto it = POIWidgets.CreateIterator(); it; ++it)
	{
		it.Value()->RemoveFromParent();
	}
}


// Called every frame
void UMapPOI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

UUserWidget* UMapPOI::GetWidgetFor(UUserWidget* MapWidget)
{
	if(const auto FindResult=POIWidgets.Find(MapWidget))
	{
		return *FindResult;
	}else
	{
		const auto NewWidget=CreateWidget<UUserWidget>(MapWidget, WidgetClass);
		POIWidgets.Add(MapWidget,NewWidget);
		return NewWidget;
	}
	
}

void UMapPOI::CallOnWidgetUpdate(UUserWidget* InWidget)
{
	OnWidgetUpdate.Broadcast(InWidget);
}
