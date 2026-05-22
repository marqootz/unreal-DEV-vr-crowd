#include "CrowdAnimProcessor.h"
#include "CrowdAnimFragment.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassCommonTypes.h"

UCrowdAnimProcessor::UCrowdAnimProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	bAutoRegisterWithProcessingPhases = true;
}

void UCrowdAnimProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FCrowdAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FCrowdAnimParams>();
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
}

void UCrowdAnimProcessor::Execute(FMassEntityManager& EntityManager,
                                  FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		const float DeltaTime = ChunkContext.GetDeltaTimeSeconds();
		const FCrowdAnimParams& Params = ChunkContext.GetConstSharedFragment<FCrowdAnimParams>();

		const TArrayView<FCrowdAnimFragment> Anims =
			ChunkContext.GetMutableFragmentView<FCrowdAnimFragment>();
		const TConstArrayView<FMassVelocityFragment> Velocities =
			ChunkContext.GetFragmentView<FMassVelocityFragment>();

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FCrowdAnimFragment& A = Anims[i];

			FVector V = Velocities[i].Value;
			V.Z = 0.f;
			const float Speed = V.Size();

			// Ease walk blend (1 = full walk, 0 = idle).
			const float TargetBlend = FMath::Clamp(Speed / FMath::Max(1.f, Params.WalkSpeedThresholdCmPerSec), 0.f, 1.f);
			const float Alpha = FMath::Clamp(Params.BlendRate * DeltaTime, 0.f, 1.f);
			A.WalkBlend = FMath::Lerp(A.WalkBlend, TargetBlend, Alpha);

			// Advance anim time scaled by velocity so the walk anim's foot speed matches
			// translation. Idle uses a steady tick to keep some life in stationary entities.
			const float TimeScale = FMath::Lerp(1.f, Speed / FMath::Max(1.f, Params.AnimNominalSpeedCmPerSec), A.WalkBlend);
			A.CurrentTime += DeltaTime * TimeScale;
		}
	});
}
