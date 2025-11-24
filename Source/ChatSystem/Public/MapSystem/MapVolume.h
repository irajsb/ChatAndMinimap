// Copyright 2024 Iraj Mohtasham aurelion.net 

#pragma once

#include "CoreMinimal.h"
#include "Engine/SceneCapture2D.h"
#include "MapVolume.generated.h"

/**
 * Class to automatically capture a volume
 */
UCLASS()
class CHATSYSTEM_API AMapVolume : public ASceneCapture2D
{
	GENERATED_BODY()
	AMapVolume();

};
