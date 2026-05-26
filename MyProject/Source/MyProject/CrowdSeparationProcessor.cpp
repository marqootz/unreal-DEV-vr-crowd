#include "CrowdSeparationProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"

UCrowdSeparationProcessor::UCrowdSeparationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(TEXT("WandererProcessor"));
	bAutoRegisterWithProcessingPhases = true;
}

void UCrowdSeparationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FCrowdSeparationParams>();
}

void UCrowdSeparationProcessor::Execute(FMassEntityManager& EntityManager,
                                        FMassExecutionContext& Context)
{
	// Snapshot all positions in pass 1, apply separation in pass 2.
	TArray<FVector> Positions;
	Positions.Reserve(512);

	EntityQuery.ForEachEntityChunk(Context, [&Positions](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms =
			ChunkContext.GetFragmentView<FTransformFragment>();
		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			Positions.Add(Transforms[i].GetTransform().GetLocation());
		}
	});

	if (Positions.Num() < 2)
	{
		return;
	}

	int32 GlobalIndex = 0;
	EntityQuery.ForEachEntityChunk(Context, [&Positions, &GlobalIndex](FMassExecutionContext& ChunkContext)
	{
		const float DeltaTime = ChunkContext.GetDeltaTimeSeconds();
		const FCrowdSeparationParams& Params = ChunkContext.GetConstSharedFragment<FCrowdSeparationParams>();

		const TConstArrayView<FTransformFragment> Transforms =
			ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> Velocities =
			ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		const float Radius = Params.Radius;
		const float RadiusSq = Radius * Radius;
		const float MaxImpulse = Params.MaxImpulseCmPerSec * DeltaTime;

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i, ++GlobalIndex)
		{
			const FVector& SelfPos = Positions[GlobalIndex];
			FVector Accel = FVector::ZeroVector;

			for (int32 j = 0; j < Positions.Num(); ++j)
			{
				if (j == GlobalIndex) continue;
				FVector Delta = SelfPos - Positions[j];
				Delta.Z = 0.f;
				const float DistSq = Delta.SizeSquared();
				if (DistSq >= RadiusSq || DistSq < KINDA_SMALL_NUMBER) continue;

				const float Dist = FMath::Sqrt(DistSq);
				const float Falloff = 1.f - (Dist / Radius);
				Accel += (Delta / Dist) * (Params.Strength * Falloff * Falloff);
			}

			if (Accel.IsNearlyZero())
			{
				continue;
			}

			FVector Impulse = Accel * DeltaTime;
			const float Mag = Impulse.Size();
			if (Mag > MaxImpulse && MaxImpulse > 0.f)
			{
				Impulse *= MaxImpulse / Mag;
			}
			Velocities[i].Value += Impulse;
		}
	});
}
