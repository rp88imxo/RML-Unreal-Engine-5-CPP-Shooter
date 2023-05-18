// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
ARMLShooterCharacter::ARMLShooterCharacter() :
	BaseTurnRate{ 45.f },
	BaseLookUpRate{ 45.f },
	BaseMouseRotationSensitivity { 1.f}
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

void ARMLShooterCharacter::HandleMoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0., Rotation.Yaw,0. };

		//const FVector Direction = YawRotation.Vector() * Value;
		const FVector Direction = { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };

		AddMovementInput(Direction, Value);
	}
}

void ARMLShooterCharacter::HandleMoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0., Rotation.Yaw,0. };

		//const FVector Direction = YawRotation.Vector() * Value;
		const FVector Direction = { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };

		AddMovementInput(Direction, Value);
	}
}

void ARMLShooterCharacter::HandleTurnRate(float Rate)
{
	const float yawValue = Rate * BaseTurnRate * GetWorld()->DeltaTimeSeconds;
	AddControllerYawInput(yawValue);
}

void ARMLShooterCharacter::HandleLookUpRate(float Rate)
{
	const float pitchValue = Rate * BaseTurnRate * GetWorld()->DeltaTimeSeconds;
	AddControllerPitchInput(pitchValue);
}

void ARMLShooterCharacter::HandleTurn(float Value)
{
	const float yawValue = Value * BaseMouseRotationSensitivity;
	AddControllerYawInput(yawValue);
}

void ARMLShooterCharacter::HandleLookUp(float Value)
{
	const float pitchValue = Value * BaseMouseRotationSensitivity;
	AddControllerPitchInput(pitchValue);
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
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(
		TEXT("MoveForward"), this, &ARMLShooterCharacter::HandleMoveForward);

	PlayerInputComponent->BindAxis(
		TEXT("MoveRight"), this, &ARMLShooterCharacter::HandleMoveRight);

	PlayerInputComponent->BindAxis(
		TEXT("TurnRate"), this, &ARMLShooterCharacter::HandleTurnRate);

	PlayerInputComponent->BindAxis(
		TEXT("LookUpRate"), this, &ARMLShooterCharacter::HandleLookUpRate);

	PlayerInputComponent->BindAxis(
		TEXT("Turn"), this, &ARMLShooterCharacter::HandleTurn);

	PlayerInputComponent->BindAxis(
		TEXT("LookUp"), this, &ARMLShooterCharacter::HandleLookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
}

USpringArmComponent* ARMLShooterCharacter::GetCameraBoom() const
{
	return CameraSpringArm;
}

UCameraComponent* ARMLShooterCharacter::GetFollowCamera() const
{
	return FollowCamera;
}

