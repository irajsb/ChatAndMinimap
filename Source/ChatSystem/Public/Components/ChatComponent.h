// Copyright 2024 Iraj Mohtasham aurelion.net 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerState.h"
#include "ChatComponent.generated.h"
class APingActor;
DECLARE_LOG_CATEGORY_EXTERN(LogChatSystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnReciveMessage, const FString&, PlayerName, uint8, SenderTeamIndex,
                                              uint8, MyTeamIndex, const FString&, Message);

/*Class to handle a replicated chat system
 * Needs to be attached to a player controller for correct replication
 * Chat widget must be added after this component is initialized  
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHATSYSTEM_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UChatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
public:
	// Key to open chat. you can also handle this manually
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	FKey ChatOpenKey = EKeys::Enter;
	// Key to open all chat. you can also handle this manually
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	FKey AllChatOpenKey = EKeys::Enter;
	//Should combine key with shift (like lol)
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	bool MustHoldShiftForAllChatKey = true;
	//If player name is not valid reject the message
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	bool RejectMessagesWithNoPlayerName;
	//Do players have authority over their names or should server be only one to be able the change names
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	bool AllowClientToChangeName = false;
	/*
	*1-254 Team Index
	*255 =No Team */

	UFUNCTION(BlueprintCallable, Server, Reliable, Category="TitanUMG|ChatSystem")
	void SetTeamIndexOnServer(uint8 Index);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	uint8 GetTeamIndex() const;
	//Send message to team members
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void SendString(const FString& Input, bool SendToAll);
	//Send message to all

	//
	UFUNCTION(Server, Reliable)
	void SendStringOnServer(const FString& Input);
	UFUNCTION(Server, Reliable)
	void SendToAllStringToServer(const FString& Input);


	UFUNCTION(Server, Reliable)
	void SetPlayerNameOnServer(const FString& Input);
	/*To be called from server. Player name is stored in player state and is usually is passed by online subsystem*/
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void SetPlayerName(const FString& Input);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	const FString& GetPlayerName() const;

	void NotifyMessageReceived(uint8 TeamIndex, const FString& Input, const FString& SenderName);
	UFUNCTION(Client, Reliable)
	void NotifyMessageReceivedOnClient(uint8 TeamIndex, const FString& Input, const FString& SenderName);


	//Make A server Announcement Can be called from any chat component
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	void MakeServerAnnouncement(FString Message) const;
	//Make A server Announcement Can be called from any chat component 
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	void MakeServerAnnouncementToPlayer(FString Message, FString Player) const;
	//Make announcement to a certain team 
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	void MakeServerAnnouncementToTeam(FString Message, uint8 Team) const;
	//Mute is local 
	//Ignores messages from a player
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void MutePlayer(const FString& Player);
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void UnMutePlayer(const FString& Player);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	TArray<FString>& GetMutedPlayers();


	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void SetMuteEnemies(bool Mute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	void BanPlayerFromChatAndPing(const FString& Player);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	void UnBanPlayerFromChatAndPing(const FString& Player);
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category="TitanUMG|ChatSystem")
	TArray<FString>& GetBannedPlayers();

	UPROPERTY(BlueprintAssignable)
	FOnReciveMessage OnReceiveMessage;


private:
	UPROPERTY(Replicated)
	uint8 MyTeamIndex;

	UPROPERTY(Transient)
	TArray<FString> MutedPlayers;
	UPROPERTY(Transient)
	TArray<FString> PingMutedPlayers;
	bool MuteEnemies;
	UPROPERTY(Transient)
	TArray<FString> BannedPlayers;

	UPROPERTY(Replicated)
	FString PlayerName = TEXT("None");

	//Message to show when banned player is attempting to send messages 
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	FString BanMessage = TEXT("</> <Error> You don't have premission to send messages");

	//Optional Server Name
	UPROPERTY(EditAnywhere, Category="ChatComponent")
	FString ServerMessageSenderName;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	static UChatComponent* GetChatComponent(APlayerController* PlayerController);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	static UChatComponent* GetChatComponentFromPlayerState(APlayerState* PlayerState);


	//Ping system

	UPROPERTY(EditAnywhere, Category="ChatComponent")
	float MinTimeBetweenPings;

	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void SpawnPingAtLocation(FVector Location, TSubclassOf<APingActor> PingClass);

	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void SpawnPingAtScreenLocation(TSubclassOf<APingActor> PingClass, FVector2D ScreenLocation,
	                               ETraceTypeQuery TraceChannel, float Distance = 100000000.f);

	UFUNCTION(Server, Reliable)
	void SpawnPingAtLocationServer(FVector Location, TSubclassOf<APingActor> PingClass);


public:
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void MutePlayerPings(const FString& Player);
	UFUNCTION(BlueprintCallable, Category="TitanUMG|ChatSystem")
	void UnMutePlayerPings(const FString& Player);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TitanUMG|ChatSystem")
	const TArray<FString>& GetPingMutedPlayers() const;


private:
	UPROPERTY(Transient)
	float PingTimer;

	UPROPERTY(Transient)
	TArray<AActor*> SpawnedPings;
};
