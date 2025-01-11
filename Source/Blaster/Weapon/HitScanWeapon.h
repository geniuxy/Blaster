// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

	virtual FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	UPROPERTY(EditAnywhere)
	float Damage = 10.f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticle; // 命中点特效

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticle; // 手枪弹道特效

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

	/**
	 * Trace end with scatter
	 */

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float DistanceToSphere = 800.f; // 散弹枪最远射程

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float SphereRadius = 75.f; // 散弹枪散射半径

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	bool bUseScatter = false;

private:
};
