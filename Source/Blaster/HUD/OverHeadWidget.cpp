// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverHeadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
		DisplayText->SetText(FText::FromString(TextToDisplay));
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPwan)
{
	// ENetRole RemoteRole = InPwan->GetRemoteRole();
	// FString Role;
	// switch (RemoteRole)
	// {
	// case ROLE_Authority:
	// 	Role = FString("Authority");
	// 	break;
	// case ROLE_AutonomousProxy:
	// 	Role = FString("Autonomous Proxy");
	// 	break;
	// case ROLE_SimulatedProxy:
	// 	Role = FString("Simulated Proxy");
	// 	break;
	// case ROLE_None:
	// 	Role = FString("None");
	// 	break;
	// }
	// FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	// SetDisplayText(RemoteRoleString);

	APlayerState* PlayerState = InPwan->GetPlayerState();
	FString PlayerName;
	if (PlayerState)
		// 使用GetPlayerName()获取玩家名称
		PlayerName = PlayerState->GetPlayerName();
	FString PlayerNameString = FString::Printf(TEXT("%s"), *PlayerName);
	SetDisplayText(PlayerNameString);
}


void UOverHeadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();
}
