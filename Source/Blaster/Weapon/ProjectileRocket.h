// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "NiagaraFunctionLibrary.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere)
	USoundCue* RocketLoop;

	UPROPERTY()
	UAudioComponent* RocketLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* RocketLoopAttenuation;

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;

private:
};
