// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "RocketMovementComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"

#define ROCKET_SPEED 1500.f

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent =
		CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true); // 指定在网上是否被复制
	RocketMovementComponent->InitialSpeed = ROCKET_SPEED;
	RocketMovementComponent->MaxSpeed = ROCKET_SPEED;
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	// 仅在服务端执行OnHit函数
	if (!HasAuthority())
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

	SpawnTrailSystem();

	if (RocketLoop && RocketLoopAttenuation)
	{
		RocketLoopComponent = UGameplayStatics::SpawnSoundAttached(
			RocketLoop, GetRootComponent(), FName(), GetActorLocation(),
			EAttachLocation::KeepWorldPosition, false, 1.f,
			1.f, 0, RocketLoopAttenuation, (USoundConcurrency*)nullptr, false);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return;

	ExplodeDamage();
	
	StartDestroyTimer();

	if (ImpactParticle)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorLocation());
	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	if (CollisionBox)
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (ProjectileMesh)
		ProjectileMesh->SetVisibility(false);
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	if (RocketLoopComponent && RocketLoopComponent->IsPlaying())
		RocketLoopComponent->Stop();
}

void AProjectileRocket::Destroyed()
{
}
