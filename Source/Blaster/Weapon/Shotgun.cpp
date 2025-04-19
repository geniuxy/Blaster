// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (InstigatorPawn == nullptr) return;
	AController* InstigatorController = InstigatorPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	// 先判断一下武器的socket是否存在
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumsOfPellets; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (HitCharacter && HasAuthority() && InstigatorController)
			{
				if (HitMap.Contains(HitCharacter)) HitMap[HitCharacter]++;
				else HitMap.Emplace(HitCharacter, 1);
			}

			if (ImpactParticle)
				UGameplayStatics::SpawnEmitterAtLocation(
					this,
					ImpactParticle,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation() // 按照命中点的法线旋转粒子特效
				);

			if (HitSound)
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f, // 霰弹枪 每个子弹声音轻点
					FMath::RandRange(-.5f, .5f) // 音调也轻点 
				);
		}

		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
				UGameplayStatics::ApplyDamage(HitPair.Key, Damage * HitPair.Value,
				                              InstigatorController, this, UDamageType::StaticClass());
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumsOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
	}
}
