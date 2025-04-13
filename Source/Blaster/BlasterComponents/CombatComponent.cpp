// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 360.f;
	BaseCrouchWalkSpeed = 350.f;
	CrouchAimWalkSpeed = 250.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
		Reload();
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerCharacter)
	{
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BaseCrouchWalkSpeed;

		// 初始化视野 DefaultFOV赋值
		if (PlayerCharacter->GetFollowCamera())
		{
			DefaultFOV = PlayerCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		// 只在server端初始化携带弹药
		if (PlayerCharacter->HasAuthority())
			InitializeCarriedAmmoMap();
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PlayerCharacter && PlayerCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshair(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (PlayerCharacter == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	// 这边是因为不管是server还是client，都可以调用这个方法
	ServerSetAiming(bIsAiming);
	if (PlayerCharacter)
	{
		UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement();
		if (!PlayerCharacter->bIsCrouched)
			MovementComponent->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		else
			MovementComponent->MaxWalkSpeedCrouched = bIsAiming ? CrouchAimWalkSpeed : BaseCrouchWalkSpeed;
	}
	if (PlayerCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
		PlayerCharacter->ShowSniperScopeWidget(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (PlayerCharacter)
	{
		UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement();
		if (!PlayerCharacter->bIsCrouched)
			MovementComponent->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		else
			MovementComponent->MaxWalkSpeedCrouched = bIsAiming ? CrouchAimWalkSpeed : BaseCrouchWalkSpeed;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
		Fire();
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = PlayerCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && PlayerCharacter->GetReloadMontage())
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);
		if (EquippedWeapon)
			CrosshairShootingFactor = 0.75f;
		StartFireTimer();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	bool bFireWhenShotgunReloading = !EquippedWeapon->IsEmpty() && bCanFire && CombatState ==
		ECombatState::ECS_Reloading &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun;
	bool bFireWhenUnoccupied = !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
	return bFireWhenShotgunReloading || bFireWhenUnoccupied;
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || PlayerCharacter == nullptr) return;
	PlayerCharacter->GetWorldTimerManager().SetTimer(
		FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
		Fire();
	// 如果武器没子弹了，自动换弹
	ReloadEmptyWeapon();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TracerHitTarget)
{
	MulticastFire(TracerHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TracerHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (PlayerCharacter && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() ==
		EWeaponType::EWT_Shotgun)
	{
		PlayerCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TracerHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}
	if (PlayerCharacter && CombatState == ECombatState::ECS_Unoccupied)
	{
		PlayerCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TracerHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (PlayerCharacter == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon && !SecondaryWeapon)
		EquipSecondaryWeapon(WeaponToEquip);
	else
		EquipPrimaryWeapon(WeaponToEquip);

	// 角色面向与运动方向相同(bOrientRotationToMovement)
	PlayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	// 面向与镜头方向相同(bUseControllerRotationYaw)
	PlayerCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;

	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	// 这边WeaponState属性改变了之后，通过OnRep方法可以通知client进行操作
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHand(WeaponToEquip);
	// 这个SetOwner方法可以看定义，当服务器端调用SetOwner的时候，会通知客户端
	EquippedWeapon->SetOwner(PlayerCharacter);
	// 服务端更新子弹数
	EquippedWeapon->UpdateWeaponAmmo();

	// 更新携带弹药HUD
	UpdateCarriedAmmo();
	// 更新携带手榴弹HUD
	UpdateHUDGrenades();
	// 更新携带武器类型HUD
	UpdateCarriedWeaponType();
	// 播放捡枪声音
	PlayEquipWeaponSound(EquippedWeapon);
	// 如果捡起来的武器没子弹了，自动换弹
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackPack(WeaponToEquip);
	PlayEquipWeaponSound(SecondaryWeapon);
	SecondaryWeapon->SetOwner(PlayerCharacter);
}

void UCombatComponent::SwapWeapons()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	PlayEquipWeaponSound(EquippedWeapon);
	EquippedWeapon->UpdateWeaponAmmo();
	UpdateCarriedWeaponType();
	UpdateCarriedAmmo();
	UpdateHUDGrenades();

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackPack(SecondaryWeapon);
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return EquippedWeapon && SecondaryWeapon;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && PlayerCharacter)
	{
		// 这里调用SetWeaponState和AttachActor
		// 是为了在client端，能够先调用SetWeaponState，再执行AttachActor
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		
		AttachActorToRightHand(EquippedWeapon);
		// 播放捡枪声音
		PlayEquipWeaponSound(EquippedWeapon);
		// 更新子弹数
		EquippedWeapon->UpdateWeaponAmmo();
		// 当装备上武器后，需要更新客户端client的HUD武器类型
		UpdateCarriedWeaponType();
		UpdateCarriedAmmo();
		// 更新携带手榴弹HUD
		UpdateHUDGrenades();

		// 角色面向与运动方向相同(bOrientRotationToMovement)
		PlayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		// 面向与镜头方向相同(bUseControllerRotationYaw)
		PlayerCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && PlayerCharacter)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackPack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
		EquippedWeapon->Dropped();
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (PlayerCharacter == nullptr || PlayerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = PlayerCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
		HandSocket->AttachActor(ActorToAttach, PlayerCharacter->GetMesh());
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (PlayerCharacter == nullptr || PlayerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr ||
		EquippedWeapon == nullptr)
		return;
	bool bUsePistolWeapon =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;
	FName SocketName = bUsePistolWeapon ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* HandSocket = PlayerCharacter->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
		HandSocket->AttachActor(ActorToAttach, PlayerCharacter->GetMesh());
}

void UCombatComponent::AttachActorToBackPack(AActor* ActorToAttach)
{
	if (PlayerCharacter == nullptr || PlayerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* BackPackSocket = PlayerCharacter->GetMesh()->GetSocketByName(FName("BackPackSocket"));
	if (BackPackSocket)
		BackPackSocket->AttachActor(ActorToAttach, PlayerCharacter->GetMesh());
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
}

void UCombatComponent::UpdateCarriedWeaponType()
{
	if (EquippedWeapon == nullptr) return;
	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDCarriedWeaponType(EquippedWeapon->GetWeaponType());
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (PlayerCharacter && WeaponToEquip && WeaponToEquip->EquipSound)
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, PlayerCharacter->GetActorLocation());
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
		Reload();
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (PlayerCharacter && PlayerCharacter->GetAttachGrenade())
		PlayerCharacter->GetAttachGrenade()->SetVisibility(bShowGrenade);
}

void UCombatComponent::Reload()
{
	// 必须有子弹以及不处于Reloading状态才能Reload
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
		ServerReload();
}

void UCombatComponent::ServerReload_Implementation()
{
	if (PlayerCharacter == nullptr) return;
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// 下面两行判断作用是：client端在reload之后能持续开枪
		if (bFireButtonPressed)
			Fire();
		break;
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		HandleThrowGrenade();
		break;
	default: ;
	}
}

void UCombatComponent::HandleReload()
{
	if (PlayerCharacter == nullptr) return;
	PlayerCharacter->PlayReloadMontage();
	if (bAiming && CombatState == ECombatState::ECS_Reloading)
		SetAiming(false);
}

void UCombatComponent::FinishReload()
{
	if (PlayerCharacter == nullptr) return;
	if (PlayerCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValue();
	}
	// 下面两行判断作用是：server端在reload之后能持续开枪
	if (bFireButtonPressed)
		Fire();
}

void UCombatComponent::ShotgunShellReload()
{
	if (PlayerCharacter && PlayerCharacter->HasAuthority())
		UpdateShotgunAmmoValue();
}

void UCombatComponent::UpdateAmmoValue()
{
	if (PlayerCharacter == nullptr || EquippedWeapon == nullptr) return;

	int32 ReloadAmmoAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmmoAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// 调整CarriedAmmo数量和HUD
	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	// 调整Ammo数量和HUD(由于Ammo是可复制的，因此在OnRep中会更新HUD)
	EquippedWeapon->AddAmmo(ReloadAmmoAmount);
}

void UCombatComponent::UpdateShotgunAmmoValue()
{
	if (PlayerCharacter == nullptr || EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// 调整CarriedAmmo数量和HUD
	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	// 调整Ammo数量和HUD(由于Ammo是可复制的，因此在OnRep中会更新HUD)
	EquippedWeapon->AddAmmo(1);
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
		// jump to ShotgunEnd Section
		JumpToShotgunEnd();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 CarriedAmount = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, CarriedAmount);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::ThrowGrenade()
{
	if (CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon)
		ServerThrowGrenade();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0) return;
	if (PlayerCharacter == nullptr) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	HandleThrowGrenade();
}

void UCombatComponent::HandleThrowGrenade()
{
	if (EquippedWeapon && PlayerCharacter)
	{
		PlayerCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	// 调整CarriedAmmo数量和HUD
	PlayerController =
		PlayerController == nullptr ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller) : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDGrenades(Grenades);
}

void UCombatComponent::ThrowGrenadeFinished()
{
	if (PlayerCharacter == nullptr || EquippedWeapon == nullptr) return;
	if (PlayerCharacter->HasAuthority())
		CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (PlayerCharacter && PlayerCharacter->IsLocallyControlled())
		ServerLaunchGrenade(HitTarget);
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (PlayerCharacter && PlayerCharacter->GetAttachGrenade())
	{
		const FVector StartingLocation = PlayerCharacter->GetAttachGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = PlayerCharacter;
		SpawnParameters.Instigator = PlayerCharacter;
		if (UWorld* World = GetWorld())
		{
			AProjectile* Grenade =
				World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParameters);
			if (Grenade)
				Grenade->CollisionBox->IgnoreActorWhenMoving(SpawnParameters.Owner, true);
		}
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun && CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
		JumpToShotgunEnd();
}

void UCombatComponent::InitializeCarriedAmmoMap()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartCarriedAmmoAmount_AR);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartCarriedAmmoAmount_RL);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartCarriedAmmoAmount_PIS);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartCarriedAmmoAmount_SG);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartCarriedAmmoAmount_Shotgun);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartCarriedAmmoAmount_Snip);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartCarriedAmmoAmount_GL);
}

// 转换得到CrosshairLocation和TraceHitResult
void UCombatComponent::TraceUnderCrosshair(FHitResult& TraceHitResult)
{
	FVector2d ViewportSize;
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2d CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	// 这里得到的 CrosshairWorldPosition 是摄像头的位置，CrosshairWorldDirection 是 摄像头的方向
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
	                                                               CrosshairLocation,
	                                                               CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		// start、end分别是准星瞄准对象检测线的起点和终点
		FVector Start = CrosshairWorldPosition;

		if (PlayerCharacter)
		{
			float DistanceToCharacter = (PlayerCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility);
		// 如果没有命中，将命中点设置为End
		if (!TraceHitResult.bBlockingHit)
			TraceHitResult.ImpactPoint = End;

		// 准星遇到其他玩家 变红色
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
			HUDPackage.CrosshairColor = FLinearColor::Red;
		else
			HUDPackage.CrosshairColor = FLinearColor::White;
	}
}

void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (PlayerCharacter == nullptr || PlayerCharacter->Controller == nullptr) return;

	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(PlayerCharacter->Controller)
		                   : PlayerController;
	if (PlayerController)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(PlayerController->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			/**
			 *	Carculate crosshair spread
			 */

			// Velocity factor
			if (PlayerCharacter->bIsCrouched)
			{
				WalkSpeedRange = FVector2D(0.f, PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed);
				VelocityMultiplierRange = FVector2D(0.f, 1.f);
			}
			else
			{
				WalkSpeedRange = FVector2D(0.f, PlayerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched);
				VelocityMultiplierRange = FVector2D(0.f, 0.5f);
			}
			FVector Velocity = PlayerCharacter->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange,
			                                                            VelocityMultiplierRange, Velocity.Size());
			// In air factor
			if (PlayerCharacter->GetCharacterMovement()->IsFalling())
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			else
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);

			// aim factor
			if (bAiming)
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			else
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);

			// 随时将 CrosshairShootingFactor 调整为 0 ，一旦其不射击了
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread = 0.5f +
				CrosshairVelocityFactor +
				CrosshairInAirFactor -
				CrosshairAimFactor +
				CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime,
		                              EquippedWeapon->GetZoomInterpSpeed());
	else
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);

	if (PlayerCharacter && PlayerCharacter->GetFollowCamera())
		// 这里用SetFieldOfView赋值，而不是直接FieldOfView = CurrentFOV
		PlayerCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}
