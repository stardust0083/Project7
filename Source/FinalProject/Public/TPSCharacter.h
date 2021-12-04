// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HealthComponent.h"
#include "TPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "TPSCharacter.generated.h"

UCLASS()
class FINALPROJECT_API ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATPSCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void BeginCrouch();
	void EndCrouch();

	UFUNCTION(BlueprintCallable, Category="Camera")
	void BeginAiming();

	UFUNCTION(BlueprintCallable, Category="Camera")
	void EndAiming();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCameraComponent* TPSCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Components")
	UCameraComponent* AimCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Components")
	USpringArmComponent* AimSpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	ATPSWeapon* EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	ATPSWeapon* NewGun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	FName NewWeaponType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* DeathFX;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	TSubclassOf<ATPSWeapon> SpawnedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bAiming = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Components")
	UHealthComponent* HealthComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Player")
	bool bIsAlive;

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* OwningHealthComp, float Health, float HealthDelta,
	                     const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);


	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SwitchGun();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAim();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAimEnd();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchGun();


	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerAttach();
	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerNewAimCamera();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Weapon")
	TArray<FString> GunList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Weapon")
	int CurrentIndex = 0;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintCallable, Category="Camera")
	FVector CameraLocation();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireTest();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
