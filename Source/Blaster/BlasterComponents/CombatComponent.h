// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// friend class 可以访问所有的protected和private
	friend class ABlasterCharacter;
	
	void EquipWeapon(AWeapon* WeaponToEquip);

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReload();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void FireButtonPressed(bool bPressed);

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void UpdateCarriedWeaponType();
	void PlayEquipWeaponSound();
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);
	
	void Fire();
	
	// FVector_NetQuantize 通常用于角色位置、方向或其他需要在网络上同步的向量数据的处理。
	// 是FVector的一个派生（子）类
	// 缩放后的浮点数值四舍五入到最接近的整数值
	// 使用这个函数可以帮助优化网络性能，特别是在处理大量数据或者网络带宽有限的情况下。
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TracerHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TracerHitTarget);

	void TraceUnderCrosshair(FHitResult& TraceHitResult);

	void SetHUDCrosshair(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload(); // 用来处理Reload中server和client都要执行的事务

	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void HandleThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

private:
	// 用于绑定对应的Character(BlasterCharacter)
	UPROPERTY()
	ABlasterCharacter* PlayerCharacter;
	UPROPERTY()
	ABlasterPlayerController* PlayerController;
	UPROPERTY()
	ABlasterHUD* HUD;

	// 只会在server端被赋值，client端都为nullptr
	UPROPERTY(ReplicatedUsing= OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere)
	float BaseCrouchWalkSpeed;

	UPROPERTY(EditAnywhere)
	float CrouchAimWalkSpeed;

	bool bFireButtonPressed;

	/**
	 *	factors of crosshair spread
	 */

	float CrosshairVelocityFactor;
	FVector2D WalkSpeedRange;
	FVector2D VelocityMultiplierRange;

	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/**
	 * Aiming and FOV(field of view)
	 */

	// Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category= Combat)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category= Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/**
	 * Automatic fire
	 */

	FTimerHandle FireTimer;
	bool bCanFire = true;
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	/**
	 * Character Carried Ammo
	 */

	UPROPERTY(ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_AR = 30;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_RL = 10;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_PIS = 20;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_SG = 20;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_Shotgun = 20;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_Snip = 10;

	UPROPERTY(EditAnywhere)
	int32 StartCarriedAmmoAmount_GL = 5;

	void InitializeCarriedAmmoMap();

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	
	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValue();
	
	void UpdateShotgunAmmoValue();
public:
};
