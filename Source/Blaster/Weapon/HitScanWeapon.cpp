// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"
#include "Particles/ParticleSystemComponent.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (InstigatorPawn == nullptr) return;
	AController* InstigatorController = InstigatorPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	// 先判断一下武器的socket是否存在
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		if (HitCharacter && HasAuthority() && InstigatorController)
			UGameplayStatics::ApplyDamage(HitCharacter, Damage, InstigatorController, this,
			                              UDamageType::StaticClass());

		if (ImpactParticle)
			UGameplayStatics::SpawnEmitterAtLocation(
				this,
				ImpactParticle,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation() // 按照命中点的法线旋转粒子特效
			);

		if (HitSound)
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);

		if (MuzzleFlash)
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);

		if (FireSound)
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	if (UWorld* World = GetWorld())
	{
		FVector End = bUseScatter
			              ? TraceEndWithScatter(TraceStart, HitTarget)
			              : TraceStart + (HitTarget - TraceStart) * 1.25;
		// 画一条线，看看有没有命中的目标
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECC_Visibility);
		FVector BeamEnd = End;
		// 如果命中了造成伤害
		if (OutHit.bBlockingHit)
			BeamEnd = OutHit.ImpactPoint;
		if (BeamParticle)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World, BeamParticle, TraceStart, FRotator::ZeroRotator, true);
			if (Beam)
				Beam->SetVectorParameter(TEXT("Target"), BeamEnd); // 粒子特效里把Target设置为BeamEnd
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	// DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Purple, true);
	// DrawDebugLine(
	// 	GetWorld(),
	// 	TraceStart,
	// 	FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
	// 	FColor::Cyan,
	// 	true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}
