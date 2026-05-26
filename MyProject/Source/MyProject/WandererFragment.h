#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "WandererFragment.generated.h"

UENUM()
enum class EWandererState : uint8
{
	NotInitialized,
	Walking,
	Idle,
	PermanentIdle,
};

USTRUCT()
struct FWandererArchetype
{
	GENERATED_BODY()

	// Display label (debug only).
	UPROPERTY(EditAnywhere, Category = "Archetype")
	FName Name = TEXT("Casual");

	// Relative selection weight in the archetype mix.
	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	float BaseSpeedCmPerSec = 150.f;

	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.1"))
	float SpeedMultiplierMin = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.1"))
	float SpeedMultiplierMax = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkToIdleProbability = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleToWalkProbability = 0.5f;

	// Probability this entity is a permanent stander (never walks) IF this archetype is picked.
	UPROPERTY(EditAnywhere, Category = "Archetype", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PermanentIdleProbability = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	float MinWalkDurationSec = 6.f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	float MaxWalkDurationSec = 18.f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	float MinIdleDurationSec = 3.f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	float MaxIdleDurationSec = 10.f;
};

USTRUCT()
struct FWandererFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	float SpeedJitter = 0.f;

	UPROPERTY()
	float TimeUntilReroll = 0.f;

	// -1 or +1 along the preferred axis; 0 means "not assigned yet". Picked once on first tick.
	UPROPERTY()
	float LaneDirection = 0.f;

	UPROPERTY()
	EWandererState State = EWandererState::NotInitialized;

	// Per-entity offset perpendicular to PreferredAxis, in cm (signed).
	UPROPERTY()
	float LaneOffsetCm = 0.f;

	// Per-entity multiplier on BaseSpeedCmPerSec — assigned once on init.
	UPROPERTY()
	float SpeedMultiplier = 1.f;

	// Seconds remaining in the current state before considering a transition.
	UPROPERTY()
	float TimeUntilStateChange = 0.f;

	// Per-entity resolved archetype params — chosen once on init from FWandererParams::Archetypes
	// (or built from the legacy params fields). Processor reads from here, not from FWandererParams.
	UPROPERTY()
	FWandererArchetype Resolved;
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

	// Corridor axis the crowd walks along (world space). Default ±Y (platform long axis).
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor")
	FVector PreferredAxis = FVector(0.f, 1.f, 0.f);

	// 0 = free omnidirectional wander, 1 = locked to PreferredAxis. Per-tick blend toward axis.
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AxisAlignmentStrength = 0.7f;

	// World-space center of the corridor (entities reverse direction once they pass the half-length).
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor")
	FVector CorridorCenter = FVector::ZeroVector;

	// Half-length along PreferredAxis. Beyond this distance the entity's lane flips.
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor")
	float CorridorHalfLength = 800.f;

	// Half-width perpendicular to PreferredAxis. Each entity is assigned a random lane offset
	// in this range and gently steers toward it so the crowd spreads laterally.
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor")
	float CorridorHalfWidth = 250.f;

	// Strength of lateral lane-return pull (units: 1/sec). Low = entities are free to step laterally
	// for avoidance. High = entities snap back to their assigned lane fast (use sparingly).
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor", meta = (ClampMin = "0.0"))
	float LaneReturnGain = 0.4f;

	// Random walk applied to LaneOffsetCm each tick (cm/sec). Keeps entities from locking to a single lane.
	UPROPERTY(EditAnywhere, Category = "Wander|Corridor")
	float LaneDriftCmPerSec = 4.f;

	// Archetype mix — each entity randomly picks one at spawn (weighted by Archetype.Weight) and uses
	// that archetype's per-entity tunables (speed, idle/walk probs, etc.). If empty, the legacy
	// FWandererParams fields below are used. Configure this on ONE config (e.g. global wanderer) so the
	// SAME mix applies regardless of character variant.
	UPROPERTY(EditAnywhere, Category = "Wander|Archetypes")
	TArray<FWandererArchetype> Archetypes;

	// Max acceleration (cm/sec²) — clamps abrupt velocity changes so start/stop reads as smooth.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float MaxAccelCmPerSec2 = 250.f;

	// Yaw smoothing time toward velocity direction (sec). 0 disables, larger = lazier turn-in-place.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float YawSmoothingTimeSec = 0.4f;

	// Static yaw offset applied to the rotation so the mesh's local forward axis aligns with the
	// velocity direction. UE5 mannequin SM faces -Y → use -90. If mesh faces +X (engine convention)
	// use 0. If mesh faces +Y use 90.
	UPROPERTY(EditAnywhere, Category = "Wander")
	float MeshYawOffsetDeg = -90.f;

	// Fraction of the crowd that should be permanently standing still.
	UPROPERTY(EditAnywhere, Category = "Wander|States", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PermanentIdleFraction = 0.1f;

	// On state-change tick, probability that a walker pauses to idle.
	UPROPERTY(EditAnywhere, Category = "Wander|States", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkToIdleProbability = 0.15f;

	// On state-change tick, probability that an idler starts walking again.
	UPROPERTY(EditAnywhere, Category = "Wander|States", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleToWalkProbability = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Wander|States")
	float MinWalkDurationSec = 6.f;

	UPROPERTY(EditAnywhere, Category = "Wander|States")
	float MaxWalkDurationSec = 18.f;

	UPROPERTY(EditAnywhere, Category = "Wander|States")
	float MinIdleDurationSec = 3.f;

	UPROPERTY(EditAnywhere, Category = "Wander|States")
	float MaxIdleDurationSec = 10.f;
};
