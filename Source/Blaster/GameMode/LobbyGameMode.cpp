// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumOfPlayers = GameState.Get()->PlayerArray.Num();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
		                                 FString::Printf(TEXT("Num of Players is %d"), NumOfPlayers));
	if (NumOfPlayers == 2)
	{
		if (UWorld* World = GetWorld())
		{
			// 使用无缝过渡
			bUseSeamlessTravel = true;
			if (World->ServerTravel(FString("/Game/Maps/BlasterMap?listen")))
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
					                                 FString(TEXT("Server travel success")));
			}
			else
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
													 FString(TEXT("Server travel failed")));
			}
		}
	}
}
