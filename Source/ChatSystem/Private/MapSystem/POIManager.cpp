// Copyright 2024 Iraj Mohtasham aurelion.net 


#include "MapSystem/POIManager.h"

#include "Engine/World.h"
#include "MapSystem/MapPOI.h"

UPOIManager* UPOIManager::GetInstance(UWorld* World)
{

	auto POIManager = UWorld::GetSubsystem<UPOIManager>(World);
	return POIManager;
}

void UPOIManager::AddPoi(UMapPOI* POI)
{
	if (POI)
	{
		POIList.Add(POI);
	}
}

void UPOIManager::RemovePoi(UMapPOI* POI)
{
	POIList.Remove(POI);
}



const TArray<UMapPOI*>& UPOIManager::GetPoiList(const UObject* WorldContextObject) 
{
		return *(reinterpret_cast<const TArray<UMapPOI*>*>(&POIList));
}