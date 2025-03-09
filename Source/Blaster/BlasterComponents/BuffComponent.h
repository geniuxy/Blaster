// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Pickups/HealthPickup.h"
#include "Blaster/Pickups/JumpPickup.h"
#include "Blaster/Pickups/ShieldPickup.h"
#include "Blaster/Pickups/SpeedPickup.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();

	// friend class 可以访问所有的protected和private
	friend class ABlasterCharacter;

	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);
	
	void ReplenishShield(float ShieldAmount, float ReplenishShieldTime);
	
	void IncreaseSpeed(float BaseBuffSpeed, float CrouchBuffSpeed, float SpeedBuffTime);
	void SetInitSpeed(float BaseSpeed, float CrouchSpeed);
	
	void BuffJump(float BuffJumpVelocity, float JumpBuffTime);
	void SetInitJumpVelocity(float BaseJumpVelocity);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ABlasterCharacter* BlasterCharacter;

	/** Healing */
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;
	void HealRampUp(float DeltaTime);

	/** Shield */
	bool bShielding = false;
	float ShieldingRate = 0.f;
	float AmountToShield = 0.f;
	void ShieldRampUp(float DeltaTime);

	/** Speed */
	float InitBaseSpeed;
	float InitCrouchSpeed;

	FTimerHandle SpeedBuffTimer;
	void ResetSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetSpeed(float BaseSpeed, float CrouchSpeed);

	/** Jump */
	float InitBaseJumpVelocity;
	
	FTimerHandle JumpBuffTimer;
	void ResetJumpVelocity();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetJumpVelocity(float JumpVelocity);
public:
};
