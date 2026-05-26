#include "WandererProcessor.h"
#include "WandererFragment.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"

UWandererProcessor::UWandererProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	bAutoRegisterWithProcessingPhases = true;
}

void UWandererProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FWandererFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FWandererParams>();
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
}

void UWandererProcessor::Execute(FMassEntityManager& EntityManager,
                                 FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		const float DeltaTime = ChunkContext.GetDeltaTimeSeconds();
		const FWandererParams& Params = ChunkContext.GetConstSharedFragment<FWandererParams>();

		const TArrayView<FWandererFragment> Wanderers =
			ChunkContext.GetMutableFragmentView<FWandererFragment>();
		const TArrayView<FMassVelocityFragment> Velocities =
			ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FTransformFragment> TransformFrags =
			ChunkContext.GetMutableFragmentView<FTransformFragment>();

		const FVector Axis = Params.PreferredAxis.GetSafeNormal2D();
		const float AxisBlend = FMath::Clamp(Params.AxisAlignmentStrength, 0.f, 1.f);
		const float MaxAccelStep = Params.MaxAccelCmPerSec2 * DeltaTime;

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FWandererFragment& W = Wanderers[i];

			// One-time initialization: lane direction, lateral offset, archetype, state.
			if (W.State == EWandererState::NotInitialized)
			{
				W.LaneDirection = FMath::RandBool() ? 1.f : -1.f;
				W.LaneOffsetCm = FMath::FRandRange(-Params.CorridorHalfWidth, Params.CorridorHalfWidth);

				// Resolve archetype: weighted pick from Params.Archetypes, or fall back to legacy fields.
				if (Params.Archetypes.Num() > 0)
				{
					float TotalWeight = 0.f;
					for (const FWandererArchetype& A : Params.Archetypes)
					{
						TotalWeight += FMath::Max(0.f, A.Weight);
					}
					if (TotalWeight > 0.f)
					{
						float Pick = FMath::FRandRange(0.f, TotalWeight);
						for (const FWandererArchetype& A : Params.Archetypes)
						{
							Pick -= FMath::Max(0.f, A.Weight);
							if (Pick <= 0.f)
							{
								W.Resolved = A;
								break;
							}
						}
					}
					else
					{
						W.Resolved = Params.Archetypes[0];
					}
				}
				else
				{
					// Build resolved from legacy fields.
					W.Resolved.BaseSpeedCmPerSec = Params.BaseSpeedCmPerSec;
					W.Resolved.SpeedMultiplierMin = 0.85f;
					W.Resolved.SpeedMultiplierMax = 1.15f;
					W.Resolved.WalkToIdleProbability = Params.WalkToIdleProbability;
					W.Resolved.IdleToWalkProbability = Params.IdleToWalkProbability;
					W.Resolved.PermanentIdleProbability = Params.PermanentIdleFraction;
					W.Resolved.MinWalkDurationSec = Params.MinWalkDurationSec;
					W.Resolved.MaxWalkDurationSec = Params.MaxWalkDurationSec;
					W.Resolved.MinIdleDurationSec = Params.MinIdleDurationSec;
					W.Resolved.MaxIdleDurationSec = Params.MaxIdleDurationSec;
				}

				W.SpeedMultiplier = FMath::FRandRange(W.Resolved.SpeedMultiplierMin, W.Resolved.SpeedMultiplierMax);

				if (FMath::FRand() < W.Resolved.PermanentIdleProbability)
				{
					W.State = EWandererState::PermanentIdle;
					W.TimeUntilStateChange = TNumericLimits<float>::Max();
				}
				else
				{
					W.State = EWandererState::Walking;
					W.TimeUntilStateChange = FMath::FRandRange(0.f, W.Resolved.MaxWalkDurationSec);
				}
				W.TimeUntilReroll = FMath::FRandRange(0.f, Params.RerollIntervalSec);
			}

			// State transitions — use per-entity resolved archetype values.
			W.TimeUntilStateChange -= DeltaTime;
			if (W.TimeUntilStateChange <= 0.f)
			{
				const FWandererArchetype& A = W.Resolved;
				if (W.State == EWandererState::Walking)
				{
					if (FMath::FRand() < A.WalkToIdleProbability)
					{
						W.State = EWandererState::Idle;
						W.TimeUntilStateChange = FMath::FRandRange(A.MinIdleDurationSec, A.MaxIdleDurationSec);
					}
					else
					{
						W.TimeUntilStateChange = FMath::FRandRange(A.MinWalkDurationSec, A.MaxWalkDurationSec);
					}
				}
				else if (W.State == EWandererState::Idle)
				{
					if (FMath::FRand() < A.IdleToWalkProbability)
					{
						W.State = EWandererState::Walking;
						W.TimeUntilStateChange = FMath::FRandRange(A.MinWalkDurationSec, A.MaxWalkDurationSec);
					}
					else
					{
						W.TimeUntilStateChange = FMath::FRandRange(A.MinIdleDurationSec, A.MaxIdleDurationSec);
					}
				}
			}

			// Speed jitter reroll.
			W.TimeUntilReroll -= DeltaTime;
			if (W.TimeUntilReroll <= 0.f)
			{
				W.SpeedJitter = FMath::FRandRange(-Params.MaxJitterCmPerSec, Params.MaxJitterCmPerSec);
				W.TimeUntilReroll = Params.RerollIntervalSec;
			}

			// Lane offset drift — keeps entities from locking to a fixed lateral lane.
			if (Params.LaneDriftCmPerSec > 0.f && Params.CorridorHalfWidth > 0.f)
			{
				W.LaneOffsetCm += FMath::FRandRange(-Params.LaneDriftCmPerSec, Params.LaneDriftCmPerSec) * DeltaTime;
				W.LaneOffsetCm = FMath::Clamp(W.LaneOffsetCm, -Params.CorridorHalfWidth, Params.CorridorHalfWidth);
			}

			FVector& V = Velocities[i].Value;
			FTransform& Transform = TransformFrags[i].GetMutableTransform();
			const FVector Loc = Transform.GetLocation();

			// Build the desired velocity ("VTarget") for this entity based on state + corridor logic.
			FVector VTarget = FVector::ZeroVector;
			if (W.State != EWandererState::Idle && W.State != EWandererState::PermanentIdle)
			{
				// Boundary reflection: flip lane direction once past the corridor half-length.
				if (!Axis.IsNearlyZero() && Params.CorridorHalfLength > 0.f)
				{
					const float SignedDist = FVector::DotProduct(Loc - Params.CorridorCenter, Axis);
					if (SignedDist * W.LaneDirection > Params.CorridorHalfLength)
					{
						W.LaneDirection = -W.LaneDirection;
					}
				}

				const float DesiredSpeed = W.Resolved.BaseSpeedCmPerSec * W.SpeedMultiplier + W.SpeedJitter;
				if (!Axis.IsNearlyZero())
				{
					VTarget = Axis * W.LaneDirection * DesiredSpeed;

					// Lateral lane-return correction.
					if (Params.LaneReturnGain > 0.f)
					{
						const FVector Perp(Axis.Y, -Axis.X, 0.f);
						const float CurrentPerp = FVector::DotProduct(Loc - Params.CorridorCenter, Perp);
						const float PerpError = W.LaneOffsetCm - CurrentPerp;
						// Apply as a perpendicular velocity contribution (proportional to error).
						VTarget += Perp * (PerpError * Params.LaneReturnGain);
					}
				}
				else
				{
					// No corridor — keep current direction at full speed.
					const FVector Dir = V.IsNearlyZero() ? FVector(0, 1, 0) : V.GetSafeNormal2D();
					VTarget = Dir * DesiredSpeed;
				}
			}

			// Acceleration clamp: limit per-tick change in V.
			FVector DeltaV = VTarget - V;
			DeltaV.Z = 0.f;
			const float DeltaMag = DeltaV.Size();
			if (DeltaMag > MaxAccelStep && MaxAccelStep > 0.f)
			{
				DeltaV *= MaxAccelStep / DeltaMag;
			}
			V += DeltaV;

			// Smooth yaw rotation toward velocity direction (only when moving).
			if (Params.YawSmoothingTimeSec > 0.f && !V.IsNearlyZero())
			{
				const FQuat CurrentRot = Transform.GetRotation();
				const float CurrentYaw = CurrentRot.Rotator().Yaw;
				const float VelocityYaw = FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X));
				const float TargetYaw = VelocityYaw + Params.MeshYawOffsetDeg;
				// Exponential smoothing on the wrapped delta yaw.
				const float Alpha = 1.f - FMath::Exp(-DeltaTime / Params.YawSmoothingTimeSec);
				const float NewYaw = FMath::UnwindDegrees(CurrentYaw + Alpha * FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));
				Transform.SetRotation(FQuat(FRotator(0.f, NewYaw, 0.f)));
			}
		}
	});
}
