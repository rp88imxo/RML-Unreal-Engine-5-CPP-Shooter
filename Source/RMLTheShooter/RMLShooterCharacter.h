// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RMLShooterCharacter.generated.h"

UENUM(BlueprintType)
enum class EAmmoType : uint8 
{
	EAT_9mm UMETA(DisplayName = "9mm"),
	EAT_AR UMETA(DisplayName = "AssaultRifle"),
	EAT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class RMLTHESHOOTER_API ARMLShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARMLShooterCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void HandleMoveForward(float Value);
	void HandleMoveRight(float Value);

	// For controller
	void HandleTurnRate(float Rate);

	void HandleLookUpRate(float Rate);

	// For mouse
	void HandleTurn(float Value);
	void HandleLookUp(float Value);

	void FireWeapon();

	bool GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation, bool& HasHitSomething);

	// Aiming
	void AimingButtonPressed();
	void AimingButtonReleased();

	void UpdatesLooksRates();

	void CalculateCrosshairSpread(float DeltaTime);

	void StartCrosshairBulletFire();

	UFUNCTION()
	void FinishCrosshairBulletFire();

	void FireButtonPressed();
	void FireButtonReleased();
	void StartFireTimer();
	/// <summary>
	/// AutoFireReset is a callback for our timer so should have a UFUNCTION macro
	/// </summary>
	UFUNCTION()
	void AutoFireReset();

	/// <summary>
	/// Line Trace for items under Crosshair
	/// </summary>
	/// <param name="OutHitResult">FHitResult will be filled with info of hitted item</param>
	/// <returns></returns>
	bool TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation);

	void TraceForItems();

	class ARMLWeapon* SpawnDefaultWeapon();

	/// <summary>
	/// Takes a weapon and attaches it to the mesh
	/// </summary>
	/// <param name="WeaponToEquip">Weapon to equip</param>
	void EquipWeapon(ARMLWeapon* WeaponToEquip);

	/// <summary>
	/// Detach a weapon from the character and drop on the ground
	/// </summary>
	void DropWeapon();

	void SwapWeapon(ARMLWeapon* WeaponToSwap);

	void SelectButtonPressed();
	void SelectButtonReleased();

	void InitializeAmmoMap();

	bool WeaponHasAmmo();

	class ARMLWeapon* GetEquippedWeapon();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	/// <summary>
	/// Camera boom that follow the character
	/// </summary>
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/// <summary>
	/// Base Turn Rate, in deg/sec
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	/// <summary>
	/// Base Look Up Rate, in deg/sec, which affects up and down rotation
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseLookUpRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipTurnRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingLookUpRate;

	/// <summary>
	/// Mouse rotation sensitivity
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipMouseSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingMouseSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class USoundCue* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HipFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* ImpactHitParticles;

	/// <summary>
	/// Beam particles is a smoke trail of the bullet being shot
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* BeamParticles;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	/// <summary>
	/// Default Camera FOV when we are NOT zoomed in
	/// </summary>
	float CameraDefaultFOV;

	/// <summary>
	/// Camera Zoom when we zoomed in (when we aimed in)
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float CameraZoomedFOV;

	float CameraCurrentFOV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed;

	float CurrentMouseSensitivity;
	
#pragma region CROSSHAIR_PARAMS
	/// <summary>
	/// Determines The Spread Of Crosshairs
	/// </summary>
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor;

	/// <summary>
	/// Max Amount of crosshair inair spread multiplier we interp to
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirMaxFactor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirMultiplierInterpSpeed;

#pragma endregion

	float ShootTimeDuration;
	bool bFiringBullet;
	FTimerHandle CrosshairShootTimer;

#pragma region AUTOMATIC_FIRE
	bool bFireButtonPressed;

	/// <summary>
	/// False when we are waiting for timer
	/// </summary>
	bool bShouldFire;

	float AutomaticFireRate;
	/// <summary>
	/// Timer between a gun shots
	/// </summary>
	FTimerHandle AutoFireTimerHandle;

#pragma endregion

	/// <summary>
	/// Indicated if we should perform a trace every frame (happens if we overlap at least one area sphere of the item)
	/// </summary>
	bool bShouldTraceForItems;
	int32 OverlappedItemCount;

	class ARMLItem* CurrentHitItem;

	/// <summary>
	/// Currently Equipped Weapon
	/// </summary>
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	ARMLWeapon* EquippedWeapon;

	/// <summary>
	/// Default Weapon class to spawn on character
	/// </summary>
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ARMLWeapon> DefaultWeaponClass;

#pragma region CAMERA_INTERP_PARAMS

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Item Location", meta = (AllowPrivateAccess = "true"))
	float CameraForwardDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Item Location", meta = (AllowPrivateAccess = "true"))
	float CameraUpDistance;

#pragma endregion

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmo;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const;
	FORCEINLINE bool GetAiming() const;
	
	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplier() const;

	FORCEINLINE int32 GetOverlappedItemCount() const {return OverlappedItemCount;}

	void IncrementOverlappedItemCount(int32 Amount);

	FVector GetCameraInterpLocation();

	void GetPickupItem(ARMLItem* Item);
private:
	void HandleCameraAimingFov(float DeltaTime);
};
