// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(*It);
		if (PlayerController)
			PlayerController->OnMatchStateSet(MatchState);
	}
}

void ABlasterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountDownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountDownTime <= 0.f)
			StartMatch();
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountDownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountDownTime <= 0.f)
			SetMatchState(MatchState::Cooldown);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountDownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountDownTime <= 0.f)
			RestartGame();
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerState = AttackerController
		                                     ? Cast<ABlasterPlayerState>(AttackerController->PlayerState)
		                                     : nullptr;
	ABlasterPlayerState* VictimState = VictimController
		                                   ? Cast<ABlasterPlayerState>(VictimController->PlayerState)
		                                   : nullptr;
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	if (AttackerState && AttackerState != VictimState && BlasterGameState)
	{
		AttackerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerState);
	}

	if (AttackerState && VictimState)
	{
		FString AttackerName = AttackerState->GetPlayerName();
		FString KilledNameString = FString::Printf(TEXT("%s"), *AttackerName);
		VictimState->ShowDeathMessage(KilledNameString);
		VictimState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
		ElimmedCharacter->Elim();
}

void ABlasterGameMode::RequestRespawn(ABlasterCharacter* ElimCharacter, AController* ElimController)
{
	if (ElimCharacter)
	{
		ElimCharacter->Reset();
		ElimCharacter->Destroy();
	}
	if (ElimController)
	{
		TArray<AActor*> PlayStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayStarts);
		int32 SelectStart = FMath::RandRange(0, PlayStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimController, PlayStarts[SelectStart]);
	}
}
