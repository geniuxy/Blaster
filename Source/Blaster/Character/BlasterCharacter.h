// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
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

	void PlayFireMontage(bool bAiming);

	void PlayElimMontage();

	void PlayReloadMontage();

	void PlayHitReatMontage();

	void PlayThrowGrenadeMontage();

	// virtual void OnRep_ReplicatedMovement() override;

	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGamePlay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

protected:
	UFUNCTION()
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
	void UpdateHUDHealth();

	// 初始化HUD状态
	// Poll for any relelvant classes and initialize our HUD
	void PollInit();

private:
	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

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
	void OnRep_Health();

	/**
	 * Elim
	 */

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

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
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

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

	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	ECombatState GetCombatState() const;

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }

	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	FORCEINLINE UStaticMeshComponent* GetAttachGrenade() const { return AttachGrenade; }
};
