// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterAnimInstance.h"
#include "RMLShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

void URMLShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<ARMLShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
		FVector Velocity{ ShooterCharacter->GetVelocity() };
		Velocity.Z = 0.;
		Speed = Velocity.Size();

		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;

		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
		
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

		if (ShooterCharacter->GetVelocity().Size() > 0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = ShooterCharacter->GetAiming();

#pragma region LOG_ROTATION_MOVEMENT
		FString MovementOffsetYawMessage =
			FString::Printf(TEXT("Movement Offset Yaw: %f"), MovementOffsetYaw);
		FString RotationMessage =
			FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);
		FString MovementRotationMessage =
			FString::Printf(TEXT("Movement Rotation Yaw: %f"), MovementRotation.Yaw);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1, 0.f, FColor::Blue, RotationMessage);
			GEngine->AddOnScreenDebugMessage(
				2, 0.f, FColor::Blue, MovementRotationMessage);
			GEngine->AddOnScreenDebugMessage(
				3, 0.f, FColor::Blue, MovementOffsetYawMessage);
		}
#pragma endregion

#pragma region VELOCITY_VECTOR_DEBUG
		auto v = ShooterCharacter->GetVelocity();
		v.Normalize();

		DrawDebugLine(GetWorld(), ShooterCharacter->GetActorLocation(),
			ShooterCharacter->GetActorLocation() + v * 100.f,
			FColor::Red, false);

#pragma endregion
	}
}

void URMLShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<ARMLShooterCharacter>(TryGetPawnOwner());
}
