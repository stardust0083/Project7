// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;
	MaxHealth = 200;
	CurrentHealth = 200;
	bIsDead = false;
	// ...
	this->SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		AActor* HealthOwner = GetOwner();
		if (HealthOwner)
		{
			HealthOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
		}
	}
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(this, CurrentHealth, 0.f, nullptr, nullptr,nullptr);
	// ...
}

void UHealthComponent::OnRep_Health(float OldHealth)
{
	float Damage = CurrentHealth - OldHealth;

	OnHealthChanged.Broadcast(this, CurrentHealth, Damage, nullptr, nullptr, nullptr);
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                           AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, DamagedActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::SanitizeFloat(CurrentHealth));
	bIsDead = CurrentHealth <= 0.0f;
	OnHealthChanged.Broadcast(this, CurrentHealth, Damage, DamageType, InstigatedBy, DamageCauser);
	/*if (bIsDead)
    	{
    		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
    		if (GM)
    		{
    			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
    		}
    	}*/
}

void UHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || CurrentHealth <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(CurrentHealth),
	       *FString::SanitizeFloat(HealAmount));

	OnHealthChanged.Broadcast(this, CurrentHealth, -HealAmount, nullptr, nullptr, nullptr);
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}
