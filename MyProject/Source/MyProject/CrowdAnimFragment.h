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

class UCrowdBakerDataAsset;

USTRUCT()
struct FCrowdAnimParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	// The CrowdBaker bake to read animation timing from. Walk is anim index 0, idle is index 1.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	TObjectPtr<UCrowdBakerDataAsset> DataAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	int32 WalkAnimIndex = 0;

	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	int32 IdleAnimIndex = 1;

	// If WalkBlend > this threshold, the walk anim plays; otherwise the idle anim.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkBlendThreshold = 0.25f;

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

	// Uniform render scale applied to the ISM instance transform. Mesh origin is at
	// the feet, so scaling shrinks the character downward onto the ground. Used to make
	// child characters read as shorter than adults (kids ~0.85). 1.0 = no change.
	UPROPERTY(EditAnywhere, Category = "CrowdAnim", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float RenderScale = 1.f;
};
