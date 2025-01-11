// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"

#include "Kismet/GameplayStatics.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	// 子弹不会阻挡摄像头
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECollisionResponse::ECR_Ignore);
	// 启用子弹物理性能
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	// 开启Collision之间发生碰撞时的事件通知
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectImpulse);
	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	// 设置计时器委托要执行的操作，这里是摧毁actor
	DestroyTimerDelegate.BindUFunction(this, FName("DestroyShell"));
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShellSound, GetActorLocation());
	// 设置计时器，5秒后调用计时器委托
	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(DestroyTimerHandle, DestroyTimerDelegate, 5.0f, false);
}

void ACasing::BeginDestroy()
{
	Super::BeginDestroy();
	
	// 取消之前设置的计时器
	if (GetWorld() && DestroyTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(DestroyTimerHandle);
}

void ACasing::DestroyShell()
{
	Destroy();
}
