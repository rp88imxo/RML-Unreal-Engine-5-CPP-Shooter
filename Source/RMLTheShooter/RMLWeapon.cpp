// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLWeapon.h"

ARMLWeapon::ARMLWeapon():
	ThrowWeaponTime(0.7f),
	bFalling(false),
	Ammo{0},
	WeaponType {EWeaponType::EWT_SubmachineGun},
	AmmoType{EAmmoType::EAT_9mm},
	ReloadMontageSection{TEXT("ReloadSMG")},
	MagazineCapacity{30},
	bMovingClip {false},
	ClipBoneName {TEXT("smg_clip")}
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARMLWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// May be better to add contraint to physic???
	if (GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ARMLWeapon::ThrowWeapon()
{
	FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward{ GetItemMesh()->GetForwardVector() };
	const FVector MeshRight{ GetItemMesh()->GetRightVector() };
	auto ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	float RandomRotation{ FMath::FRandRange(-30.f, 30.f)};
	ImpulseDirection = MeshRight.RotateAngleAxis(RandomRotation, FVector::UpVector);
	ImpulseDirection *= 20'000.f;

	GetItemMesh()->AddImpulse(ImpulseDirection);

	bFalling = true;
	GetWorldTimerManager()
		.SetTimer(
			ThrowWeaponTimer,
			this, 
			&ARMLWeapon::StopFalling,
			ThrowWeaponTime);
}

void ARMLWeapon::DecrementAmmo()
{
	Ammo = FMath::Max(0, Ammo - 1);
}

void ARMLWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, 
		TEXT("Attempted to add amount which leads to magazine capacity overflow"));
	Ammo += Amount;
}

bool ARMLWeapon::ClipIsFull() const
{
	return Ammo >= MagazineCapacity;
}

void ARMLWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
}
