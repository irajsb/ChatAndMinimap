// Copyright 2024 Iraj Mohtasham aurelion.net 


#include "MapSystem/MapVolume.h"

#include "Components/SceneCaptureComponent2D.h"

AMapVolume::AMapVolume()
{
	GetCaptureComponent2D()->ProjectionType=ECameraProjectionMode::Orthographic;
	
}
