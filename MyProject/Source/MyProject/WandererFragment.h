#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "WandererFragment.generated.h"

USTRUCT()
struct FWandererFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	float SpeedJitter = 0.f;

	UPROPERTY()
	float TimeUntilReroll = 0.f;
};

USTRUCT()
struct FWandererParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Wander")
	float MaxJitterCmPerSec = 40.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float RerollIntervalSec = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float BaseSpeedCmPerSec = 150.f;
};
