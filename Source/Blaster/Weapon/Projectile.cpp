// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	// bReplicates = true 时，该对象或属性的变化将会从服务器复制到所有连接的客户端
	// 即属性变量可复制
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		// UGameplayStatics::SpawnEmitterAttached 函数用于在游戏中创建并附加一个粒子系统（Emitter）到指定的演员（Actor）
		// 用来创建一个名为 Tracer 的粒子效果，并将其附加到当前演员的一个碰撞框（CollisionBox）上
		// FName(): 这是一个空的名字参数，通常用于指定附加点的名称.在这里，它被留空
		// EAttachLocation::KeepWorldPosition: 这个枚举值指定了粒子系统应该如何相对于附加的组件定位。
		// 自动计算相对变换，使得附加的组件保持相同的世界变换。
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(Tracer, CollisionBox, FName(),
		                                                         GetActorLocation(), GetActorRotation(),
		                                                         EAttachLocation::KeepWorldPosition);
	}

	// 仅在服务端执行OnHit函数
	if (HasAuthority())
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	// OnHit这个方法确实只会在server上执行
	// 但是Destroy()摧毁了Projectile这个对象
	// 由于bReplicates = true
	// 导致也会在client上执行Destroy() Projectile这个对象的操作
	Destroy();
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AProjectile::DestroyTimerFinished, DestroyDelay);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem, GetRootComponent(), FName(), GetActorLocation(),
			GetActorRotation(), EAttachLocation::KeepWorldPosition, false);
	}
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				1.f,
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(),
				this,
				FiringController // Instigator Controller
			);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticle)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorLocation());
	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
}
