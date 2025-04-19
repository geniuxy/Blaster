// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	PollInit();
	CheckPing(DeltaSeconds);
}

void ABlasterPlayerController::SetHUDHealth(float CurrentHealth, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText;
	if (bValid)
	{
		const float HealthPercent = CurrentHealth / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText =
			FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDCurrentHealth = CurrentHealth;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float CurrentShield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar && BlasterHUD->CharacterOverlay->ShieldText;
	if (bValid)
	{
		const float ShieldPercent = CurrentShield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText =
			FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(CurrentShield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDCurrentShield = CurrentShield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float ScoreAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->ScoreAmount;
	if (bValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(ScoreAmount));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = ScoreAmount;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->DefeatsAmount;
	if (bValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::ShowHUDDeathMessage(const FString& KillerName)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->KillerName && BlasterHUD->CharacterOverlay->KillByText;
	if (bValid)
	{
		FString KillerText = FString::Printf(TEXT("%s"), *KillerName);
		BlasterHUD->CharacterOverlay->KillerName->SetText(FText::FromString(KillerText));
		BlasterHUD->CharacterOverlay->KillerName->SetVisibility(ESlateVisibility::Visible);
		BlasterHUD->CharacterOverlay->KillByText->SetVisibility(ESlateVisibility::Visible);
	}
}

void ABlasterPlayerController::HideHUDDeathMessage()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->KillerName && BlasterHUD->CharacterOverlay->KillByText;
	if (bValid)
	{
		BlasterHUD->CharacterOverlay->KillerName->SetText(FText::FromString(""));
		BlasterHUD->CharacterOverlay->KillerName->SetVisibility(ESlateVisibility::Collapsed);
		BlasterHUD->CharacterOverlay->KillByText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->AmmoAmount;
	if (bValid)
	{
		FString AmmoAmountText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoAmountText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bValid)
	{
		FString CarriedAmmoAmountText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoAmountText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedWeaponType(EWeaponType WeaponType)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->CarriedWeaponType;
	if (bValid)
	{
		FString WeaponTypeText;
		switch (WeaponType)
		{
		case EWeaponType::EWT_AssaultRifle:
			WeaponTypeText = FString::Printf(TEXT("突击步枪"));
			break;
		case EWeaponType::EWT_RocketLauncher:
			WeaponTypeText = FString::Printf(TEXT("火箭发射器"));
			break;
		case EWeaponType::EWT_Pistol:
			WeaponTypeText = FString::Printf(TEXT("手枪"));
			break;
		case EWeaponType::EWT_SubmachineGun:
			WeaponTypeText = FString::Printf(TEXT("冲锋枪"));
			break;
		case EWeaponType::EWT_Shotgun:
			WeaponTypeText = FString::Printf(TEXT("霰弹枪"));
			break;
		case EWeaponType::EWT_SniperRifle:
			WeaponTypeText = FString::Printf(TEXT("狙击枪"));
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			WeaponTypeText = FString::Printf(TEXT("榴弹发射器"));
			break;
		case EWeaponType::EWS_Max:
			WeaponTypeText = FString::Printf(TEXT("未装备"));
			break;
		default: ;
		}
		BlasterHUD->CharacterOverlay->CarriedWeaponType->SetText(FText::FromString(WeaponTypeText));
	}
	else
	{
		bInitializeWeaponType = true;
		HUDWeaponType = WeaponType;
	}
}

void ABlasterPlayerController::SetHUDMatchCountDown(float CountDown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountDown && BlasterHUD->CharacterOverlay->CountdownAnimation;
	if (bValid)
	{
		if (CountDown <= 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountDown->SetText(FText());
			return;
		}
		int32 Minute = FMath::FloorToInt(CountDown / 60.f);
		int32 Second = FMath::FloorToInt(CountDown - Minute * 60);
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
		BlasterHUD->CharacterOverlay->MatchCountDown->SetText(FText::FromString(CountDownText));
		if (CountDown <= 10.f)
		{
			const FSlateColor BlinkCountdownColor(FLinearColor::Red);
			BlasterHUD->CharacterOverlay->MatchCountDown->SetColorAndOpacity(BlinkCountdownColor);
			BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->CountdownAnimation, 0.f, 10.f);
		}
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountDown(float CountDown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->Announcement && BlasterHUD->Announcement->WarmupTime;
	if (bValid)
	{
		if (CountDown <= 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minute = FMath::FloorToInt(CountDown / 60.f);
		int32 Second = FMath::FloorToInt(CountDown - Minute * 60);
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountDownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->GrenadeText;
	if (bValid)
	{
		FString CarriedGrenadeText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadeText->SetText(FText::FromString(CarriedGrenadeText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	// 这一步是因为，对于server来说Playcontroller的ServerCheckMatchState在GameMode的BeginPlay之前，因此初始化的值不对
	// 需要在SetHUDTime之前再获取一遍值
	float TimeLeft = 0.f;
	if (HasAuthority())
	{
		ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
	}

	if (MatchState == MatchState::WaitingToStart)
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress)
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown)
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (CountDownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
			SetHUDAnnouncementCountDown(TimeLeft);
		else if (MatchState == MatchState::InProgress)
			SetHUDMatchCountDown(TimeLeft);
	}
	CountDownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDCurrentHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDCurrentShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponType) SetHUDCarriedWeaponType(HUDWeaponType);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombat())
					if (bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
			}
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	HighPingRunningTime += DeltaSeconds;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingAnimation &&
		BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaSeconds;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->
		            PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
	}
}

void ABlasterPlayerController::RequestServerTime_Implementation(float TimeOfClientRequest)
{
	float TimeOfServerReceivedClientRequest = GetWorld()->GetTimeSeconds();
	ReportServerTime(TimeOfClientRequest, TimeOfServerReceivedClientRequest);
}

void ABlasterPlayerController::ReportServerTime_Implementation(float TimeOfClientRequest,
                                                               float TimeOfServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeOfServerReceivedClientRequest + 0.5 * RoundTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	SyncTimeDeltaRunningTime += DeltaSeconds;
	if (IsLocalController() && SyncTimeDeltaRunningTime > SyncTimeDeltaFrequency)
	{
		RequestServerTime(GetWorld()->GetTimeSeconds());
		SyncTimeDeltaRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		MatchTime = GameMode->MatchTime;
		WarmupTime = GameMode->WarmupTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientHandleMatchState(MatchTime, WarmupTime, CooldownTime, LevelStartingTime, MatchState);
	}
}

void ABlasterPlayerController::ClientHandleMatchState_Implementation(float TimeOfMatch, float TimeOfWarmup,
                                                                     float TimeOfCooldown, float StartingTime,
                                                                     FName StateOfMatch)
{
	MatchTime = TimeOfMatch;
	WarmupTime = TimeOfWarmup;
	CooldownTime = TimeOfCooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	// 为了在游戏开始时加载Announcement这个widget
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		BlasterHUD->AddAnnouncement();
}

void ABlasterPlayerController::OnMatchStateSet(FName StateOfMatch)
{
	MatchState = StateOfMatch;

	if (MatchState == MatchState::InProgress)
		HandleMatchHasStarted();
	else if (MatchState == MatchState::Cooldown)
		HandleMatchCooldown();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
		HandleMatchHasStarted();
	else if (MatchState == MatchState::Cooldown)
		HandleMatchCooldown();
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr)
			BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABlasterPlayerController::HandleMatchCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay)
			BlasterHUD->CharacterOverlay->SetVisibility(ESlateVisibility::Hidden);
		bool bValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText;
		if (bValid)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			// 初始化中文的话，需要加上TEXT(...);
			FString AnnouncementText = TEXT("即将开始新一轮的游戏:");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopScoringPlayers = BlasterGameState->TopScoringPlayers;
				FString WinnerText;
				if (TopScoringPlayers.Num() == 0)
					WinnerText = FString(TEXT("没有人获胜！"));
				else if (TopScoringPlayers.Num() == 1)
				{
					if (TopScoringPlayers[0] == BlasterPlayerState)
						WinnerText = FString(TEXT("恭喜你！你是本轮赢家~"));
					else
						WinnerText = FString::Printf(TEXT("胜者：\n%s"), *TopScoringPlayers[0]->GetPlayerName());
				}
				else if (TopScoringPlayers.Num() > 1)
				{
					WinnerText = FString(TEXT("平局获胜：\n"));
					for (auto TopScoringPlayer : TopScoringPlayers)
						WinnerText.Append(FString::Printf(TEXT("%s\n"), *TopScoringPlayer->GetPlayerName()));
				}
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(WinnerText));
			}
		}
	}
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGamePlay = true;
		BlasterCharacter->GetCombat()->FireButtonPressed(false);
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
		RequestServerTime(GetWorld()->GetTimeSeconds());
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 为了在server端阵亡后隐藏死亡信息
	HideHUDDeathMessage();
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
		SetHUDHealth(BlasterCharacter->GetCurrentHealth(), BlasterCharacter->GetMaxHealth());
}
