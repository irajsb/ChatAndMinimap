// Copyright 2024 Iraj Mohtasham aurelion.net 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "MapPOI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWidgetUpdate,UUserWidget* ,Widget);
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHATSYSTEM_API UMapPOI : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMapPOI();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // Add this EndPlay function
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Widget to show on map
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MapPOI")
	TSubclassOf<UUserWidget> WidgetClass;
	// This will remain on corner of the maps if out of bounds instead of hiding it 
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MapPOI")
	bool ShowEvenIfOutOfBounds=false;
	//Alignment of widget (center point , like in canvas alignemnt)
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MapPOI")
	FVector2D WidgetAlignment=FVector2D(0.5f,1.f);
	//Size of widget to apply to out of bounds calculation 
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MapPOI")
	FVector2D OutOfBoundsSize=FVector2D(60,60);
	//Size to widget size
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MapPOI")
	bool AutoSize=true;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(EditCondition="AutoSize==false"),Category="MapPOI")
	FVector2D CustomSize;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category="TitanUMG|MapPOI")
	UUserWidget* GetWidgetFor(UUserWidget* MapWidget);
	UPROPERTY()
	class UPOIManager* POIManager;
public:
	
	//List of widgets owned by this POI (one for each map instance) Key is map widget and value is POI widget
	UPROPERTY()
	TMap<UUserWidget*,UUserWidget*>  POIWidgets;
	UPROPERTY(BlueprintAssignable)
	FOnWidgetUpdate OnWidgetUpdate;
	UFUNCTION(BlueprintCallable,Category="TitanUMG|MapPOI")
	void CallOnWidgetUpdate(UUserWidget* InWidget);
};
