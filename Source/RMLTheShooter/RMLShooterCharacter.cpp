// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
ARMLShooterCharacter::ARMLShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->bUsePawnControlRotation = true; // Rotating the arm based on controller rotation

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void ARMLShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	int myInt{ 42 };
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

USpringArmComponent* ARMLShooterCharacter::GetCameraBoom() const
{
	return CameraSpringArm;
}

UCameraComponent* ARMLShooterCharacter::GetFollowCamera() const
{
	return FollowCamera;
}

