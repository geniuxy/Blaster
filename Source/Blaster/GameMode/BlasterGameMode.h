// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown;
	// Match duration has been reached. Display winner and begin cooldown timer.
}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();

	virtual void Tick(float DeltaSeconds) override;

	virtual void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ABlasterCharacter* ElimCharacter, AController* ElimController);

	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime;

protected:
	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

private:
	float CountDownTime = 0.f;

public:
	// FORCEINLINE float GetCountDownTime() const { return CountDownTime; }
};
