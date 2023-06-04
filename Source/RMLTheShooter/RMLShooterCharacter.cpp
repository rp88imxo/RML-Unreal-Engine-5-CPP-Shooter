// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "RMLItem.h"
#include "Components/WidgetComponent.h"
#include "RMLWeapon.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
ARMLShooterCharacter::ARMLShooterCharacter() :
	BaseTurnRate{ 45.f },
	BaseLookUpRate{ 45.f },
	HipMouseSensitivity { 1.f},
	bAiming {false},
	CameraCurrentFOV{0.f},
	ZoomInterpSpeed{30.f},
	AimingMouseSensitivity {0.7f},
	HipTurnRate {90.f},
	HipLookUpRate {90.f},
	AimingTurnRate {20.f},
	AimingLookUpRate{20.f},
	CurrentMouseSensitivity{0.f},
	CrosshairInAirMaxFactor{2.5f},
	CrosshairInAirMultiplierInterpSpeed{10.f},
	CrosshairAimFactor{0.f},
	CrosshairShootingFactor{0.f},
	ShootTimeDuration{0.05f},
	bFiringBullet {false},
	AutomaticFireRate {60.f / 600.f},
	bShouldFire{true},
	bFireButtonPressed{false},
	bShouldTraceForItems{false},
	CameraForwardDistance {250.f},
	CameraUpDistance {50.f},
	Starting9mmAmmo{36},
	StartingARAmmo{180}
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->bUsePawnControlRotation = true; // Rotating the arm based on controller rotation

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// We don't need to rotate character to match the control rotation from pawn
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;

	// we rotate character to orient toward the direction of movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
}

// Called when the game starts or when spawned
void ARMLShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	CameraDefaultFOV = GetFollowCamera()->FieldOfView;
	CameraCurrentFOV = CameraDefaultFOV;

	// Spawn a default weapon and Equip it
	auto SpawnedWeapon = SpawnDefaultWeapon();
	EquipWeapon(SpawnedWeapon);

	InitializeAmmoMap();
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
	const float yawValue = Value * CurrentMouseSensitivity;
	AddControllerYawInput(yawValue);
}

void ARMLShooterCharacter::HandleLookUp(float Value)
{
	const float pitchValue = Value * CurrentMouseSensitivity;
	AddControllerPitchInput(pitchValue);
}

void ARMLShooterCharacter::FireWeapon()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const auto BarrelSocket =
		GetMesh()->GetSocketByName("RMLBarrelSocket");

	const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached
		(MuzzleFlash,
			GetMesh(),
			TEXT("RMLBarrelSocket"));
	}

	FVector OutBeamLocation;
	bool HasHitSomething;
	auto bIsSuccesfull = GetBeamEndLocation(
		SocketTransform.GetLocation(), OutBeamLocation, HasHitSomething);
	if (bIsSuccesfull)
	{
		// Spawn impact particles right after perform a final beam end location
		if (ImpactHitParticles && HasHitSomething)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
				ImpactHitParticles, OutBeamLocation);
		}

		if (BeamParticles)
		{
			auto BeamParticleSystemComponent =
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					BeamParticles, SocketTransform);

			BeamParticleSystemComponent->SetVectorParameter(FName("Target"), OutBeamLocation);
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(TEXT("StartFire"));
	}

	// indicates that we shooted so we can spread a crosshair for some amount of time
	StartCrosshairBulletFire();

	if (EquippedWeapon)
	{
		EquippedWeapon->DecrementAmmo();
	}
}


bool ARMLShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation, bool& HasHitSomething)
{
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		OutBeamLocation = CrosshairHitResult.ImpactPoint;
	}
	else 
	{
		// We didnt hit
	}

	// Perform a second trace form a gun barrel so we can stop a smoke beam at right point
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocation };
	const FVector WeaponEndTrace{ MuzzleSocketLocation + StartToEnd * 1.25f };

	auto bHasHitFromBarrel = GetWorld()->LineTraceSingleByChannel(
		WeaponTraceHit,
		WeaponTraceStart,
		WeaponEndTrace,
		ECollisionChannel::ECC_Visibility);

	if (bHasHitFromBarrel)
	{
		OutBeamLocation = WeaponTraceHit.ImpactPoint;
	}

	HasHitSomething = bCrosshairHit || bHasHitFromBarrel;
	return HasHitSomething;
}

void ARMLShooterCharacter::AimingButtonPressed()
{
	bAiming = true;

	CurrentMouseSensitivity = AimingMouseSensitivity;
}

void ARMLShooterCharacter::AimingButtonReleased()
{
	bAiming = false;

	CurrentMouseSensitivity = HipMouseSensitivity;
}

void ARMLShooterCharacter::UpdatesLooksRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void ARMLShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	// Calc Crosshair Velocity Factor
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;

	CrosshairVelocityFactor = 
		FMath::GetMappedRangeValueClamped(
			WalkSpeedRange,
			VelocityMultiplierRange,
			Velocity.Size());

	// Calc Crosshair Air Factor
	if (GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor =
			FMath::FInterpTo(
				CrosshairInAirFactor, 
				CrosshairInAirMaxFactor,
				DeltaTime,
				CrosshairInAirMultiplierInterpSpeed);
	}
	else
	{
		CrosshairInAirFactor =
			FMath::FInterpTo(
				CrosshairInAirFactor,
				0.f,
				DeltaTime,
				50.f);
	}

	// Handle Aiming crosshair factor
	if (bAiming)
	{
		CrosshairAimFactor =
			FMath::FInterpTo(
				CrosshairAimFactor,
				-0.75f,
				DeltaTime,
				10.f);
	}
	else 
	{
		CrosshairAimFactor =
			FMath::FInterpTo(
				CrosshairAimFactor,
				0.f,
				DeltaTime,
				10.f);
	}

	// Handle Shot Crosshair Spread
	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.4f,
			DeltaTime,
			90.f);;
	}
	else 
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.f,
			DeltaTime,
			60.f);;
	}

	CrosshairSpreadMultiplier = 0.5f + 
		CrosshairVelocityFactor +
		CrosshairInAirFactor +
		CrosshairAimFactor +
		CrosshairShootingFactor;

	CrosshairSpreadMultiplier =
		FMath::Max(CrosshairSpreadMultiplier, 0.f);
}

void ARMLShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager()
		.SetTimer(CrosshairShootTimer, this,
			&ARMLShooterCharacter::FinishCrosshairBulletFire,
			ShootTimeDuration);
}

void ARMLShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void ARMLShooterCharacter::FireButtonPressed()
{
	if (WeaponHasAmmo)
	{
		bFireButtonPressed = true;
		StartFireTimer();
	}
}

void ARMLShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void ARMLShooterCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;
		GetWorldTimerManager()
			.SetTimer(AutoFireTimerHandle, this,
				&ARMLShooterCharacter::AutoFireReset,
				AutomaticFireRate);
	}
}

void ARMLShooterCharacter::AutoFireReset()
{
	if (WeaponHasAmmo)
	{
		bShouldFire = true;

		if (bFireButtonPressed)
		{
			StartFireTimer();
		}
	}
}

bool ARMLShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation{ ViewportSize.X / 2.f, ViewportSize.Y / 2.f };

	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	auto bIsSucceded = UGameplayStatics::DeprojectScreenToWorld(
		GetController<APlayerController>(),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);
	
	if (bIsSucceded)
	{
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 5000.f };
		OutHitLocation = End;
		auto isHitSomething =
			GetWorld()->LineTraceSingleByChannel(
				OutHitResult,
				Start,
				End,
				ECC_Visibility);

		if (isHitSomething)
		{
			OutHitLocation = OutHitResult.ImpactPoint;
			return true;
		}
	}

	return false;
}

void ARMLShooterCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		auto isHitSomething = TraceUnderCrosshairs(ItemTraceResult, HitLocation);

		if (isHitSomething)
		{
			auto newHitItem = Cast<ARMLItem>(ItemTraceResult.GetActor());

			if (CurrentHitItem != newHitItem)
			{
				if (newHitItem != nullptr)
				{
					newHitItem->GetPickupWidget()->SetVisibility(true);
				}

				if (CurrentHitItem)
				{
					CurrentHitItem->GetPickupWidget()->SetVisibility(false);
				}

				CurrentHitItem = newHitItem;
			}
		}
	}
	else 
	{
		if (CurrentHitItem != nullptr)
		{
			CurrentHitItem->GetPickupWidget()->SetVisibility(false);
			CurrentHitItem = nullptr;
		}
	}
}

ARMLWeapon* ARMLShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		auto defaultWeapon = GetWorld()->SpawnActor<ARMLWeapon>(DefaultWeaponClass);

		return defaultWeapon;
	}

	return nullptr;
}

void ARMLShooterCharacter::EquipWeapon(ARMLWeapon* WeaponToEquip)
{
	if (WeaponToEquip)
	{
		auto const rightHandSocket = GetMesh()->GetSocketByName("RightHandSocket");

		if (rightHandSocket)
		{
			rightHandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void ARMLShooterCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();

		EquippedWeapon = nullptr;
	}
}

void ARMLShooterCharacter::SwapWeapon(ARMLWeapon* WeaponToSwap)
{
	if (WeaponToSwap)
	{
		DropWeapon();
		EquipWeapon(WeaponToSwap);
		CurrentHitItem = nullptr;
	}
}

void ARMLShooterCharacter::SelectButtonPressed()
{
	if (CurrentHitItem)
	{
		CurrentHitItem->StartItemCurve(this);
	}
}

void ARMLShooterCharacter::SelectButtonReleased()
{

}

void ARMLShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool ARMLShooterCharacter::WeaponHasAmmo()
{
	if (!EquippedWeapon)
	{
		return false;
	}

	return EquippedWeapon->GetAmmo() > 0;
}

ARMLWeapon* ARMLShooterCharacter::GetEquippedWeapon()
{
	return EquippedWeapon;
}

// Called every frame
void ARMLShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle camera fov when we aiming or not
	HandleCameraAimingFov(DeltaTime);

	UpdatesLooksRates();

	CalculateCrosshairSpread(DeltaTime);

	// Trace for interectable items
	TraceForItems();
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

	PlayerInputComponent->BindAction(TEXT("FireButton"), IE_Pressed, this, &ARMLShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("FireButton"), IE_Released, this, &ARMLShooterCharacter::FireButtonReleased);
	
	PlayerInputComponent->BindAction
	(TEXT("AimingButton"), IE_Pressed, this, &ARMLShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction
	(TEXT("AimingButton"), IE_Released, this, &ARMLShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction
	(TEXT("Select"), IE_Pressed, this, &ARMLShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction
	(TEXT("Select"), IE_Released, this, &ARMLShooterCharacter::SelectButtonReleased);
}

USpringArmComponent* ARMLShooterCharacter::GetCameraBoom() const
{
	return CameraSpringArm;
}

UCameraComponent* ARMLShooterCharacter::GetFollowCamera() const
{
	return FollowCamera;
}

bool ARMLShooterCharacter::GetAiming() const
{
	return bAiming;
}

float ARMLShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void ARMLShooterCharacter::IncrementOverlappedItemCount(int32 Amount)
{
	OverlappedItemCount = FMath::Max(0, OverlappedItemCount + Amount);
	bShouldTraceForItems = OverlappedItemCount > 0;
}

FVector ARMLShooterCharacter::GetCameraInterpLocation()
{
	auto const CameraLocation = GetFollowCamera()->GetComponentLocation();
	auto const CameraForwardDirection = GetFollowCamera()->GetForwardVector();
	auto const CameraUpDirection = GetFollowCamera()->GetUpVector();

	return FVector(CameraLocation 
		+ CameraForwardDirection * CameraForwardDistance
		+ CameraUpDirection * CameraUpDistance);
}

void ARMLShooterCharacter::GetPickupItem(ARMLItem* Item)
{
	auto Weapon = Cast<ARMLWeapon>(Item);

	if (Weapon)
	{
		SwapWeapon(Weapon);
	}
}

void ARMLShooterCharacter::HandleCameraAimingFov(float DeltaTime)
{
	float CurrentUsedFOV = bAiming ? CameraZoomedFOV : CameraDefaultFOV;

	CameraCurrentFOV =
		FMath::FInterpTo(CameraCurrentFOV, CurrentUsedFOV, DeltaTime, ZoomInterpSpeed);

	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

