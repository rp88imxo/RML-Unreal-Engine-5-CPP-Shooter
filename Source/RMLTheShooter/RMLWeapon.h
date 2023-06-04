// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RMLItem.h"
#include "RMLWeapon.generated.h"

/**
 * 
 */
UCLASS()
class RMLTHESHOOTER_API ARMLWeapon : public ARMLItem
{
	GENERATED_BODY()
public:
	ARMLWeapon();

	virtual void Tick(float DeltaTime) override;

protected:
	void StopFalling();
private:
	FTimerHandle ThrowWeaponTimer;
	float ThrowWeaponTime;
	bool bFalling;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo;

public:
	void ThrowWeapon();
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	void DecrementAmmo();
};
