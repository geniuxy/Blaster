// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

class UTextBlock;
class UVerticalBox;
/**
 * 
 */
UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetElimAnnouncementText(FString AttackerName, FString VictimName);

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* AnnouncementBox;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;
};
