#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "CrowdAnimFragment.generated.h"

USTRUCT()
struct FCrowdAnimFragment : public FMassFragment
{
	GENERATED_BODY()

	// Phase offset in seconds (random per entity) — decorrelates instance anims.
	UPROPERTY()
	float TimeOffset = 0.f;

	// Accumulated anim time in seconds. Pushed to ISM custom data 0.
	UPROPERTY()
	float CurrentTime = 0.f;

	// 0 = idle pose, 1 = full walk speed. Pushed to ISM custom data 1.
	UPROPERTY()
	float WalkBlend = 0.f;
};

class UAnimToTextureDataAsset;

USTRUCT()
struct FCrowdAnimParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	// The ATL bake to read animation timing from. Walk is anim index 0, idle is index 1.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	TObjectPtr<UAnimToTextureDataAsset> DataAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	int32 WalkAnimIndex = 0;

	// Below this speed, blend toward idle.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	float WalkSpeedThresholdCmPerSec = 100.f;

	// Walk-vs-idle blend ease rate (1/sec).
	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	float BlendRate = 4.f;

	// Locomotion speed of the baked walk anim. CurrentTime advances at
	// speed/AnimNominalSpeed to cancel foot sliding.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	float AnimNominalSpeedCmPerSec = 150.f;
};
