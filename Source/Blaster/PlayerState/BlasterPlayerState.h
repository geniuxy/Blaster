// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Replicated notifies
	 */
	virtual void OnRep_Score() override;
	UFUNCTION()
	virtual void OnRep_Defeats();
	UFUNCTION()
	virtual void OnRep_KilledBy();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	
	void ShowDeathMessage(const FString& KillerName);
	void UpdateDeathMessage();

	void SetTeam(ETeam TeamToSet);

private:
	ABlasterCharacter* Character;
	ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing= OnRep_KilledBy)
	FString KilledBy;
	
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE void SetKilledBy(const FString& KillerName) { KilledBy = KillerName; }
};
