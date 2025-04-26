// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent =
		CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true); // 指定在网上是否被复制
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	// GetFName()  ...
	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	// FPredictProjectilePathParams PathParams;
	// PathParams.bTraceWithChannel = true;
	// PathParams.bTraceWithCollision = true;
	// PathParams.DrawDebugTime = 5.f;
	// PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	// PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	// PathParams.MaxSimTime = 4.f;
	// PathParams.ProjectileRadius = 5.f;
	// PathParams.SimFrequency = 30.f;
	// PathParams.StartLocation = GetActorLocation();
	// PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	// PathParams.ActorsToIgnore.Add(this);
	//
	// FPredictProjectilePathResult PathResult;
	//
	// UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);
		if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind) // no ssr
		{
			const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;
			
			UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
			Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
			return;
		}
		ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
		// with ssr
		if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
		{
			OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
				HitCharacter,
				TraceStart,
				InitialVelocity,
				OwnerController->GetServerTime() - OwnerController->SingleTripTime
			);
		}
	}

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
