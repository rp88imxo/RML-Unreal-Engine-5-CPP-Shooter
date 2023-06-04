// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLItem.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "RMLShooterCharacter.h"
#include "Camera/CameraComponent.h"

// Sets default values
ARMLItem::ARMLItem() :
	ItemName{ FString("Not Set") },
	ItemCount{ 0 },
	ItemRarity{ EItemRarity::EIR_Common },
	ItemState{ EItemState::EIS_Pickup },
	ZCurveTime{ 0.7f },
	ItemInterpStartLocation{ FVector::ZeroVector },
	CameraTargetLocation{ FVector::ZeroVector },
	bInterping{ false },
	ItemInterpX{0.f},
	ItemInterpY{0.f},
	InterpInitialYawOffset{0.f}
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Item Mesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
	PickupWidget->SetupAttachment(RootComponent);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Area Sphere"));
	AreaSphere->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ARMLItem::BeginPlay()
{
	Super::BeginPlay();

	// Hide a pickup widget by default
	PickupWidget->SetVisibility(false);

	InitActiveStars();

	// Setup overlap for AreaSphere
	AreaSphere->OnComponentBeginOverlap.AddDynamic(
		this, &ARMLItem::OnAreaSphereBeginOverlap);

	AreaSphere->OnComponentEndOverlap.AddDynamic(
		this, &ARMLItem::OnAreaSphereEndOverlap);

	SetItemProperties(ItemState);
}

void ARMLItem::OnAreaSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool fromSweep,
	const FHitResult& SweepResult)
{
	TryChangeOverlapAmount(OtherActor, 1);
}

void ARMLItem::OnAreaSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	TryChangeOverlapAmount(OtherActor, -1);
}

void ARMLItem::InitActiveStars()
{
	for (size_t i = 0; i <= 5; i++)
	{
		ActiveStars.Add(false);
	}

	int starsAmount;
	switch (ItemRarity)
	{
	case EItemRarity::EIR_Damaged:
		starsAmount = 1;
		break;
	case EItemRarity::EIR_Common:
		starsAmount = 2;
		break;
	case EItemRarity::EIR_Uncommon:
		starsAmount = 3;
		break;
	case EItemRarity::EIR_Rare:
		starsAmount = 4;
		break;
	case EItemRarity::EIR_Legendary:
		starsAmount = 5;
		break;
	case EItemRarity::EIR_MAX:
		starsAmount = 5;
		break;
	default:
		break;
	}

	for (int i = 1; i <= starsAmount; i++)
	{
		ActiveStars[i] = true;
	}
}

void ARMLItem::SetItemProperties(EItemState CurrentState)
{
	switch (CurrentState)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel
		(ECC_Visibility, ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		break;
	case EItemState::EIS_EquipInterping:
		PickupWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		break;
	case EItemState::EIS_PickedUp:
		break;
	case EItemState::EIS_Equipped:
		PickupWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		break;
	case EItemState::EIS_Falling:
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		ItemMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		break;
	case EItemState::EIS_MAX:
		break;
	default:
		break;
	}
}

void ARMLItem::HandleFinishInterping()
{
	bInterping = false;
	if (ShooterCharacter)
	{
		ShooterCharacter->GetPickupItem(this);
	}

	SetActorScale3D(FVector(1.f));
}

void ARMLItem::ItemInterp(float DeltaTime)
{
	if (!bInterping)
	{
		return;
	}

	if (ShooterCharacter && ItemZCurve)
	{
		const float ElapsedTime =
			GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

		FVector ItemLocation = ItemInterpStartLocation;
		const FVector CameraInterpLocation{ ShooterCharacter->GetCameraInterpLocation() };

		const FVector ItemToCamera{ FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z) };
		const float DeltaZ = ItemToCamera.Size();

		FVector CurrentLocation{ GetActorLocation() };
		const float InterpXValue = 
			FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 300.f);
		const float InterpYValue =
			FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 300.f);
		
		ItemLocation.Z += CurveValue * DeltaZ;
		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;

		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

		const FRotator CameraRotation{ ShooterCharacter->GetFollowCamera()->GetComponentRotation() };
		FRotator ItemRotator{ 0.f,CameraRotation.Yaw + InterpInitialYawOffset,0.f };
		SetActorRotation(ItemRotator, ETeleportType::TeleportPhysics);

		if (ItemScaleCurve)
		{
			const float ScaleItemValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleItemValue));
		}
	}
}

// Called every frame
void ARMLItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);
}

void ARMLItem::SetItemState(EItemState newItemState)
{
	ItemState = newItemState;
	SetItemProperties(ItemState);
}

void ARMLItem::StartItemCurve(ARMLShooterCharacter* CharacterToMove)
{
	ShooterCharacter = CharacterToMove;
	
	// Init a initial location of the item
	ItemInterpStartLocation = GetActorLocation();

	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);

	GetWorldTimerManager().SetTimer(
		ItemInterpTimer,
		this,
		&ARMLItem::HandleFinishInterping,
		ZCurveTime);

	const float CameraYaw = ShooterCharacter->GetFollowCamera()->GetComponentRotation().Yaw;
	const float ItemYaw = GetActorRotation().Yaw;

	InterpInitialYawOffset = ItemYaw - CameraYaw;
}

bool ARMLItem::TryChangeOverlapAmount(AActor* OtherActor, int amount)
{
	if (OtherActor)
	{
		auto Character = Cast<ARMLShooterCharacter>(OtherActor);

		if (Character)
		{
			Character->IncrementOverlappedItemCount(amount);
			return true;
		}
	}

	return false;
}

