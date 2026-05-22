#include "PlayerAvoidanceProcessor.h"
#include "AvoidanceTargetSubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "Engine/World.h"

UPlayerAvoidanceProcessor::UPlayerAvoidanceProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(TEXT("WandererProcessor"));
	bAutoRegisterWithProcessingPhases = true;
}

void UPlayerAvoidanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
}

void UPlayerAvoidanceProcessor::Execute(FMassEntityManager& EntityManager,
                                        FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World) return;

	UAvoidanceTargetSubsystem* Sub = World->GetSubsystem<UAvoidanceTargetSubsystem>();
	if (!Sub) return;

	TArray<FAvoidanceTarget> Targets;
	Sub->GetSnapshot(Targets);
	if (Targets.Num() == 0) return;

	EntityQuery.ForEachEntityChunk(Context, [&Targets](FMassExecutionContext& ChunkContext)
	{
		const float DeltaTime = ChunkContext.GetDeltaTimeSeconds();

		const TConstArrayView<FTransformFragment> Transforms =
			ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> Velocities =
			ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FVector EntityLoc = Transforms[i].GetTransform().GetLocation();
			FVector Accel = FVector::ZeroVector;

			for (const FAvoidanceTarget& T : Targets)
			{
				if (T.Radius <= 0.f) continue;
				FVector Delta = EntityLoc - T.Location;
				Delta.Z = 0.f;
				const float Dist = Delta.Size();
				if (Dist >= T.Radius || Dist < KINDA_SMALL_NUMBER) continue;

				const float Falloff = 1.f - (Dist / T.Radius);
				Accel += (Delta / Dist) * (T.Strength * Falloff);
			}

			if (!Accel.IsNearlyZero())
			{
				Velocities[i].Value += Accel * DeltaTime;
			}
		}
	});
}
