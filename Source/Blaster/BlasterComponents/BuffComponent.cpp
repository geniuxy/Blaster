// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHealing)
		HealRampUp(DeltaTime);

	if (bShielding)
		ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishShieldTime)
{
	bShielding = true;
	ShieldingRate = ShieldAmount / ReplenishShieldTime;
	AmountToShield += ShieldAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->IsElimmed()) return;

	const float HealingThisFrame = HealingRate * DeltaTime;
	BlasterCharacter->SetCurrentHealth(
		FMath::Clamp(BlasterCharacter->GetCurrentHealth() + HealingThisFrame, 0.f, BlasterCharacter->GetMaxHealth()));
	BlasterCharacter->UpdateHUDHealth();
	AmountToHeal -= HealingThisFrame;

	if (AmountToHeal <= 0.f || BlasterCharacter->GetCurrentHealth() >= BlasterCharacter->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
		HealingRate = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->IsElimmed()) return;

	const float ShieldingThisFrame = ShieldingRate * DeltaTime;
	BlasterCharacter->SetCurrentShield(
		FMath::Clamp(BlasterCharacter->GetCurrentShield() + ShieldingThisFrame, 0.f, BlasterCharacter->GetMaxShield()));
	BlasterCharacter->UpdateHUDShield();
	AmountToShield -= ShieldingThisFrame;

	if (AmountToShield <= 0.f || BlasterCharacter->GetCurrentShield() >= BlasterCharacter->GetMaxShield())
	{
		bShielding = false;
		AmountToShield = 0.f;
		ShieldingRate = 0.f;
	}
}

void UBuffComponent::IncreaseSpeed(float BaseBuffSpeed, float CrouchBuffSpeed, float SpeedBuffTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr ||
		BlasterCharacter->GetCombat() == nullptr)
		return;

	BlasterCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeed, SpeedBuffTime);

	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseBuffSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchBuffSpeed;
	BlasterCharacter->GetCombat()->SetBaseWalkSpeed(BaseBuffSpeed);
	BlasterCharacter->GetCombat()->SetBaseCrouchSpeed(CrouchBuffSpeed);
	MulticastSetSpeed(BaseBuffSpeed, CrouchBuffSpeed);
}

void UBuffComponent::SetInitSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitBaseSpeed = BaseSpeed;
	InitCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitJumpVelocity(float BaseJumpVelocity)
{
	InitBaseJumpVelocity = BaseJumpVelocity;
}

void UBuffComponent::ResetSpeed()
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr ||
		BlasterCharacter->GetCombat() == nullptr)
		return;

	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = InitBaseSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitCrouchSpeed;
	BlasterCharacter->GetCombat()->SetBaseWalkSpeed(InitBaseSpeed);
	BlasterCharacter->GetCombat()->SetBaseCrouchSpeed(InitCrouchSpeed);
	MulticastSetSpeed(InitBaseSpeed, InitCrouchSpeed);
}

void UBuffComponent::MulticastSetSpeed_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr) return;
	
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float JumpBuffTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr) return;

	BlasterCharacter->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &UBuffComponent::ResetJumpVelocity, JumpBuffTime);

	BlasterCharacter->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	MulticastSetJumpVelocity(BuffJumpVelocity);
}

void UBuffComponent::ResetJumpVelocity()
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr) return;
	
	BlasterCharacter->GetCharacterMovement()->JumpZVelocity = InitBaseJumpVelocity;
	MulticastSetJumpVelocity(InitBaseJumpVelocity);
}

void UBuffComponent::MulticastSetJumpVelocity_Implementation(float JumpVelocity)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr) return;
	
	BlasterCharacter->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}

