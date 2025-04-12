// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "TimerManager.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/HUD/OverHeadWidget.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 其他角色不应该挡住摄像头
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 300.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// 附着的物体+附着的位置
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	// bOrientRotationToMovement 设置为 true 时，角色的旋转将会根据其移动方向自动进行适应
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	// 在蓝图类中允许crouch
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// 设置转身速率
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// 设置net更新频率
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Component"));
	AttachGrenade->SetupAttachment(GetMesh(), FName(TEXT("GrenadeSocket")));
	AttachGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// void ABlasterCharacter::OnRep_ReplicatedMovement()
// {
// 	Super::OnRep_ReplicatedMovement();
// 	// 为什么要有这个方法呢？ 就是因为对于ROLE_SimulatedProxy类型的角色，会出现模型抖动
// 	// 为什么会出现模型抖动呢？ 是因为动画蓝图中对于Rotate Boon操作的更新频率小于Tick的频率，会导致抖动
// 	SimProxiesTurn();
// 	TimeSinceLastMovementReplication = 0.f;
// }

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
		ElimBotComponent->DestroyComponent();

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
		Combat->EquippedWeapon->Destroy();
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();

	BlasterPlayerController = BlasterPlayerController == nullptr
		                          ? Cast<ABlasterPlayerController>(Controller)
		                          : BlasterPlayerController;
	if (BlasterPlayerController)
		BlasterPlayerController->HideHUDDeathMessage();
	UpdateHUDHealth();
	UpdateHUDShield();
	if (HasAuthority())
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	if (AttachGrenade)
		AttachGrenade->SetVisibility(false);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 以下是91节的内容，缺点是ROLE_SimulatedProxy的角色会与本地的角色旋转不一样
	// 如果要取消的话，直接把下面的这段内容只留下AimOffset(DeltaTime);即可 (同时记得把anim蓝图里对应的内容改掉)
	// if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	RotateInplace(DeltaTime);
	// else
	// {
	// 	TimeSinceLastMovementReplication += DeltaTime;
	// 	if (TimeSinceLastMovementReplication > 0.25f)
	// 		OnRep_ReplicatedMovement();
	// 	CarculateAO_Pitch();
	// }

	HideCameraIfCharacterClose();
	PollInit();
}

void ABlasterCharacter::RotateInplace(float DeltaTime)
{
	if (bDisableGamePlay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	AimOffset(DeltaTime);
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Released, this, &ThisClass::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ThisClass::ThrowGrenadeButtonPressed);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 只有控制这个角色的客户端会接收到这个属性的更新
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, CurrentHealth);
	DOREPLIFETIME(ABlasterCharacter, CurrentShield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGamePlay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
		Combat->PlayerCharacter = this;
	if (Buff)
	{
		Buff->BlasterCharacter = this;
		Buff->SetInitSpeed(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireMontage)
	{
		AnimInstance->Montage_Play(FireMontage);
		FName SectionName = bAiming ? FName("Aim") : FName("Hip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
		AnimInstance->Montage_Play(ElimMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Sniper");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("Grenade");
			break;
		default: ;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReatMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
                                      class AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return;
	float DamageToHealth = Damage;
	if (CurrentShield > 0.f)
	{
		if (CurrentShield >= Damage)
		{
			DamageToHealth = 0.f;
			CurrentShield = FMath::Clamp(CurrentShield - Damage, 0.f, MaxShield);
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - CurrentShield, 0.f, Damage);
			CurrentShield = 0.f;
		}
	}
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	if (!bElimmed)
		PlayHitReatMontage();

	if (CurrentHealth == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr
				                          ? Cast<ABlasterPlayerController>(Controller)
				                          : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::MoveForward(float value)
{
	if (bDisableGamePlay) return;
	if (Controller != nullptr && value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::MoveRight(float value)
{
	if (bDisableGamePlay) return;
	if (Controller != nullptr && value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::Turn(float value)
{
	AddControllerYawInput(value);
}

void ABlasterCharacter::LookUp(float value)
{
	AddControllerPitchInput(value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGamePlay) return;
	if (Combat)
	{
		if (HasAuthority())
			Combat->EquipWeapon(OverlappingWeapon);
		else
			// 如果不是server服务端的话，需要调用RPC方法
			ServerEquipButtonPressed();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGamePlay) return;
	if (bIsCrouched)
		UnCrouch();
	Crouch();
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->SetAiming(true);
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->SetAiming(false);
}

float ABlasterCharacter::CarculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	float Speed = CarculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing, not jumping
	{
		// bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
			InterpAO_Yaw = AO_Yaw;
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // walking, or jumping
	{
		// bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CarculateAO_Pitch();
}

void ABlasterCharacter::CarculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// 因为UE5会在网络传递值时，将-90~0的值转换为270~360，因此有问题，我们得转回来
		FVector2d InRange(270.f, 360.f);
		FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;
	float Speed = CarculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (ProxyYaw > TurnThreshold)
		TurningInPlace = ETurningInPlace::ETIP_Right;
	else if (ProxyYaw < -TurnThreshold)
		TurningInPlace = ETurningInPlace::ETIP_Left;
	else
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if (bDisableGamePlay) return;
	if (bIsCrouched)
		UnCrouch();
	else
		Super::Jump();
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->FireButtonPressed(false);
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->Reload();
}

void ABlasterCharacter::ThrowGrenadeButtonPressed()
{
	if (bDisableGamePlay) return;
	if (Combat)
		Combat->ThrowGrenade();
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
		Combat->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
		OverlappingWeapon->ShowPickUpWidget(true);
	// 用于客户端人物离开时，隐藏PickupWidget
	if (LastWeapon)
		LastWeapon->ShowPickUpWidget(false);
}


void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
		TurningInPlace = ETurningInPlace::ETIP_Right;
	else if (AO_Yaw < -90.f)
		TurningInPlace = ETurningInPlace::ETIP_Left;

	// 通过插值，完成转向动作
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		// 将人物和武器的mesh都设置为不可见
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
	}
	else
	{
		// 将人物和武器的mesh都设置为可见
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	// 这里的Controller 是每个Character自带的成员变量Controller
	BlasterPlayerController =
		BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
		BlasterPlayerController->SetHUDHealth(CurrentHealth, MaxHealth);
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController =
		BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
		BlasterPlayerController->SetHUDShield(CurrentShield, MaxShield);
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController =
		BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && Combat)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(Combat->StartingWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		Combat->EquipWeapon(StartingWeapon);
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
			BlasterPlayerState->SetKilledBy("");
		}
	}
}

void ABlasterCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		if (Combat->EquippedWeapon->bDestroyWeapon)
			Combat->EquippedWeapon->Destroy();
		else
			Combat->EquippedWeapon->Dropped();
	}

	MulticastElim();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->SetHUDCarriedAmmo(0);
		BlasterPlayerController->SetHUDCarriedWeaponType(EWeaponType::EWS_Max);
	}

	bElimmed = true;
	PlayElimMontage();

	// start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.6f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// disable character movement
	bDisableGamePlay = false;
	// 停止移动
	GetCharacterMovement()->DisableMovement();
	if (Combat)
		Combat->FireButtonPressed(false);
	// GetCharacterMovement()->DisableMovement();
	// // 停止转向
	// GetCharacterMovement()->StopMovementImmediately();
	// // 停止输入(防止开火)
	// if (BlasterPlayerController)
	// 	DisableInput(BlasterPlayerController);

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		UGameplayStatics::SpawnEmitterAtLocation(this, ElimBotEffect, ElimBotSpawnPoint);
	}
	if (ElimBotSound)
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());

	bool bSniperAiming = IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bSniperAiming)
		ShowSniperScopeWidget(false);
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
		BlasterGameMode->RequestRespawn(this, Controller);
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (!bElimmed && CurrentHealth < LastHealth)
		PlayHitReatMontage();
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (!bElimmed && CurrentShield < LastShield)
		PlayHitReatMontage();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// 如果OverlappingWeapon有值，则说明存在lastWeapon
	// 用于服务器端人物离开时，隐藏PickupWidget
	if (OverlappingWeapon)
		OverlappingWeapon->ShowPickUpWidget(false);

	OverlappingWeapon = Weapon;
	// 用于服务器端显示PickUpWidget
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
			OverlappingWeapon->ShowPickUpWidget(true);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming()
{
	return Combat && Combat->bAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_DefaultMax;
	return Combat->CombatState;
}
