// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"

#define BULLET_SPEED 15000.f

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent =
		CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true); // 指定在网上是否被复制
	ProjectileMovementComponent->InitialSpeed = BULLET_SPEED;
	ProjectileMovementComponent->MaxSpeed = BULLET_SPEED;
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && HasAuthority())
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
	}

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
