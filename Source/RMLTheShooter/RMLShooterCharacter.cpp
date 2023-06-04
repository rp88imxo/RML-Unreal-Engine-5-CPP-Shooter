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
#include "Components/CapsuleComponent.h"

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
	AutomaticFireRate {60.f / 650.f},
	bShouldFire{true},
	bFireButtonPressed{false},
	bShouldTraceForItems{false},
	CameraForwardDistance {250.f},
	CameraUpDistance {50.f},
	Starting9mmAmmo{36},
	StartingARAmmo{180},
	CombatState{ECombatState::EAS_Unoccupied},
	bAutoReloadWeapon {true},
	bCrouching{false},
	BaseMovementSpeed {600.f},
	CrouchMovementSpeed {450.f},
	StandingCapsuleHalfHeight {88.f},
	CrouchingCapsuleHalfHeight {44.f}
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

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));
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

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
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

void ARMLShooterCharacter::PlayFireSound()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void ARMLShooterCharacter::SendBullet()
{

	const auto BarrelSocket =
		EquippedWeapon->GetItemMesh()->GetSocketByName("RMLBarrelSocket");

	const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached
		(MuzzleFlash,
			EquippedWeapon->GetItemMesh(),
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
}

void ARMLShooterCharacter::PlayGunfireMontage()
{
	PlayMontage(HipFireMontage, TEXT("StartFire"));
}

void ARMLShooterCharacter::PlayReloadMontage()
{
	FName SectionName = EquippedWeapon->GetReloadMontageSectionName();
	PlayMontage(ReloadMontage, SectionName);
}

void ARMLShooterCharacter::PlayMontage(UAnimMontage* MontageToPlay, FName SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		if (!SectionName.IsNone())
		{
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ARMLShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void ARMLShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::EAS_Unoccupied)
	{
		return;
	}

	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if (bAiming)
		{
			StopAiming();
		}

		CombatState = ECombatState::EAS_Reloading;
		PlayReloadMontage();
	}
}

void ARMLShooterCharacter::HandleFinishReloading()
{
	CombatState = ECombatState::EAS_Unoccupied;

	if (bAimingButtonPressed)
	{
		Aim();
	}

	const auto CurrentAmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(CurrentAmmoType))
	{
		int32 CarriedAmmo = AmmoMap[CurrentAmmoType];

		// Space Left in the magazine
		const int32 MagEmptySpace =
			EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace >= CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(CurrentAmmoType, CarriedAmmo);
		}
		else 
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(CurrentAmmoType, CarriedAmmo);
		}
	}

	//if (AmmoMap[CurrentAmmoType] >= EquippedWeapon->GetMagazineCapacity())
	//{
	//	AmmoMap[CurrentAmmoType] = AmmoMap[CurrentAmmoType] - EquippedWeapon->GetMagazineCapacity();
	//	// TODO: Setup Current Ammo to magazine capacity
	//}
	//else 
	//{
	//	// TODO: Setup current ammo count to left AmmoMap[CurrentAmmoType]
	//}
}

bool ARMLShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon)
	{
		auto AmmoType = EquippedWeapon->GetAmmoType();

		if (AmmoMap.Contains(AmmoType))
		{
			return AmmoMap[AmmoType] > 0;
		}
	}

	return false;
}

void ARMLShooterCharacter::GrabClip()
{
	if (EquippedWeapon)
	{
		int32 ClipBoneIndex
		{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };

		ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
		HandSceneComponent->AttachToComponent(
			GetMesh(),
			AttachmentRules,
			FName(TEXT("Hand_L")));
		HandSceneComponent->SetWorldTransform(ClipTransform);

		EquippedWeapon->SetMovingClip(true);
	}
}


void ARMLShooterCharacter::ReleaseClip()
{
	UE_LOG(LogTemp, Warning, TEXT("Called Release Clip!"));
	EquippedWeapon->SetMovingClip(false);
}

void ARMLShooterCharacter::CrouchButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}

	GetCharacterMovement()->MaxWalkSpeed = bCrouching ? CrouchMovementSpeed : BaseMovementSpeed;
}

void ARMLShooterCharacter::HandleJumpPressed()
{
	if (bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else 
	{
		Super::Jump();
	}
}

void ARMLShooterCharacter::HandleJumpReleased()
{
	Super::StopJumping();
}

void ARMLShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight{ 0.f };
	if (bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else 
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}

	const float InterpHalfHeight{ FMath::FInterpTo(
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
		TargetCapsuleHalfHeight,
		DeltaTime,
		20.f) };

	const float DeltaCapsuleHalfHeight{ InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };
	const FVector MeshOffset{ 0.f, 0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);
	
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void ARMLShooterCharacter::Aim()
{
	bAiming = true;

	CurrentMouseSensitivity = AimingMouseSensitivity;

	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void ARMLShooterCharacter::StopAiming()
{
	bAiming = false;

	CurrentMouseSensitivity = HipMouseSensitivity;

	if (!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}


void ARMLShooterCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (CombatState != ECombatState::EAS_Unoccupied)
	{
		return;
	}

	if (WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunfireMontage();

		// indicates that we shooted so we can spread a crosshair for some amount of time
		StartCrosshairBulletFire();

		EquippedWeapon->DecrementAmmo();

		StartFireTimer();
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
	bAimingButtonPressed = true;

	if (CombatState != ECombatState::EAS_Reloading)
	{
		Aim();
	}
}

void ARMLShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;

	StopAiming();
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
	bFireButtonPressed = true;
	/*if (WeaponHasAmmo())
	{
		StartFireTimer();
	}*/
	FireWeapon();
}

void ARMLShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void ARMLShooterCharacter::StartFireTimer()
{
	CombatState = ECombatState::EAS_FireTimerInProgress;

	GetWorldTimerManager()
		.SetTimer(AutoFireTimerHandle, this,
			&ARMLShooterCharacter::AutoFireReset,
			AutomaticFireRate);

}

void ARMLShooterCharacter::AutoFireReset()
{
	CombatState = ECombatState::EAS_Unoccupied;

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed)
		{
			FireWeapon();
		}
	}
	else 
	{
		if (bAutoReloadWeapon)
		{
			ReloadWeapon();
		}
	}

	/*if (WeaponHasAmmo())
	{
		bShouldFire = true;

		if (bFireButtonPressed)
		{
			StartFireTimer();
		}
	}*/
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

		if (CurrentHitItem->GetPickupSound())
		{
			UGameplayStatics::PlaySound2D(this, CurrentHitItem->GetPickupSound());
		}
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

	// Interp Capsule Half height based on Crouching/Standing state
	InterpCapsuleHalfHeight(DeltaTime);
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

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ARMLShooterCharacter::HandleJumpPressed);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ARMLShooterCharacter::HandleJumpReleased);

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

	PlayerInputComponent->BindAction
	(TEXT("ReloadButton"), IE_Pressed, this, &ARMLShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction
	(TEXT("Crouch"), IE_Pressed, this, &ARMLShooterCharacter::CrouchButtonPressed);
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
	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}

	auto Weapon = Cast<ARMLWeapon>(Item);

	if (Weapon)
	{
		SwapWeapon(Weapon);
	}
}

int32 ARMLShooterCharacter::GetAmmoInMag() const
{
	if (EquippedWeapon)
	{
		return EquippedWeapon->GetAmmo();
	}

	return 0;
}

void ARMLShooterCharacter::HandleCameraAimingFov(float DeltaTime)
{
	float CurrentUsedFOV = bAiming ? CameraZoomedFOV : CameraDefaultFOV;

	CameraCurrentFOV =
		FMath::FInterpTo(CameraCurrentFOV, CurrentUsedFOV, DeltaTime, ZoomInterpSpeed);

	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

