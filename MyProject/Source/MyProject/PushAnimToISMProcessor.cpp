#include "PushAnimToISMProcessor.h"
#include "CrowdAnimFragment.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassLODFragments.h"
#include "MassVisualizationComponent.h"
#include "MassRepresentationTypes.h"
#include "CrowdBakerInstancePlaybackHelpers.h"
#include "CrowdBakerDataAsset.h"

UPushAnimToISMProcessor::UPushAnimToISMProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);
	bRequiresGameThreadExecution = true;
	bAutoRegisterWithProcessingPhases = true;
}

void UPushAnimToISMProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FCrowdAnimFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FCrowdAnimParams>();
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	EntityQuery.AddSharedRequirement<FMassRepresentationSubsystemSharedFragment>(EMassFragmentAccess::ReadWrite);
}

void UPushAnimToISMProcessor::Execute(FMassEntityManager& EntityManager,
                                      FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
	{
		UMassRepresentationSubsystem* RepresentationSubsystem =
			ChunkContext.GetSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		if (!RepresentationSubsystem)
		{
			return;
		}
		FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

		const FCrowdAnimParams& Params = ChunkContext.GetConstSharedFragment<FCrowdAnimParams>();
		if (Params.DataAsset == nullptr)
		{
			return;
		}

		const TConstArrayView<FCrowdAnimFragment> Anims =
			ChunkContext.GetFragmentView<FCrowdAnimFragment>();
		const TConstArrayView<FMassRepresentationFragment> Reps =
			ChunkContext.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> LODs =
			ChunkContext.GetFragmentView<FMassRepresentationLODFragment>();

		const int32 NumEntities = ChunkContext.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FMassRepresentationFragment& Rep = Reps[i];
			if (Rep.CurrentRepresentation != EMassRepresentationType::StaticMeshInstance)
			{
				continue;
			}
			const int32 InfoIdx = Rep.StaticMeshDescHandle.ToIndex();
			if (!ISMInfo.IsValidIndex(InfoIdx))
			{
				continue;
			}

			const int32 AnimIndex = (Anims[i].WalkBlend > Params.WalkBlendThreshold)
				? Params.WalkAnimIndex
				: Params.IdleAnimIndex;

			FCrowdBakerFrameData FrameData;
			UCrowdBakerInstancePlaybackLibrary::GetFrameDataFromDataAsset(
				Params.DataAsset, AnimIndex, Anims[i].CurrentTime,
				FrameData, /*TimeOffset=*/0.f, /*PlayRate=*/1.f);
			// Disable motion blur on the VAT — the engine helper sets PrevFrame=Frame-1
			// (one anim frame back), which UE interprets as a per-render-frame delta and
			// smears the verts. For a background crowd we don't need anim motion blur.
			FrameData.PrevFrame = FrameData.Frame;

			ISMInfo[InfoIdx].AddBatchedCustomData<FCrowdBakerFrameData>(
				FrameData, LODs[i].LODSignificance, Rep.PrevLODSignificance);
		}
	});
}
