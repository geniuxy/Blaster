// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, KilledBy);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = IsValid(Character) ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = IsValid(Controller) ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
			Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = IsValid(Character) ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = IsValid(Controller) ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
			Controller->SetHUDDefeats(Defeats);
	}
}

void ABlasterPlayerState::OnRep_KilledBy()
{
	UpdateDeathMessage();
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = IsValid(Character) ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = IsValid(Controller) ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
			Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = IsValid(Character) ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = IsValid(Controller) ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
			Controller->SetHUDDefeats(Defeats);
	}
}

void ABlasterPlayerState::ShowDeathMessage(const FString& KillerName)
{
	SetKilledBy(KillerName);
	UpdateDeathMessage();
}

void ABlasterPlayerState::UpdateDeathMessage()
{
	Character = IsValid(Character) ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = IsValid(Controller) ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
			Controller->ShowHUDDeathMessage(KilledBy);
	}
}
