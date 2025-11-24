// Copyright 2024 Iraj Mohtasham aurelion.net 


#include "PingSystem/PingActor.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Components/ChatComponent.h"
#include "Components/TitanWidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MapSystem/MapPOI.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APingActor::APingActor(): LifeTime(3.f), MaxNumberOfPings(1)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(SphereComponent);


	TitanWidgetComponent = CreateDefaultSubobject<UTitanWidgetComponent>("Widget");
	TitanWidgetComponent->SetupAttachment(RootComponent);
	TitanWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	bReplicates = true;
}

// Called when the game starts or when spawned
void APingActor::BeginPlay()
{
	Super::BeginPlay();

	if (LifeTime > 0 && GetLocalRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda(([&]
		{
			if(!UKismetSystemLibrary::IsValid(this))
			{
				DestroyOnServer();
			}
		})), LifeTime, false);
	}

#if WITH_EDITOR
	FEditorDelegates::PrePIEEnded.AddLambda([&](bool in)
	{
		if (this)
		{
			if (TimerHandle.IsValid())
			{
				TimerHandle.Invalidate();
			}
		}
	});

#endif
}

bool APingActor::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (!IsGlobalPing)
	{
		if (const APlayerController* PC = Cast<APlayerController>(RealViewer))
		{
			if (const UChatComponent* TargetChatComponent = Cast<UChatComponent>(
				PC->GetPlayerState<APlayerState>()->GetComponentByClass(UChatComponent::StaticClass())))
			{
				if (const UChatComponent* OwningChatComponent = GetOwningChatComponent())
				{
					if (OwningChatComponent->GetTeamIndex() != TargetChatComponent->GetTeamIndex() ||
						TargetChatComponent->GetPingMutedPlayers().Contains(OwningChatComponent->GetPlayerName()))
					{
						return false;
					}
				}
			}
		}
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

// Called every frame
void APingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (PingPendingDestroy)
	{
		if (IsPingReadyToDestroy())
		{
			Destroy();
		}
	}
}

bool APingActor::IsPingReadyToDestroy_Implementation()
{
	return true;
}

void APingActor::K2_DestroyActor()
{
	// if owner or authority
	if (GetOwner() == GetWorld()->GetFirstPlayerController()->GetPlayerState<APlayerState>() || GetLocalRole() == ROLE_Authority)
	{
		DestroyOnServer();
	}
}

UChatComponent* APingActor::GetOwningChatComponent() const
{
	if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		return Cast<UChatComponent>(PS->GetComponentByClass(UChatComponent::StaticClass()));
	}
	return nullptr;
}

void APingActor::TornOff()
{
	PingBeginDestroy();
	PingPendingDestroy = true;

	if(AutoDestroyAttachedPOI)
	{
		auto Component=GetComponentByClass(UMapPOI::StaticClass());
		if(Component)
		{
			Component->DestroyComponent();
		}
	}
}


void APingActor::DestroyOnServer_Implementation()
{
	TearOff();
	PingBeginDestroy();
	PingPendingDestroy = true;
	
}


void APingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APingActor, OwningPlayerName);
	DOREPLIFETIME(APingActor, TeamIndex);
}
