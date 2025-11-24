// Copyright 2024 Iraj Mohtasham aurelion.net 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/NoExportTypes.h"
#include "POIManager.generated.h"

/**
 * 
 */
UCLASS()
class CHATSYSTEM_API UPOIManager : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static UPOIManager* GetInstance(UWorld* World);

	void AddPoi(class UMapPOI* POI);
	void RemovePoi(class UMapPOI* POI);
	

	UFUNCTION(BlueprintCallable,BlueprintPure,Category="TitanUMG|POI",meta=(WorldContext="WorldContextObject"))
	 const TArray<class UMapPOI*>& GetPoiList(const UObject* WorldContextObject) ;
private:
	UPROPERTY()
	TArray<UMapPOI*>POIList;
};
