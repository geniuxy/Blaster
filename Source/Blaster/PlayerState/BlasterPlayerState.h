// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

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

private:
	class ABlasterCharacter* Character;
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing= OnRep_KilledBy)
	FString KilledBy;

public:
	FORCEINLINE void SetKilledBy(const FString& KillerName) { KilledBy = KillerName; }
};
