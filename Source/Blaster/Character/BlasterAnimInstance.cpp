// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (BlasterCharacter == nullptr)
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	if (BlasterCharacter == nullptr)
		return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();

	// 这里的YawOffset和Lean不需要Replicated，
	// 是因为GetBaseAimRotation和GetActorRotation这些方法已经做到和server和client上Replicated了
	// Yaw Offset for strafe
	// AimRotator指摄像头camera的方向角度，MovementRotator指运动的角度，0度为一开始目光正向
	FRotator AimRotator = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotator = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DelaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotator, AimRotator);
	// RInterpTo这个插值方法适用于从-180到180这种变化很大的插值算法
	// 如果你正在处理标量值，使用 FInterpTo；如果你正在处理旋转值，使用 RInterpTo。
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DelaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// calculate Lean
	// GetBaseAimRotation() 更多地与角色的“视线”或“瞄准”方向相关
	// GetActorRotation() 返回的旋转是角色的根组件（通常是胶囊体或网格体）的旋转，它决定了角色在世界中的朝向。
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	// 由Target插值(Interp)计算出Lean，第四个参数InterpSpeed越大，插值变化得越快
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// 搭配蓝图中逆运动学fabrik将左手放到lefthandsocket位置
		// 这里只为得到lefthandsocket的位置
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(),
		                                                  FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasterCharacter->IsLocallyControlled())
		{
			bIsLocallyControlled = true;
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget())
			);
			//RInterpTo用于FRotator类型，即旋转插值；而FInterpTo用于float类型，即单一浮点数值的插值。
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 20.f);

			// // 显示枪管法线、枪管到准星的连线
			// FTransform MuzzleTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(
			// 	FName("MuzzleFlash"), RTS_World);
			// FVector MuzzleX(FRotationMatrix(MuzzleTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			// DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), MuzzleTransform.GetLocation() + MuzzleX * 1000.f,
			//               FColor::Red);
			// DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Blue);
		}
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	if (BlasterCharacter->IsLocallyControlled() && BlasterCharacter->GetCombatState() !=
		ECombatState::ECS_ThrowingGrenade)
	{
		bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
	}
	bUseAimOffset =
		BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->bDisableGamePlay;
	bTransformRightHand =
		BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->bDisableGamePlay;
}
