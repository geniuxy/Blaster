// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	
	if (!HasAuthority()) return;
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	// 先判断一下武器的socket是否存在
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			// instigator 用于确定谁应该对某个事件负责，而 owner 用于确定谁拥有或控制某个对象
			// Owner是武器的主人
			SpawnParams.Owner = GetOwner();
			// Instigator是子弹射出事件的发起人
			SpawnParams.Instigator = InstigatorPawn;
			// Owner可能会变，Instigator在子弹击中后就不会变了
			if (UWorld* World = GetWorld())
				// SpawnActor 用于在游戏世界中创建（即“生成”）一个新的 Actor 实例
				World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation,
				                               SpawnParams);
		}
	}
}
