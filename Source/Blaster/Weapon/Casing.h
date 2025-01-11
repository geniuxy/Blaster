// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"
#include "Casing.generated.h"

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()

public:
	ACasing();
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			   FVector NormalImpulse, const FHitResult& Hit);

	virtual void BeginDestroy() override;

	UFUNCTION()
	void DestroyShell();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere)
	float ShellEjectImpulse;

	UPROPERTY(EditAnywhere)
	USoundCue* ShellSound;
	
	// 定义一个计时器句柄
	FTimerHandle DestroyTimerHandle;

	// 定义一个计时器委托，它将在计时器结束时被调用
	FTimerDelegate DestroyTimerDelegate;

public:
};
