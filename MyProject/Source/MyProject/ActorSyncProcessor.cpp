#include "ActorSyncProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassActorSubsystem.h"
#include "GameFramework/Actor.h"

UActorSyncProcessor::UActorSyncProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = true;
}

void UActorSyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
}

void UActorSyncProcessor::Execute(FMassEntityManager& EntityManager,
                                  FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms =
			ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassActorFragment> ActorFrags =
			ChunkContext.GetMutableFragmentView<FMassActorFragment>();

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			AActor* Actor = ActorFrags[i].GetMutable();
			if (Actor == nullptr)
			{
				continue;
			}
			Actor->SetActorTransform(Transforms[i].GetTransform(), /*bSweep=*/false, nullptr,
			                         ETeleportType::TeleportPhysics);
		}
	});
}
