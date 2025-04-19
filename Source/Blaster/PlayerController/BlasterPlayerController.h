// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float CurrentHealth, float MaxHealth);
	void SetHUDShield(float CurrentShield, float MaxShield);
	void SetHUDScore(float ScoreAmount);
	void SetHUDDefeats(int32 Defeats);
	void ShowHUDDeathMessage(const FString& KillerName);
	void HideHUDDeathMessage();
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDCarriedWeaponType(EWeaponType WeaponType);
	void SetHUDMatchCountDown(float CountDown);
	void SetHUDAnnouncementCountDown(float CountDown);
	void SetHUDGrenades(int32 Grenades);

	// 这是为了解决server端，玩家重生之后没有更新生命条的bug
	// 当玩家重生以后，Controller会重新连接，这个时候会调用OnPossess
	virtual void OnPossess(APawn* InPawn) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	virtual float GetServerTime();

	virtual void ReceivedPlayer() override;

	void OnMatchStateSet(FName StateOfMatch);

	void HandleMatchHasStarted();

	void HandleMatchCooldown();

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();

	void PollInit();

	void CheckPing(float DeltaSeconds);

	void HighPingWarning();
	void StopHighPingWarning();

	/**
	 * Sync time between client and server
	 */

	UFUNCTION(Server, Reliable)
	void RequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ReportServerTime(float TimeOfClientRequest, float TimeOfServerReceivedClientRequest);

	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category=Time)
	float SyncTimeDeltaFrequency = 5.f;

	float SyncTimeDeltaRunningTime = 0.f;

	void CheckTimeSync(float DeltaSeconds);

	/**
	 * 处理matchstate相关的状态、变量操作
	 */

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientHandleMatchState(float TimeOfMatch, float TimeOfWarmup, float TimeOfCooldown, float StartingTime,
	                            FName StateOfMatch);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	float LevelStartingTime = 0.f;
	uint32 CountDownInt = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bInitializeHealth = false;
	float HUDCurrentHealth;
	float HUDMaxHealth;
	bool bInitializeShield = false;
	float HUDCurrentShield;
	float HUDMaxShield;
	bool bInitializeScore = false;
	float HUDScore;
	bool bInitializeDefeats = false;
	int32 HUDDefeats;
	bool bInitializeGrenades = false;
	int32 HUDGrenades;
	bool bInitializeWeaponAmmo = false;
	int32 HUDWeaponAmmo;
	bool bInitializeCarriedAmmo = false;
	int32 HUDCarriedAmmo;
	bool bInitializeWeaponType = false;
	EWeaponType HUDWeaponType;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f; 
};
