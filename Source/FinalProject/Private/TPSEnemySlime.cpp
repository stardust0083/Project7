// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSEnemySlime.h"

// Sets default values
ATPSEnemySlime::ATPSEnemySlime()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATPSEnemySlime::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATPSEnemySlime::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATPSEnemySlime::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

