// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
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
		if (HitCharacter && InstigatorController)
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || InstigatorPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage) // 不考虑ssr, 只在服务器上考虑伤害
			{
				UGameplayStatics::ApplyDamage(HitCharacter, Damage, InstigatorController, this,
				                              UDamageType::StaticClass());
			}
			// 考虑ssr, 只在服务器上考虑伤害
			if (!HasAuthority() && bUseServerSideRewind)
			{
				BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr
					                        ? Cast<ABlasterCharacter>(InstigatorPawn)
					                        : BlasterOwnerCharacter;
				BlasterOwnerController = BlasterOwnerController == nullptr
					                         ? Cast<ABlasterPlayerController>(InstigatorController)
					                         : BlasterOwnerController;

				if (BlasterOwnerCharacter && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation() &&
					BlasterOwnerCharacter->IsLocallyControlled())
				{
					BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						HitCharacter,
						Start,
						HitTarget,
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime,
						this
					);
				}
			}
		}

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
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25;
		// 画一条线，看看有没有命中的目标
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECC_Visibility);
		FVector BeamEnd = End;
		// 如果命中了造成伤害
		if (OutHit.bBlockingHit)
			BeamEnd = OutHit.ImpactPoint;

		DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Orange, true);

		if (BeamParticle)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World, BeamParticle, TraceStart, FRotator::ZeroRotator, true);
			if (Beam)
				Beam->SetVectorParameter(TEXT("Target"), BeamEnd); // 粒子特效里把Target设置为BeamEnd
		}
	}
}
