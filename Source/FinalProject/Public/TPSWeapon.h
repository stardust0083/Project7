// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSWeapon.generated.h"

USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector_NetQuantize TraceFrom;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class FINALPROJECT_API ATPSWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATPSWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMeshComponent* WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName SocketName = "MuzzleFlash";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	UParticleSystem* FireEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	float LastValidFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	float FireGapTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin=0.0f))
	float BulletTraceSpread;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin=0.0f))
	float HeadShotDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin=0.0f))
	float BulletDamage;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(bool Aiming);

	UPROPERTY(ReplicatedUsing=OnRep_Trace)
	FHitScanTrace BulletTrace;

	UFUNCTION()
	void OnRep_Trace();


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Replicated, Meta=(ExposeOnSpawn=true), Category="Weapon")
	FName GunType;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire(bool Aiming);
};
