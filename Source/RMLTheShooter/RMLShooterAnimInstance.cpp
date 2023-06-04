// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterAnimInstance.h"
#include "RMLShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"

URMLShooterAnimInstance::URMLShooterAnimInstance() :
	Speed{0},
	bIsInAir {false},
	bIsAccelerating{false},
	CharacterYawTurnInPlace{0.f},
	CharacterYawLastFrameTurnInPlace{0.f},
	OffsetState{EOffsetState::EOS_Hip},
	YawDeltaInterpSpeed{300.f},
	CharacterRotation {FRotator::ZeroRotator},
	CharacterRotationLastFrame {FRotator::ZeroRotator},
	YawDelta {0.f},
	RecoilWeight{0.f},
	bTurningInPlace {false}
{

}

void URMLShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<ARMLShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
		bCrouching = ShooterCharacter->GetCrouching();
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::EAS_Reloading;

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

		if (bReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir) 
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetAiming())
		{
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else 
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
		
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

	TurnInPlace();
	UpdateControlPitchRotation();
	UpdateLean(DeltaTime);
}

void URMLShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<ARMLShooterCharacter>(TryGetPawnOwner());
}

void URMLShooterAnimInstance::TurnInPlace()
{
	if (ShooterCharacter == nullptr)
	{
		return;
	}

	if (Speed > 0.f || bIsInAir)
	{
		RootYawOffset = 0.f;
		CharacterYawTurnInPlace = ShooterCharacter->GetActorRotation().Yaw;
		CharacterYawLastFrameTurnInPlace = CharacterYawTurnInPlace;
		RotationCurveValue = 0.f;
		RotationCurveValueLastFrame = 0.f;
	}
	else // turn in place only if we stand still
	{
		CharacterYawLastFrameTurnInPlace = CharacterYawTurnInPlace;
		CharacterYawTurnInPlace = ShooterCharacter->GetActorRotation().Yaw;

		const float YawDeltaTurnInPlace{ CharacterYawTurnInPlace - CharacterYawLastFrameTurnInPlace };

		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDeltaTurnInPlace);

		const float Turning{ GetCurveValue(TEXT("Turning")) };
		bTurningInPlace = Turning > 0.f;

		if (Turning > 0.f)
		{
			RotationCurveValueLastFrame = RotationCurveValue;
			RotationCurveValue = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation{ RotationCurveValue - RotationCurveValueLastFrame };

			if (RootYawOffset > 0) // That means we turning left
			{
				RootYawOffset -= DeltaRotation;
			}
			else // Otherwise we turning right
			{
				RootYawOffset += DeltaRotation;
			}

			const float ABSRootYawOffset{ FMath::Abs(RootYawOffset) };

			if (ABSRootYawOffset > 90.f)
			{
				const float YawExcess{ ABSRootYawOffset  - 90.f};
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1, 0.f, FColor::Blue,
				FString::Printf(TEXT("CharacterYaw: %f"), CharacterYawTurnInPlace));
			GEngine->AddOnScreenDebugMessage(
			2, 0.f, FColor::Blue,
				FString::Printf(TEXT("RootYawOffset: %f"), RootYawOffset));
		}
	}

#pragma region UPDATE_RECOIL_WEIGHT

	if (bTurningInPlace)
	{
		if (bReloading)
		{
			RecoilWeight = 1.f;
		}
		else
		{
			RecoilWeight = 0.f;
		}
	}
	else
	{
		if (bCrouching)
		{
			if (bReloading)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if (bReloading)
			{
				RecoilWeight = 1.0f;
			}
			else if (bAiming)
			{
				if (bReloading)
				{
					RecoilWeight = 1.0f;
				}
				else {
					RecoilWeight = 0.5f;
				}
			}
			else
			{
				RecoilWeight = 1.0f;
			}
		}
	}

#pragma endregion
}

void URMLShooterAnimInstance::UpdateControlPitchRotation()
{
	if (ShooterCharacter)
	{
		ControlPitchRotation = ShooterCharacter->GetBaseAimRotation().Pitch;
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				3, 0.f, FColor::Blue,
				FString::Printf(TEXT("ControlPitchRotation: %f"), ControlPitchRotation));
		}
	}
}

void URMLShooterAnimInstance::UpdateLean(float DeltaTime)
{
	if (ShooterCharacter == nullptr)
	{
		return;
	}

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();

	const FRotator DeltaRotator{ UKismetMathLibrary::NormalizedDeltaRotator
	(CharacterRotation,
	CharacterRotationLastFrame) };

	const float Target{(static_cast<float>(DeltaRotator.Yaw))};
	const float Interp{ FMath::FInterpTo(YawDelta, Target / DeltaTime, DeltaTime, YawDeltaInterpSpeed) };

	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			4, -1.f, FColor::Blue,
			FString::Printf(TEXT("Lean Yaw Delta: %f"), YawDelta));
	}
}
