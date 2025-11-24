// Copyright 2024 Iraj Mohtasham aurelion.net 

#include "Components/ChatComponent.h"
#include "EngineUtils.h"
#include "Engine/DemoNetDriver.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "PingSystem/PingActor.h"

// Define a custom log category for the chat system
DEFINE_LOG_CATEGORY(LogChatSystem)

// Constructor for the ChatComponent
UChatComponent::UChatComponent() :
	MinTimeBetweenPings(0.2f)
{
	// Allow this component to tick
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

// Called when the game starts or when spawned
void UChatComponent::BeginPlay()
{
	Super::BeginPlay();

	// If this is the server, set the player's name to the first player's name
	if (GetOwnerRole() == ROLE_Authority)
	{
	
		SetPlayerName(	Cast<APlayerState>(GetOwner())->GetPlayerName());
	}

	// Setup input mappings for chat system
	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("ChatKey", ChatOpenKey));
	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("AllChat", AllChatOpenKey, MustHoldShiftForAllChatKey));
}

// Called every frame
void UChatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Decrement the ping timer each frame
	PingTimer -= DeltaTime;
}

// Get the team index for the player
uint8 UChatComponent::GetTeamIndex() const
{
	return MyTeamIndex;
}

// Send a chat message
void UChatComponent::SendString(const FString& Input, bool SendToAll)
{
	UE_LOG(LogChatSystem, Log, TEXT("Sending String: %s %s"), *Input, SendToAll ? TEXT("Sending To All") : TEXT("Sending To Team"));

	// Check if the input message is empty, and ignore it if so
	if (Input.IsEmpty())
	{
		return;
	}

	// Check if the player name is empty or "None," and handle it based on the setting
	if (PlayerName == TEXT("None") || PlayerName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("No Player Name"));
		if (RejectMessagesWithNoPlayerName)
		{
			// If configured to reject messages with no player name, ignore the message
			return;
		}
	}

	// If this is not the server, forward the message to the server for processing
	if (GetOwnerRole() < ROLE_Authority)
	{
		if (SendToAll)
		{
			SendToAllStringToServer(Input);
		}
		else
		{
			SendStringOnServer(Input);
		}
	}
	else
	{
		// Server-side handling of the message

		// Check if the player is banned and ignore the message if so
		if (BannedPlayers.Find(PlayerName) != INDEX_NONE)
		{
			UE_LOG(LogChatSystem, Log, TEXT("Message from Banned player %s will be ignored. Message: %s"), *PlayerName, *Input);
			// Make a server announcement to the player that they are banned
			MakeServerAnnouncementToPlayer(BanMessage, PlayerName);
			return;
		}

		// Iterate through all player controllers and send the message to relevant players
		for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			UChatComponent* ChatComponent = GetChatComponent(It->Get());
			if (ChatComponent)
			{
				// Check if the message should be sent to this player based on team index or if it's a global message
				if (ChatComponent->MyTeamIndex == MyTeamIndex || SendToAll)
				{
					// Notify the receiving player about the message
					ChatComponent->NotifyMessageReceived(MyTeamIndex, Input, PlayerName);
				}
			}
		}
	}
}

// Send a chat message to all players on the server (called on the server)
void UChatComponent::SendToAllStringToServer_Implementation(const FString& Input)
{
	SendString(Input, true);
}

// Set the player's name (can be called on the server or client)
void UChatComponent::SetPlayerName(const FString& Input)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		if (AllowClientToChangeName)
		{
			// Forward the request to change the player name to the server for processing
			SetPlayerNameOnServer(Input);
		}
		else
		{
			UE_LOG(LogChatSystem, Log, TEXT("Client attempted to set name. Ignoring the request because AllowClientToChangeName is false"))
		}
	}
	else
	{
		// Server-side change of player name
		PlayerName = Input;
	}
}

// Get the player's name
const FString& UChatComponent::GetPlayerName() const
{
	return PlayerName;
}

// Notify that a message has been received (server-side or client-side)
void UChatComponent::NotifyMessageReceived(uint8 TeamIndex, const FString& Input, const FString& SenderName)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		// Server-side handling of received message
		NotifyMessageReceivedOnClient(TeamIndex, Input, SenderName);

		// If this is a client, return after notifying the client, as the server doesn't need to notify itself
		if (GetNetMode() == NM_Client)
		{
			return;
		}
	}

	// If the sender is muted, ignore the message
	if (MutedPlayers.Find(SenderName) != INDEX_NONE || ((MuteEnemies && TeamIndex != MyTeamIndex) && TeamIndex != 255))
	{
		UE_LOG(LogChatSystem, Log, TEXT("Received message from muted player %s. Ignoring the message"), *SenderName);
		return;
	}

	// Broadcast the received message to listeners
	OnReceiveMessage.Broadcast(SenderName, TeamIndex, MyTeamIndex, Input);
}

// Make a server-wide announcement (broadcast the message to all players)
void UChatComponent::MakeServerAnnouncement(const FString Message) const
{
	const TSubclassOf<APlayerController> Class = APlayerController::StaticClass();
	for (TActorIterator<APlayerController> It(GetWorld(), Class); It; ++It)
	{
		UChatComponent* ChatComponent = GetChatComponent(*It);
		if (ChatComponent)
		{
			// 255 is used as the TeamIndex for server-wide announcements
			ChatComponent->NotifyMessageReceived(255, Message, ServerMessageSenderName);
		}
	}
}

// Make a server announcement to a specific player
void UChatComponent::MakeServerAnnouncementToPlayer(const FString Message, const FString Player) const
{
	const TSubclassOf<APlayerController> Class = APlayerController::StaticClass();
	for (TActorIterator<APlayerController> It(GetWorld(), Class); It; ++It)
	{
		UChatComponent* ChatComponent = GetChatComponent(*It);
		if (ChatComponent)
		{
			if (ChatComponent->PlayerName == Player)
			{
				// 255 is used as the TeamIndex for server-wide announcements
				ChatComponent->NotifyMessageReceived(255, Message, ServerMessageSenderName);
			}
		}
	}
}

// Make a server announcement to a specific team
void UChatComponent::MakeServerAnnouncementToTeam(FString Message, uint8 Team) const
{
	const TSubclassOf<APlayerController> Class = APlayerController::StaticClass();
	for (TActorIterator<APlayerController> It(GetWorld(), Class); It; ++It)
	{
		UChatComponent* ChatComponent = GetChatComponent(*It);
		if (ChatComponent)
		{
			if (ChatComponent->MyTeamIndex == Team)
			{
				// 255 is used as the TeamIndex for server-wide announcements
				ChatComponent->NotifyMessageReceived(255, Message, ServerMessageSenderName);
			}
		}
	}
}

// Mute a player (ignore messages from the player)
void UChatComponent::MutePlayer(const FString& Player)
{
	MutedPlayers.AddUnique(Player);
}

// Unmute a player (stop ignoring messages from the player)
void UChatComponent::UnMutePlayer(const FString& Player)
{
	MutedPlayers.Remove(Player);
}

// Get the list of muted players
TArray<FString>& UChatComponent::GetMutedPlayers()
{
	return MutedPlayers;
}

// Set whether to mute messages from enemies
void UChatComponent::SetMuteEnemies(const bool Mute)
{
	MuteEnemies = Mute;
}

// Ban a player from chat and ping system (server-side only)
void UChatComponent::BanPlayerFromChatAndPing(const FString& Player)
{
	UE_LOG(LogChatSystem, Log, TEXT("Banning %s"), *Player);
	BannedPlayers.AddUnique(Player);
}

// Unban a player from chat and ping system (server-side only)
void UChatComponent::UnBanPlayerFromChatAndPing(const FString& Player)
{
	UE_LOG(LogChatSystem, Log, TEXT("UnBanning %s"), *Player);
	BannedPlayers.Remove(Player);
}

// Get the list of banned players
TArray<FString>& UChatComponent::GetBannedPlayers()
{
	return BannedPlayers;
}

// Notify that a message has been received on the client (server-side and client-side)
void UChatComponent::NotifyMessageReceivedOnClient_Implementation(uint8 TeamIndex, const FString& Input,
                                                                  const FString& SenderName)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		// Forward the received message notification to the client-side function
		NotifyMessageReceived(TeamIndex, Input, SenderName);
	}
}

// Get the ChatComponent from a player controller
UChatComponent* UChatComponent::GetChatComponent(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}
	
	return Cast<UChatComponent>(PlayerController->GetPlayerState<APlayerState>()->GetComponentByClass(StaticClass()));
}

UChatComponent* UChatComponent::GetChatComponentFromPlayerState(APlayerState* PlayerState)
{
	if(!PlayerState)
	{
		return nullptr;
	}
	return  Cast<UChatComponent>(PlayerState->GetComponentByClass(StaticClass()));
}

// Set the player's name on the server (called on the server)
void UChatComponent::SetPlayerNameOnServer_Implementation(const FString& Input)
{
	SetPlayerName(Input);
}

// Set the team index on the server (called on the server)
void UChatComponent::SetTeamIndexOnServer_Implementation(const uint8 Index)
{
	UE_LOG(LogChatSystem,Log,TEXT("Player %s joined team %d"),*GetPlayerName(),Index)
	MyTeamIndex = Index;
}

// Send a chat message to a specific player on the server (called on the server)
void UChatComponent::SendStringOnServer_Implementation(const FString& Input)
{
	SendString(Input, false);
}

// Replication function for lifetime properties
void UChatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Replicate the player name and team index
	DOREPLIFETIME(UChatComponent, PlayerName);
	DOREPLIFETIME(UChatComponent, MyTeamIndex);
}

// Spawn a ping marker at a location (server-side and client-side)
void UChatComponent::SpawnPingAtLocation(FVector Location, TSubclassOf<APingActor> PingClass)
{
	// Check if the ping timer allows placing a new ping
	if (PingTimer > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ignoring ping because the timer is not zero"));
		return;
	}

	// Reset the ping timer
	PingTimer = MinTimeBetweenPings;

	// If this is not the server, forward the ping request to the server for processing
	if (GetOwnerRole() != ROLE_Authority)
	{
		SpawnPingAtLocationServer(Location, PingClass);
	}
	else
	{
		// Server-side handling of ping placement

		// Get the number of pings of the specified class already spawned
		int32 NumPingsOfClass = 0;
		for (AActor* Actor : SpawnedPings)
		{
			const APingActor* PingActor = Cast<APingActor>(Actor);
			if (PingActor && PingActor->IsA(PingClass))
			{
				NumPingsOfClass++;
			}
		}

		// Check if the number of pings of the specified class exceeds the maximum allowed
		if (NumPingsOfClass >= Cast<APingActor>(PingClass->ClassDefaultObject)->MaxNumberOfPings)
		{
			// Remove the oldest ping of the specified class to make room for a new one
			for (int32 Index = 0; Index < SpawnedPings.Num(); Index++)
			{
				const APingActor* PingActor = Cast<APingActor>(SpawnedPings[Index]);
				if (PingActor && PingActor->IsA(PingClass))
				{
					// Destroy the oldest ping and remove it from the SpawnedPings array
					Cast<APingActor>(SpawnedPings[Index])->DestroyOnServer();
					SpawnedPings.RemoveAt(Index);
					break;
				}
			}
		}

		// Spawn a new ping actor at the specified location and add it to the SpawnedPings array
		APingActor* Actor = GetWorld()->SpawnActorDeferred<APingActor>(PingClass.Get(), FTransform(Location),
		                                                               GetOwner(), nullptr,
		                                                               ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Actor->OwningPlayerName = PlayerName;
		Actor->TeamIndex = MyTeamIndex;
		UGameplayStatics::FinishSpawningActor(Actor, FTransform(Location));

		SpawnedPings.Add(Actor);
	}
}

// Spawn a ping marker at a screen location (server-side and client-side)
void UChatComponent::SpawnPingAtScreenLocation(TSubclassOf<APingActor> PingClass, FVector2D ScreenLocation,
                                               ETraceTypeQuery TraceChannel, float Distance)
{
	FHitResult HitResult;
	FVector WorldPos, WorldDir;

	// Convert screen location to world space
	UGameplayStatics::DeprojectScreenToWorld(GetWorld()->GetFirstPlayerController(), ScreenLocation, WorldPos, WorldDir);
	const FVector CameraLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);
	FCollisionQueryParams Params;

	// Perform a line trace from the camera to the specified distance to determine the ping location
	GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, CameraLocation + WorldDir * Distance, CollisionChannel);

	if (HitResult.IsValidBlockingHit())
	{
		// Spawn the ping at the hit location
		SpawnPingAtLocation(HitResult.ImpactPoint, PingClass);
	}
}

// Mute a player's pings (ignore their pings)
void UChatComponent::MutePlayerPings(const FString& Player)
{
	PingMutedPlayers.AddUnique(Player);
}

// Unmute a player's pings (stop ignoring their pings)
void UChatComponent::UnMutePlayerPings(const FString& Player)
{
	PingMutedPlayers.Remove(Player);
}

// Get the list of ping-muted players
const TArray<FString>& UChatComponent::GetPingMutedPlayers() const
{
	return PingMutedPlayers;
}

// Spawn a ping marker at a location on the server (called on the server)
void UChatComponent::SpawnPingAtLocationServer_Implementation(FVector Location, TSubclassOf<APingActor> PingClass)
{
	if (!BannedPlayers.Contains(PlayerName))
	{
		SpawnPingAtLocation(Location, PingClass);
	}
}
