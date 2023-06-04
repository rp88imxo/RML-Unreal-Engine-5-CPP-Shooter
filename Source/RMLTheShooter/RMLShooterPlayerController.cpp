// Fill out your copyright notice in the Description page of Project Settings.


#include "RMLShooterPlayerController.h"
#include "Blueprint/UserWidget.h"

ARMLShooterPlayerController::ARMLShooterPlayerController()
{

}

void ARMLShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if (HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}
