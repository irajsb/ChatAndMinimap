// Copyright 2024 Iraj Mohtasham aurelion.net 

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "PingActor.generated.h"

class UTitanWidgetComponent;
UCLASS(Abstract,Blueprintable)
class CHATSYSTEM_API APingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APingActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Components")
	USphereComponent* SphereComponent;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Components")
	UTitanWidgetComponent* TitanWidgetComponent;
	//Show this ping for all players
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Behavior")
	bool IsGlobalPing;
	//LifeTime . Negative means manual
	UPROPERTY(EditAnywhere,Category="Behavior")
	float LifeTime;
	//Max number of this type of ping. of exceeding the oldest one will be removed 
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Behavior")
	int MaxNumberOfPings;
	// Auto destroys any attached ping on call of ping destruction eliminating any delay .
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Behavior")
	bool AutoDestroyAttachedPOI=true;

	virtual void K2_DestroyActor() override;
	// Set to false to postpone destroy (handling animations etc...)
	UFUNCTION(BlueprintNativeEvent)
	bool IsPingReadyToDestroy();
	UFUNCTION(BlueprintImplementableEvent)
	void PingBeginDestroy();
	//only valid on owning client and server 
	UFUNCTION(BlueprintCallable,BlueprintPure,Category="TitanUMG|PingActor")
	class UChatComponent* GetOwningChatComponent()const;

	
	UFUNCTION(Server,Reliable)
	void DestroyOnServer();
	
	UPROPERTY(Replicated,BlueprintReadOnly,Category="Behavior")
	FString OwningPlayerName;

	UPROPERTY(Replicated,BlueprintReadOnly,Category="Behavior")
	uint8 TeamIndex;

	
	FTimerHandle TimerHandle;

	bool PingPendingDestroy;
	virtual void TornOff() override;
	
};
