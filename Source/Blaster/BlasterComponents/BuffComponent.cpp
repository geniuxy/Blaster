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
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
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

void UBuffComponent::IncreaseSpeed(float BaseBuffSpeed, float CrouchBuffSpeed, float SpeedBuffTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCharacterMovement() == nullptr) return;

	BlasterCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeed, SpeedBuffTime);

	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseBuffSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchBuffSpeed;
	MulticastSetSpeed(BaseBuffSpeed, CrouchBuffSpeed);
}

void UBuffComponent::SetInitSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitBaseSpeed = BaseSpeed;
	InitCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::ResetSpeed()
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = InitBaseSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitCrouchSpeed;
	MulticastSetSpeed(InitBaseSpeed, InitCrouchSpeed);
}

void UBuffComponent::MulticastSetSpeed_Implementation(float BaseSpeed, float CrouchSpeed)
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}
