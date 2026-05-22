#include "WandererProcessor.h"
#include "WandererFragment.h"
#include "MassExecutionContext.h"
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

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FWandererFragment& W = Wanderers[i];
			W.TimeUntilReroll -= DeltaTime;
			if (W.TimeUntilReroll <= 0.f)
			{
				W.SpeedJitter = FMath::FRandRange(-Params.MaxJitterCmPerSec,
				                                  Params.MaxJitterCmPerSec);
				W.TimeUntilReroll = Params.RerollIntervalSec;
			}

			FVector& V = Velocities[i].Value;
			if (V.IsNearlyZero())
			{
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				V = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Params.BaseSpeedCmPerSec;
			}
			V += V.GetSafeNormal() * W.SpeedJitter * DeltaTime;
		}
	});
}
