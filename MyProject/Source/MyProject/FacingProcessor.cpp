#include "FacingProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"

UFacingProcessor::UFacingProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	bAutoRegisterWithProcessingPhases = true;
}

void UFacingProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
}

void UFacingProcessor::Execute(FMassEntityManager& EntityManager,
                               FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		const float DeltaTime = ChunkContext.GetDeltaTimeSeconds();
		const TArrayView<FTransformFragment> Transforms =
			ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassVelocityFragment> Velocities =
			ChunkContext.GetFragmentView<FMassVelocityFragment>();

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FVector V = Velocities[i].Value;
			V.Z = 0.f;
			if (V.IsNearlyZero())
			{
				continue;
			}

			FTransform& T = Transforms[i].GetMutableTransform();
			const FQuat Current = T.GetRotation();
			const FQuat VelRot  = V.GetSafeNormal().ToOrientationQuat();
			// UE5 mannequin SM faces -Y by default; rotate -90° around Z so its
			// forward aligns with the velocity vector.
			const FQuat MeshOffset(FRotator(0.f, -90.f, 0.f));
			const FQuat Target = VelRot * MeshOffset;
			const float TurnRate = 8.f;
			const float Alpha = FMath::Clamp(TurnRate * DeltaTime, 0.f, 1.f);
			T.SetRotation(FQuat::Slerp(Current, Target, Alpha));
		}
	});
}
