// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

#define GRENADE_SPEED 2000.f

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent =
		CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true); // 指定在网上是否被复制
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->InitialSpeed = GRENADE_SPEED;
	ProjectileMovementComponent->MaxSpeed = GRENADE_SPEED;
}

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay();

	StartDestroyTimer();
	SpawnTrailSystem();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
}

void AProjectileGrenade::Destroyed()
{
	ExplodeDamage();
	
	Super::Destroyed();
}