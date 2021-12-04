// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSWeapon.h"

#include "DrawDebugHelpers.h"
#include "TPSCharacter.h"
#include "Components/LineBatchComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ATPSWeapon::ATPSWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMesh"));
	RootComponent = WeaponMeshComponent;
	this->bReplicates=true;
}

// Called when the game starts or when spawned
void ATPSWeapon::BeginPlay()
{
	Super::BeginPlay();
	LastValidFire = GetWorld()->TimeSeconds - 10;
}

void ATPSWeapon::Fire(bool Aiming)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		ServerFire(Aiming);
	}
	ATPSCharacter* Shooter = Cast<ATPSCharacter, AActor>(GetOwner());
	float AttemptFireTime = GetWorld()->TimeSeconds;
	if (AttemptFireTime - LastValidFire > FireGapTime && Shooter)
	{
		LastValidFire = AttemptFireTime;
		//dealing with the direction and location of aiming
		FHitResult HitTarget;
		FVector EyeLocation;
		FRotator EyeRotation;
		Shooter->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector ShootDirection = EyeRotation.Vector();

		if (Shooter->HasActiveCameraComponent())
		{
			EyeLocation = Shooter->CameraLocation();
		}
		if (Shooter->HasActiveCameraComponent() && WeaponMeshComponent->DoesSocketExist("Aim") && Aiming)
		{
			EyeLocation = Shooter->CameraLocation();
			//DrawDebugSphere(GetWorld(),EyeLocation,1000,50,FColor::Cyan,true,100.f,0,1);
			ShootDirection = (WeaponMeshComponent->GetSocketLocation("Aim") - EyeLocation).GetSafeNormal();
			//EyeLocation =WeaponMeshComponent->GetSocketLocation("Aim");
			//ShootDirection=GetActorRotation().Vector().GetSafeNormal();
		}
		//FVector GunShotLocation=WeaponMeshComponent->GetSocketLocation("MuzzleFlash");

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Shooter);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;

		if (GunType == "AR" || GunType == "SG" || GunType == "RF")
		{
			//Light Weapon
			//ShotGun->larger spread, 7 bullets one shot
			//AutoRifle->line trace with little spread
			//Sniper->accurate shot, aiming is dealt with in character
			int MaxShot = 1;
			if (GunType == "SG")
			{
				MaxShot = 7;
			}
			for (int i = 0; i < MaxShot; ++i)
			{
				float HalfRad = FMath::DegreesToRadians(BulletTraceSpread);
				FVector NewShootDirection = FMath::VRandCone(ShootDirection, HalfRad, HalfRad);
				FVector ShootEnd = EyeLocation + (NewShootDirection * 10000.f);
				if (GetWorld()->LineTraceSingleByChannel(HitTarget, EyeLocation, ShootEnd,
				                                         ECC_Camera, QueryParams))
				{
					//Damage function

					AActor* GetHitActor = HitTarget.GetActor();
					GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, HitTarget.BoneName.ToString());
					if (HitTarget.BoneName == "head")
					{
						UGameplayStatics::ApplyPointDamage(GetHitActor, HeadShotDamage, NewShootDirection, HitTarget,
						                                   Shooter->GetInstigatorController(),
						                                   this, DamageType);
					}
					else
					{
						UGameplayStatics::ApplyPointDamage(GetHitActor, BulletDamage, NewShootDirection, HitTarget,
						                                   Shooter->GetInstigatorController(),
						                                   this, DamageType);
					}
					if (HitTarget.GetComponent()->IsSimulatingPhysics())
					{
						HitTarget.GetComponent()->AddImpulseAtLocation(
							100.f * HitTarget.GetComponent()->GetMass() * NewShootDirection / NewShootDirection.Size(),
							HitTarget.Location, HitTarget.BoneName);
					}
					if (ImpactEffect)
					{
						UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, HitTarget.ImpactPoint,
						                                         HitTarget.ImpactNormal.Rotation());
					}
				}
				GetWorld()->LineBatcher->DrawLine(EyeLocation+50.f*NewShootDirection/NewShootDirection.Size(),  ShootEnd, FColor::Green, 0, 10.f, 10.f);
				if (HasAuthority())
				{
					BulletTrace.TraceTo = ShootEnd;
					BulletTrace.TraceFrom = EyeLocation + 50.f * NewShootDirection / NewShootDirection.Size();
				}
			}
		}
		else if (GunType == "RL")
		{
			//Rocket Launcher
		}
		else if (GunType == "GL")
		{
			//Grenade Launcher
		}

		if (FireEffect && FireSound)
		{
			UGameplayStatics::SpawnEmitterAttached(FireEffect, WeaponMeshComponent, SocketName);
			UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMeshComponent, SocketName);
		}
	}
}


void ATPSWeapon::ServerFire_Implementation(bool Aiming)
{
	Fire(Aiming);
}

bool ATPSWeapon::ServerFire_Validate(bool Aiming)
{
	return true;
}

void ATPSWeapon::OnRep_Trace()
{
	if (FireEffect && FireSound)
	{
		UGameplayStatics::SpawnEmitterAttached(FireEffect, WeaponMeshComponent, SocketName);
		UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMeshComponent, SocketName);
	}
	if (ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, BulletTrace.TraceTo,
		                                         (BulletTrace.TraceTo - BulletTrace.TraceFrom).Rotation());
	}
	GetWorld()->LineBatcher->DrawLine(WeaponMeshComponent->GetSocketLocation("MuzzleFlash"), BulletTrace.TraceTo,
	                                  FColor::White, 0, 1.f, 10.f);
}

// Called every frame
void ATPSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATPSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATPSWeapon, BulletTrace, COND_SkipOwner);
	DOREPLIFETIME(ATPSWeapon, GunType);
}
