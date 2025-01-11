// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	// bReplicates = true 时，该对象或属性的变化将会从服务器复制到所有连接的客户端
	// 即属性变量可复制
	bReplicates = true;
	// movement在网络上复制
	AActor::SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	// 此时weapon还是物体状态，所以要忽略pawn+取消collision
	// 下面两行设置，可以将手枪可与其他所有物体发生碰撞，除了玩家
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	// 在所有client上都设置为NoCollision，后续在server上设置为QueryAndPhysics
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 设置初始武器的custom depth
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 在所有client上都设置为NoCollision，后续在server上设置为QueryAndPhysics
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickUpWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickUpWidget)
		PickUpWidget->SetVisibility(false);

	// 只有在服务器上可以检测是否捡武器
	// <==> if (GetLocalRole() == ROLE_Authority)
	if (HasAuthority())
	{
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// 用来人物靠近时，显示pickup提示
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		// 用来人物离开时，不显示pickup提示
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
		UpdateWeaponAmmo();
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                              const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
		BlasterCharacter->SetOverlappingWeapon(this);
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
		BlasterCharacter->SetOverlappingWeapon(nullptr);
}

void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = IsValid(BlasterOwnerCharacter)
		                        ? BlasterOwnerCharacter
		                        : Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	UpdateWeaponAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	UpdateWeaponAmmo();
}

void AWeapon::UpdateWeaponAmmo()
{
	BlasterOwnerCharacter = IsValid(BlasterOwnerCharacter)
		                        ? BlasterOwnerCharacter
		                        : Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = IsValid(BlasterOwnerController)
			                         ? BlasterOwnerController
			                         : Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
		if (BlasterOwnerController)
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickUpWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		EnableCustomDepth(false);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	default: ;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickUpWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		EnableCustomDepth(false);
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	default: ;
	}
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
		WeaponMesh->SetRenderCustomDepth(bEnable);
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

void AWeapon::ShowPickUpWidget(bool bShowWidget)
{
	if (PickUpWidget)
		PickUpWidget->SetVisibility(bShowWidget);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	// 执行动画
	if (FireAnimation)
		WeaponMesh->PlayAnimation(FireAnimation, false);

	// 生成子弹
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = GetWeaponMesh()->GetSocketByName(FName("AmmoEject"));
		// 先判断一下武器的socket是否存在
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(GetWeaponMesh());
			if (UWorld* World = GetWorld())
				// SpawnActor 用于在游戏世界中创建（即“生成”）一个新的 Actor 实例
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(),
				                           SocketTransform.GetRotation().Rotator());
		}
	}

	// 子弹量减一 
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmountToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmountToAdd, 0, MagCapacity);
	UpdateWeaponAmmo();
}
