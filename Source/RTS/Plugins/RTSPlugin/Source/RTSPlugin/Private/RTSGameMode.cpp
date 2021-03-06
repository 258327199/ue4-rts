#include "RTSPluginPrivatePCH.h"
#include "RTSGameMode.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

#include "RTSCharacter.h"
#include "RTSPlayerController.h"
#include "RTSPlayerStart.h"
#include "RTSTeamInfo.h"


void ARTSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	AGameModeBase::InitGame(MapName, Options, ErrorMessage);

	// Set up teams.
	if (TeamClass == nullptr)
	{
		TeamClass = ARTSTeamInfo::StaticClass();
	}

	for (uint8 i = 0; i < NumTeams; ++i)
	{
		ARTSTeamInfo* NewTeam = GetWorld()->SpawnActor<ARTSTeamInfo>(TeamClass);
		NewTeam->TeamIndex = i;
		Teams.Add(NewTeam);
	}
}

ARTSPlayerStart* ARTSGameMode::FindRTSPlayerStart(AController* Player)
{
	// Choose a player start.
	TArray<ARTSPlayerStart*> UnOccupiedStartPoints;
	TArray<ARTSPlayerStart*> OccupiedStartPoints;

	for (TActorIterator<ARTSPlayerStart> It(GetWorld()); It; ++It)
	{
		ARTSPlayerStart* PlayerStart = *It;

		if (PlayerStart->Player == nullptr)
		{
			UnOccupiedStartPoints.Add(PlayerStart);
		}
		else
		{
			OccupiedStartPoints.Add(PlayerStart);
		}
	}

	if (UnOccupiedStartPoints.Num() > 0)
	{
		return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
	}
	else if (OccupiedStartPoints.Num() > 0)
	{
		return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
	}

	return nullptr;
}

void ARTSGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	ARTSPlayerStart* StartSpot = FindRTSPlayerStart(NewPlayer);
	RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

void ARTSGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	// Spawn default pawns (most likely, this will be the camera pawn in our case).
	AGameModeBase::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	// Verify parameters.
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if (!StartSpot)
	{
		return;
	}

	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(NewPlayer);

	// Occupy start spot.
	ARTSPlayerStart* PlayerStart = Cast<ARTSPlayerStart>(StartSpot);

	if (PlayerStart)
	{
		UE_LOG(RTSLog, Log, TEXT("Start spot %s is now occupied by player %s."), *PlayerStart->GetName(), *NewPlayer->GetName());
		PlayerStart->Player = NewPlayer;
	}

	// Set team.
	if (PlayerStart->TeamIndex >= Teams.Num())
	{
		UE_LOG(RTSLog, Warning, TEXT("Player start team index is %d, but game only has %d teams."), PlayerStart->TeamIndex, Teams.Num());
	}
	else
	{
		Teams[PlayerStart->TeamIndex]->AddToTeam(NewPlayer);
	}

	// Build spawn transform.
	// Don't allow initial actors to be spawned with any pitch or roll.
	FVector SpawnLocation = StartSpot->GetActorLocation();

	FRotator SpawnRotation(ForceInit);
	SpawnRotation.Yaw = StartSpot->GetActorRotation().Yaw;

	FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);

	// Build spawn info.
	for (TSubclassOf<AActor> ActorClass : InitialActors)
	{
		// Spawn actor.
		SpawnActorForPlayer(ActorClass, PlayerController, SpawnTransform);
	}
}

AActor* ARTSGameMode::SpawnActorForPlayer(TSubclassOf<AActor> ActorClass, ARTSPlayerController* ActorOwner, const FTransform& SpawnTransform)
{
	// Spawn actor.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass->GetDefaultObject()->GetClass(), SpawnTransform, SpawnParams);

	// Set owning player.
	if (SpawnedActor && ActorOwner)
	{
		UE_LOG(RTSLog, Log, TEXT("Spawned %s for player %s at %s."), *SpawnedActor->GetName(), *ActorOwner->GetName(), *SpawnTransform.GetLocation().ToString());

		// Set owning player.
		ActorOwner->TransferOwnership(SpawnedActor);
	}

	return SpawnedActor;
}

void ARTSGameMode::NotifyOnActorKilled(AActor* Actor, AController* ActorOwner)
{
	if (DefeatConditionActor == nullptr)
	{
		return;
	}

	APlayerController* OwningPlayer = Cast<APlayerController>(ActorOwner);

	if (OwningPlayer == nullptr)
	{
		return;
	}

	// Check if any required actors are still alive.
	for (AActor* OwnedActor : OwningPlayer->Children)
	{
		if (OwnedActor->GetClass() == DefeatConditionActor)
		{
			return;
		}
	}

	UE_LOG(RTSLog, Log, TEXT("Player %s does not control any %s anymore and has been defeated."), *OwningPlayer->GetName(), *DefeatConditionActor->GetName());

	// Notify listeners.
	NotifyOnPlayerDefeated(OwningPlayer);
}

void ARTSGameMode::NotifyOnPlayerDefeated(APlayerController* Player)
{
	ReceiveOnPlayerDefeated(Player);
}
