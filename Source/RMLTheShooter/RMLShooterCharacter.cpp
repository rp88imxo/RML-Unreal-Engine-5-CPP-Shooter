// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterCharacter.h"

// Sets default values
ARMLShooterCharacter::ARMLShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARMLShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARMLShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARMLShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

