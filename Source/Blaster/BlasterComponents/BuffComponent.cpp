// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"

#include "Blaster/Character/BlasterCharacter.h"


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

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}
