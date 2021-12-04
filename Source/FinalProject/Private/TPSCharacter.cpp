// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCharacter.h"

#include "TPSWeapon.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HealthComponent.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "Niagara/Public/NiagaraFunctionLibrary.h"
// Sets default values
ATPSCharacter::ATPSCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->SetupAttachment(RootComponent);

	TPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TPSCamera"));
	TPSCamera->SetupAttachment(CameraSpringArm);

	AimSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("AimSpringArm"));
	AimSpringArm->SetupAttachment(RootComponent);
	AimSpringArm->TargetArmLength = 50;


	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	bAiming = false;
	DeathFX = ConstructorHelpers::FObjectFinder<UNiagaraSystem>(
		TEXT("NiagaraSystem'/Game/Effect/DeanSystem.DeanSystem'")).Object;
	ConstructorHelpers::FObjectFinder<UBlueprint> WeaponBP(TEXT("Blueprint'/Game/Blueprint/Weapon.Weapon'"));
	if (WeaponBP.Object)
	{
		SpawnedWeapon = (UClass*)WeaponBP.Object->GeneratedClass;
	}
	bIsAlive = true;
}

// Called when the game starts or when spawned
void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComp->OnHealthChanged.AddDynamic(this, &ATPSCharacter::OnHealthChanged);

	ACharacter::GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	const FVector Location = GetActorLocation();
	const FRotator Rotation = GetActorRotation();
	UWorld* const World = GetWorld();
	if (GetLocalRole() == ROLE_Authority)
	{
		if (World)
		{
			EquippedWeapon = World->SpawnActorDeferred<ATPSWeapon>(SpawnedWeapon, FTransform(Rotation, Location));
			EquippedWeapon->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			EquippedWeapon->SetInstigator(this);
			EquippedWeapon->GunType = "AR";
			EquippedWeapon->FinishSpawning(FTransform(Rotation, Location));
			EquippedWeapon->SetOwner(this);
			EquippedWeapon->AttachToComponent(this->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                  "WeaponSocket");
			GunList.Add("AR");
			CurrentIndex = 0;
		}


		if (EquippedWeapon)
		{
			AimSpringArm->AttachToComponent(EquippedWeapon->GetRootComponent(),
			                                FAttachmentTransformRules::SnapToTargetIncludingScale, "Aim");
			AimSpringArm->SetIsReplicated(true);
			AimCamera = NewObject<UCameraComponent>(this, UCameraComponent::StaticClass(),TEXT("AimCamera"));
			AimCamera->AttachToComponent(AimSpringArm, FAttachmentTransformRules::SnapToTargetIncludingScale);
			AimCamera->FieldOfView = 90.f;
			AimCamera->SetIsReplicated(true);
			AimCamera->AddWorldTransform(FTransform(FVector(1, 1, 1)));
			AimCamera->AddWorldTransform(FTransform(FVector(-1, -1, -1)));
		}
		//Test Code
		GunList.Add("RF");
		GunList.Add("SG");
	}
}

void ATPSCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ATPSCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ATPSCharacter::BeginCrouch()
{
	Crouch();
}

void ATPSCharacter::EndCrouch()
{
	UnCrouch();
}

void ATPSCharacter::BeginAiming()
{
	if (GetLocalRole() != ROLE_Authority)
	{
		ServerAim();
	}
	bAiming = true;
	if (TPSCamera != nullptr && AimCamera != nullptr)
	{
		TPSCamera->SetActive(false);
		AimCamera->SetActive(true);
	}
}

void ATPSCharacter::EndAiming()
{
	bAiming = false;
	if (GetLocalRole() != ROLE_Authority)
	{
		ServerAimEnd();
	}
	if (TPSCamera != nullptr && AimCamera != nullptr)
	{
		TPSCamera->SetActive(true);
		AimCamera->SetActive(false);
	}
}


void ATPSCharacter::OnHealthChanged(UHealthComponent* OwningHealthComp, float Health, float HealthDelta,
                                    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::SanitizeFloat(HealthComp->GetHealth()));
	if (Health <= 0.0f && bIsAlive)
	{
		// Die!
		bIsAlive = false;
		UNiagaraFunctionLibrary::SpawnSystemAttached(DeathFX, GetMesh(), FName("Spine"),
		                                             GetMesh()->GetComponentTransform().GetLocation(),
		                                             GetMesh()->GetComponentTransform().GetLocation().Rotation(),
		                                             EAttachLocation::SnapToTargetIncludingScale, true);
		DetachFromControllerPendingDestroy();
		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EquippedWeapon->SetLifeSpan(10.f);
		this->GetMesh()->SetIsReplicated(true);
		this->GetMesh()->SetAllBodiesSimulatePhysics(true);
		this->GetMesh()->SetAllBodiesPhysicsBlendWeight(true);
		this->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SetLifeSpan(10.0f);
	}
}

void ATPSCharacter::SwitchGun()
{
	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	UWorld* const world = GetWorld();
	//EquippedWeapon->SetReplicates(true);
	if (world && !bAiming && GetLocalRole() != ROLE_Authority && GunList.Num() != 0)
	{
		ServerSwitchGun();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, "ServerSwitch");
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::SanitizeFloat(CurrentIndex));
	}

	if (world && !bAiming && GunList.Num() != 0)
	{
		CurrentIndex = CurrentIndex + 1;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, "ClientSwitch");
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, FString::SanitizeFloat(CurrentIndex));
		NewWeaponType = FName(GunList[(CurrentIndex) % GunList.Num()]);
		NewGun = world->SpawnActorDeferred<ATPSWeapon>(SpawnedWeapon, FTransform(Rotation, Location));
		NewGun->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		NewGun->SetInstigator(this);

		NewGun->GunType = NewWeaponType;
		if (NewGun->GunType == "RF")
		{
			AimCamera->FieldOfView = 50;
		}
		else
		{
			AimCamera->FieldOfView = 90;
		}
		NewGun->FinishSpawning(FTransform(Rotation, Location));
		NewGun->SetOwner(this);
		NewGun->AttachToComponent(this->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
		                          "WeaponSocket");
		EquippedWeapon->Destroy();
		EquippedWeapon = NewGun;
		if (EquippedWeapon)
		{
			AimSpringArm->AttachToComponent(EquippedWeapon->GetRootComponent(),
			                                FAttachmentTransformRules::SnapToTargetIncludingScale, "Aim");
			AimCamera->AttachToComponent(AimSpringArm,
			                             FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}

	/*
		int RandomGun = rand();
		FVector Location = GetActorLocation();
		FRotator Rotation = GetActorRotation();
	
		UWorld* const world = GetWorld();
		if (world && !bAiming && GetLocalRole() == ROLE_Authority)
		{
			ATPSWeapon* NewGun = world->SpawnActorDeferred<ATPSWeapon>(SpawnedWeapon, FTransform(Rotation, Location));
			NewGun->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			NewGun->SetInstigator(this);
			if (RandomGun % 5 == 0)
			{
				NewGun->GunType = "AR";
				AimCamera->FieldOfView = 90;
			}
			if (RandomGun % 5 == 1)
			{
				NewGun->GunType = "SG";
				AimCamera->FieldOfView = 90;
			}
			if (RandomGun % 5 == 2)
			{
				NewGun->GunType = "GL";
				AimCamera->FieldOfView = 90;
			}
			if (RandomGun % 5 == 3)
			{
				NewGun->GunType = "RL";
				AimCamera->FieldOfView = 90;
			}
			if (RandomGun % 5 == 4)
			{
				NewGun->GunType = "RF";
				AimCamera->FieldOfView = 50;
			}
			NewGun->FinishSpawning(FTransform(Rotation, Location));
			NewGun->SetOwner(this);
			NewGun->AttachToComponent(this->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
			                          "WeaponSocket");
			EquippedWeapon->Destroy();
			EquippedWeapon = NewGun;
			if (RandomGun % 5 == 2 || RandomGun % 5 == 3)
			{
				HeavyWeaponSlot = EquippedWeapon;
			}
			else
			{
				LightWeaponSlot = EquippedWeapon;
			}
			if (EquippedWeapon)
			{
				AimSpringArm->AttachToComponent(EquippedWeapon->GetRootComponent(),
				                                FAttachmentTransformRules::SnapToTargetIncludingScale, "Aim");
				AimCamera->AttachToComponent(AimSpringArm,
				                             FAttachmentTransformRules::SnapToTargetIncludingScale);
			}
		}
		*/
}

void ATPSCharacter::ServerAim_Implementation()
{
	bAiming = true;
}

bool ATPSCharacter::ServerAim_Validate()
{
	return true;
}

void ATPSCharacter::ServerAimEnd_Implementation()
{
	bAiming = false;
}

bool ATPSCharacter::ServerAimEnd_Validate()
{
	return true;
}


// Called every frame
void ATPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector ATPSCharacter::CameraLocation()
{
	if (AimCamera)
	{
		if (AimCamera->IsActive())
		{
			return AimCamera->GetComponentLocation();
		}
	}
	if (TPSCamera)
	{
		if (TPSCamera->IsActive())
		{
			return TPSCamera->GetComponentLocation();
		}
	}
	return FVector(0.f, 0.f, 0.f);
}

inline void ATPSCharacter::ServerSwitchGun_Implementation()
{
	CurrentIndex = CurrentIndex + 1;
	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	NewWeaponType = FName(GunList[(CurrentIndex) % GunList.Num()]);
	NewGun = GetWorld()->SpawnActorDeferred<ATPSWeapon>(SpawnedWeapon, FTransform(Rotation, Location));
	NewGun->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	NewGun->SetInstigator(this);

	NewGun->GunType = NewWeaponType;
	if (NewGun->GunType == "RF")
	{
		AimCamera->FieldOfView = 50;
	}
	else
	{
		AimCamera->FieldOfView = 90;
	}
	NewGun->FinishSpawning(FTransform(Rotation, Location));
	NewGun->SetOwner(this);
	NewGun->AttachToComponent(this->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
	                          "WeaponSocket");
	EquippedWeapon->Destroy();
	EquippedWeapon = NewGun;
	if (EquippedWeapon)
	{
		AimSpringArm->AttachToComponent(EquippedWeapon->GetRootComponent(),
		                                FAttachmentTransformRules::SnapToTargetIncludingScale, "Aim");
		AimCamera->AttachToComponent(AimSpringArm,
		                             FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
}

inline bool ATPSCharacter::ServerSwitchGun_Validate()
{
	return true;
}

void ATPSCharacter::FireTest()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Fire(bAiming);
	}
	else
	{
		//Punch();
	}
}

// Called to bind functionality to input
void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &ATPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPSCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ATPSCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookRight", this, &ATPSCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ATPSCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ATPSCharacter::EndCrouch);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATPSCharacter::FireTest);
	PlayerInputComponent->BindAction("Switch", IE_Pressed, this, &ATPSCharacter::SwitchGun);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ATPSCharacter::BeginAiming);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ATPSCharacter::EndAiming);
}

void ATPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCharacter, EquippedWeapon);
	DOREPLIFETIME(ATPSCharacter, GunList);
	DOREPLIFETIME(ATPSCharacter, NewGun);
	DOREPLIFETIME(ATPSCharacter, CurrentIndex);
	DOREPLIFETIME(ATPSCharacter, AimCamera);
	DOREPLIFETIME(ATPSCharacter, AimSpringArm);
	DOREPLIFETIME(ATPSCharacter, bIsAlive);
	DOREPLIFETIME(ATPSCharacter, NewWeaponType);
	//DOREPLIFETIME(ATPSCharacter, HealthComp);
}
