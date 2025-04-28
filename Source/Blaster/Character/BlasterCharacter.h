// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairInterface.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Sound/SoundCue.h"
#include "BlasterCharacter.generated.h"

class ABlasterGameMode;
class ABlasterPlayerState;
class UNiagaraComponent;
class UNiagaraSystem;
class UCombatComponent;
class ULagCompensationComponent;
class UBuffComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	/** 
	* Play montages
	*/
	void PlayFireMontage(bool bAiming);
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayHitReatMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapWeaponMontage();

	// virtual void OnRep_ReplicatedMovement() override;
	
	void DropOrDestroyWeapons();
	void DropOrDestroyWeapon(AWeapon* Weapon);

	void Elim(bool bPlayerLeftGame);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	ECombatState GetCombatState() const;
	
	void SpawnDefaultWeapon();

	UPROPERTY(Replicated)
	bool bDisableGamePlay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	virtual void Destroyed() override;

	bool IsLocallyReloading();
	
	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishedSwapping = false;

	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	FOnLeftGame OnLeftGame;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	void SetTeamColor(ETeam Team);

protected:
	virtual void BeginPlay() override;

	void RotateInplace(float DeltaTime);
	void MoveForward(float value);
	void MoveRight(float value);
	void Turn(float value);
	void LookUp(float value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void CarculateAO_Pitch();
	float CarculateSpeed();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	                   class AController* InstigatorController, AActor* DamageCauser);

	// 初始化HUD状态
	// Poll for any relelvant classes and initialize our HUD
	void PollInit();

	/** 
	* Hit boxes used for server-side rewind
	*/

	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

private:
	UPROPERTY()
	ABlasterPlayerController* BlasterPlayerController;
	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;
	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;

	UPROPERTY(VisibleAnywhere, Category= Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category= Camera)
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UWidgetComponent* OverHeadWidget;

	/**
	 * Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachGrenade;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon); // 参数记录的是，参数复制前的旧值LastWeapon

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	ULagCompensationComponent* LagCompensation;

	// RPC client调用server去捡装备
	// 需要指定RPC是否reliable，reliable的话会重复发请求
	// UFUNCTION(Server,Reliable): 因为这个注释，说明这个方法专属于server端
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	 * Animation Montages
	 */

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* SwapWeaponMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	// 用于解决role = ROLE_SimulatedProxy时的抖动问题
	// 但是5.4中抖动问题不明显了 【就当学习了】
	bool bRotateRootBone;
	float TurnThreshold = 1.f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	/**
	 * Player Health
	 */

	UPROPERTY(EditAnywhere, Category= "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category= "Player Stats")
	float CurrentHealth = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/**
	 * Player Shield
	 */
	UPROPERTY(EditAnywhere, Category= "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Shield, EditAnywhere, Category= "Player Stats")
	float CurrentShield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	/**
	 * Elim
	 */

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	bool bLeftGame = false;

	/**
	 * Crown
	 */
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* CrownSystem;

	UPROPERTY()
	UNiagaraComponent* CrownComponent;

	/**
	 * Dissolve Material
	 */

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// 角色溶解过程中的动态MI
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// 角色溶解前的初始MI
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/** 
	* Team colors
	*/

	UPROPERTY(EditAnywhere, Category = Color)
	UMaterialInstance* RedDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Color)
	UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere, Category = Color)
	UMaterialInstance* BlueDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Color)
	UMaterialInstance* BlueMaterial;

	UPROPERTY(EditAnywhere, Category = Color)
	UMaterialInstance* OriginalMaterial;

	/**
	 *  Elim Bot
	 */

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ElimBotSound;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);

	bool IsWeaponEquipped();

	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }

	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	AWeapon* GetEquippedWeapon();

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	FORCEINLINE bool IsElimmed() const { return bElimmed; }

	FORCEINLINE void SetCurrentHealth(float HealthAmount) { CurrentHealth = HealthAmount; }

	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	FORCEINLINE void SetCurrentShield(float ShieldAmount) { CurrentShield = ShieldAmount; }

	FORCEINLINE float GetCurrentShield() const { return CurrentShield; }

	FORCEINLINE float GetMaxShield() const { return MaxShield; }

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }

	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }

	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }

	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	FORCEINLINE UStaticMeshComponent* GetAttachGrenade() const { return AttachGrenade; }
};
