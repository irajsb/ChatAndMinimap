// Copyright 2024 Iraj Mohtasham aurelion.net 

#include "Slate/STitanWidgetScreenLayer.h"

#include "Widgets/Layout/SBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/TitanWidgetComponent.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetMathLibrary.h"
#include "Widgets/SViewport.h"
#include "Slate/SGameLayerManager.h"
#include "SceneView.h"
#include "Runtime/Launch/Resources/Version.h"

static int32 GSlateWorldWidgetZOrder = 1;
static FAutoConsoleVariableRef CVarSlateWorldWidgetZOrder(
	TEXT("Slate.WorldWidgetZOrder"),
	GSlateWorldWidgetZOrder,
	TEXT("Whether to re-order world widgets projected to screen by their view point distance\n")
	TEXT(" 0: Disable re-ordering\n")
	TEXT(" 1: Re-order by distance (default, less batching, less artifacts when widgets overlap)"),
	ECVF_Default
);

STitanWorldWidgetScreenLayer::FComponentEntry::FComponentEntry()
	: WidgetComponent(nullptr),Slot(nullptr)
{
}

STitanWorldWidgetScreenLayer::FComponentEntry::~FComponentEntry()
{
	Widget.Reset();
	ContainerWidget.Reset();
}

void STitanWorldWidgetScreenLayer::Construct(const FArguments& InArgs, const FLocalPlayerContext& InPlayerContext)
{
	PlayerContext = InPlayerContext;

	bCanSupportFocus = false;
	DrawSize = FVector2D(0, 0);
	Pivot = FVector2D(0.5f, 0.5f);

	ChildSlot
	[
		SAssignNew(Canvas, SConstraintCanvas)
	];
}

void STitanWorldWidgetScreenLayer::SetWidgetDrawSize(FVector2D InDrawSize)
{
	DrawSize = InDrawSize;
}

void STitanWorldWidgetScreenLayer::SetWidgetPivot(FVector2D InPivot)
{
	Pivot = InPivot;
}

void STitanWorldWidgetScreenLayer::AddComponent(USceneComponent* Component, TSharedRef<SWidget> Widget)
{
	if (Component)
	{
		FComponentEntry& Entry = ComponentMap.FindOrAdd(FObjectKey(Component));
		Entry.Component = Component;
		Entry.WidgetComponent = Cast<UTitanWidgetComponent>(Component);
		Entry.Widget = Widget;

		Canvas->AddSlot()
		      .Expose(Entry.Slot)
		[
			SAssignNew(Entry.ContainerWidget, SBox)
			[
				Widget
			]
		];
	}
}

void STitanWorldWidgetScreenLayer::RemoveComponent(const USceneComponent* Component)
{
	if (ensure(Component))
	{
		if (FComponentEntry* EntryPtr = ComponentMap.Find(Component))
		{
			if (!EntryPtr->bRemoving)
			{
				RemoveEntryFromCanvas(*EntryPtr);
				ComponentMap.Remove(Component);
			}
		}
	}
}

FVector2D STitanWorldWidgetScreenLayer::FindPointOnRect(float Width, float Height, float AngleRadians,
                                                        FVector2D InDrawSize)
{
	const float HalfWidth = Width / 2.0;
	const float HalfHeight = Height / 2.0;

	float Cos = FMath::Cos(AngleRadians);
	float Sin = FMath::Sin(AngleRadians);

	if (FMath::Abs(Sin) < FMath::Abs(Cos))
	{
		Cos = FMath::Sign(Cos);
	}
	else
	{
		Sin = FMath::Sign(Sin);
	}
	// Calculate the coordinates of the point on the rectangle
	const float X = (HalfWidth - InDrawSize.X / 2) * Cos + HalfWidth;
	const float Y = (HalfHeight - InDrawSize.Y / 2) * Sin + HalfHeight;
	return FVector2D(X, Y);
}



void STitanWorldWidgetScreenLayer::FindOffscreenLocation(const APlayerController* PlayerController, const FComponentEntry& Entry,
                                                         const FGeometry& AllottedGeometry)
{
	const FRotator CamRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(
		PlayerController->PlayerCameraManager->GetCameraLocation(), Entry.WidgetComponent->GetComponentLocation());


	TargetRotation = (TargetRotation.Quaternion() * FRotator(-CamRotation.Pitch, 0, 0).Quaternion()).Rotator();


	FVector Result = UKismetMathLibrary::ProjectVectorOnToPlane(TargetRotation.Vector(), CamRotation.Vector());

	Result = CamRotation.UnrotateVector(Result);
	
	FVector2D NormalizeAngle = FVector2D(Result.Y, Result.Z);
	NormalizeAngle.Normalize();
	const float DotProduct = FVector2D::DotProduct(NormalizeAngle, FVector2D(1, 0));
	const float CrossProduct = FVector2D::CrossProduct(NormalizeAngle, FVector2D(1, 0));
	const float Angle = FMath::Atan2(CrossProduct, DotProduct);


	if (SConstraintCanvas::FSlot* CanvasSlot = Entry.Slot)
	{
		const FVector2D ComponentDrawSize = Entry.WidgetComponent->GetDrawSize();
		const FVector2D ComponentPivot = Entry.WidgetComponent->GetPivot();
		CanvasSlot->SetAutoSize(ComponentDrawSize.IsZero() || Entry.WidgetComponent->GetDrawAtDesiredSize());

		FVector2D LocalPosition = AllottedGeometry.GetLocalSize();
		LocalPosition = FindPointOnRect(LocalPosition.X, LocalPosition.Y, Angle, ComponentDrawSize);
		Entry.WidgetComponent->OutOfBoundsAngle=FMath::RadiansToDegrees(Angle);
		Entry.WidgetComponent->IsOutOfBounds=true;
		CanvasSlot->SetOffset(FMargin(LocalPosition.X, LocalPosition.Y, ComponentDrawSize.X, ComponentDrawSize.Y));
		CanvasSlot->SetAnchors(FAnchors(0, 0, 0, 0));
		CanvasSlot->SetAlignment(ComponentPivot);
		
	}
}

void STitanWorldWidgetScreenLayer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
                                        const float InDeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STitanWorldWidgetScreenLayer_Tick);

	if (APlayerController* PlayerController = PlayerContext.GetPlayerController())
	{
		if (UGameViewportClient* ViewportClient = PlayerController->GetWorld()->GetGameViewport())
		{
			const FGeometry& ViewportGeometry = ViewportClient->GetGameLayerManager()->GetViewportWidgetHostGeometry();

			// cache projection data here and avoid calls to UWidgetLayoutLibrary.ProjectWorldLocationToWidgetPositionWithDistance
			FSceneViewProjectionData ProjectionData;
			FMatrix ViewProjectionMatrix;
			bool bHasProjectionData = false;

			const ULocalPlayer* const LP = PlayerController->GetLocalPlayer();
			if (LP && LP->ViewportClient)
			{
#if ENGINE_MAJOR_VERSION>4
				bHasProjectionData = LP->GetProjectionData(ViewportClient->Viewport, /*out*/ ProjectionData);
#else
				bHasProjectionData = LP->GetProjectionData(ViewportClient->Viewport, eSSP_FULL, /*out*/ ProjectionData);
#endif
				
				if (bHasProjectionData)
				{
					ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
				}
			}

			for (auto It = ComponentMap.CreateIterator(); It; ++It)
			{
				FComponentEntry& Entry = It.Value();

				if (USceneComponent* SceneComponent = Entry.Component.Get())
				{
					FVector2D ComponentDrawSize = Entry.WidgetComponent->GetDrawSize();
					FVector WorldLocation = SceneComponent->GetComponentLocation();

					FVector2D ScreenPosition2D=FVector2D::ZeroVector;
					const bool bProjected = bHasProjectionData
						                        ? FSceneView::ProjectWorldToScreen(
							                        WorldLocation, ProjectionData.GetConstrainedViewRect(),
							                        ViewProjectionMatrix, ScreenPosition2D)
						                        : false;
					if (bProjected)
					{
						const float ViewportDist = FVector::Dist(ProjectionData.ViewOrigin, WorldLocation);
						const FVector2D RoundedPosition2D(FMath::RoundToInt(ScreenPosition2D.X),
						                                  FMath::RoundToInt(ScreenPosition2D.Y));
						FVector2D ViewportPosition2D;
						USlateBlueprintLibrary::ScreenToViewport(PlayerController, RoundedPosition2D,
						                                         ViewportPosition2D);
						int X, Y;
						X = AllottedGeometry.GetLocalSize().X;
						Y = AllottedGeometry.GetLocalSize().Y;
						const FVector ViewportPosition(ViewportPosition2D.X, ViewportPosition2D.Y, ViewportDist);
						const FVector2D BorderCheck = ComponentDrawSize / 2;
						if ((ViewportPosition2D.X > BorderCheck.X && ViewportPosition2D.X < X - BorderCheck.X &&
							ViewportPosition2D.Y > BorderCheck.Y && ViewportPosition2D.Y < Y - BorderCheck.Y))
						{
							Entry.ContainerWidget->SetVisibility(EVisibility::SelfHitTestInvisible);

							if (SConstraintCanvas::FSlot* CanvasSlot = Entry.Slot)
							{
								FVector2D AbsoluteProjectedLocation = ViewportGeometry.LocalToAbsolute(
									FVector2D(ViewportPosition.X, ViewportPosition.Y));
								FVector2D LocalPosition = AllottedGeometry.AbsoluteToLocal(AbsoluteProjectedLocation);
								Entry.WidgetComponent->IsOutOfBounds=false;
								if (Entry.WidgetComponent)
								{
									LocalPosition = Entry.WidgetComponent->ModifyProjectedLocalPosition(
										ViewportGeometry, LocalPosition);


									FVector2D ComponentPivot = Entry.WidgetComponent->GetPivot();
									CanvasSlot->SetAutoSize(
										ComponentDrawSize.IsZero() || Entry.WidgetComponent->GetDrawAtDesiredSize());
									CanvasSlot->SetOffset(FMargin(LocalPosition.X, LocalPosition.Y, ComponentDrawSize.X,
									                           ComponentDrawSize.Y));
									
									CanvasSlot->SetAnchors(FAnchors(0, 0, 0, 0));
									CanvasSlot->SetAlignment(ComponentPivot);

									if (GSlateWorldWidgetZOrder != 0)
									{
										CanvasSlot->SetZOrder(-ViewportPosition.Z);
									}
								}
								else
								{
									CanvasSlot->SetAutoSize(DrawSize.IsZero());
									CanvasSlot->SetOffset(
										FMargin(LocalPosition.X, LocalPosition.Y, DrawSize.X, DrawSize.Y));
									CanvasSlot->SetAnchors(FAnchors(0, 0, 0, 0));
									CanvasSlot->SetAlignment(Pivot);

									if (GSlateWorldWidgetZOrder != 0)
									{
										CanvasSlot->SetZOrder(-ViewportPosition.Z);
									}
								}
							}
						}
						else
						{
							FindOffscreenLocation(PlayerController, Entry, AllottedGeometry);
						}
					}
					if (!bProjected)
					{
						FindOffscreenLocation(PlayerController, Entry, AllottedGeometry);
					}
				}
				else
				{
					RemoveEntryFromCanvas(Entry);
					It.RemoveCurrent();
				}
			}

			// Done
			return;
		}
	}

	if (GSlateIsOnFastUpdatePath)
	{
		// Hide everything if we are unable to do any of the work.
		for (auto It = ComponentMap.CreateIterator(); It; ++It)
		{
			FComponentEntry& Entry = It.Value();
			Entry.ContainerWidget->SetVisibility(EVisibility::Collapsed);
		}
	}
}

void STitanWorldWidgetScreenLayer::RemoveEntryFromCanvas(FComponentEntry& Entry) const
{
	// Mark the component was being removed, so we ignore any other remove requests for this component.
	Entry.bRemoving = true;

	if (const TSharedPtr<SWidget> ContainerWidget = Entry.ContainerWidget)
	{
		Canvas->RemoveSlot(ContainerWidget.ToSharedRef());
	}
}

FVector2D STitanWorldWidgetScreenLayer::ComputeDesiredSize(float) const
{
	return FVector2D(0, 0);
}
