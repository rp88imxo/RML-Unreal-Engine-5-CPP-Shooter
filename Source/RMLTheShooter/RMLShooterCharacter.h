// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RMLShooterCharacter.generated.h"

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

	/// <summary>
	/// Mouse rotation sensitivity
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseMouseRotationSensitivity;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const;
};
