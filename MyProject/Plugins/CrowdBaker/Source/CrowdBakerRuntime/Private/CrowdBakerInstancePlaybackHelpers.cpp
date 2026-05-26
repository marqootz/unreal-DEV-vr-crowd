// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdBakerInstancePlaybackHelpers.h"
#include "CrowdBakerDataAsset.h"

bool UCrowdBakerInstancePlaybackLibrary::SetupInstancedMeshComponent(UInstancedStaticMeshComponent* InstancedMeshComponent, int32 NumInstances, bool bAutoPlay)
{	
	if (!InstancedMeshComponent)
	{
		return false;
	}

	// Clear data.
	InstancedMeshComponent->ClearInstances();

	if (!NumInstances)
	{
		return false;
	}

	// Allocate Transforms
	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.AddDefaulted(NumInstances);

	// Set Custom Data Length
	const SIZE_T DataSize = bAutoPlay ? sizeof(FCrowdBakerAutoPlayData) : sizeof(FCrowdBakerFrameData);
	InstancedMeshComponent->NumCustomDataFloats = DataSize / sizeof(float);

	// Initizalize Instances
	InstancedMeshComponent->AddInstances(InstanceTransforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);

	return true;
}

bool UCrowdBakerInstancePlaybackLibrary::BatchUpdateInstancesAutoPlayData(UInstancedStaticMeshComponent* InstancedMeshComponent, 
	const TArray<FCrowdBakerAutoPlayData>& AutoPlayData, const TArray<FMatrix>& Transforms, bool bMarkRenderStateDirty)
{
	return BatchUpdateInstancesData< FCrowdBakerAutoPlayData>(InstancedMeshComponent, AutoPlayData, Transforms, bMarkRenderStateDirty);
}

bool UCrowdBakerInstancePlaybackLibrary::BatchUpdateInstancesFrameData(UInstancedStaticMeshComponent* InstancedMeshComponent,
	const TArray<FCrowdBakerFrameData>& FrameData, const TArray<FMatrix>& Transforms, bool bMarkRenderStateDirty)
{
	return BatchUpdateInstancesData<FCrowdBakerFrameData>(InstancedMeshComponent, FrameData, Transforms, bMarkRenderStateDirty);
}

bool UCrowdBakerInstancePlaybackLibrary::UpdateInstanceAutoPlayData(UInstancedStaticMeshComponent* InstancedMeshComponent,
	int32 InstanceIndex, const FCrowdBakerAutoPlayData& AutoPlayData, bool bMarkRenderStateDirty)
{
	return UpdateInstanceCustomData<FCrowdBakerAutoPlayData>(InstancedMeshComponent, InstanceIndex, AutoPlayData, bMarkRenderStateDirty);
}

bool UCrowdBakerInstancePlaybackLibrary::UpdateInstanceFrameData(UInstancedStaticMeshComponent* InstancedMeshComponent,
	int32 InstanceIndex, const FCrowdBakerFrameData& FrameData, bool bMarkRenderStateDirty)
{
	return UpdateInstanceCustomData<FCrowdBakerFrameData>(InstancedMeshComponent, InstanceIndex, FrameData, bMarkRenderStateDirty);
}

bool UCrowdBakerInstancePlaybackLibrary::GetAutoPlayDataFromDataAsset(const UCrowdBakerDataAsset* DataAsset, int32 AnimationIndex,
	FCrowdBakerAutoPlayData& AutoPlayData, float TimeOffset, float PlayRate)
{
	if (DataAsset && DataAsset->Animations.IsValidIndex(AnimationIndex))
	{
		// Copy Frame Range
		const FCrowdBakerAnimInfo& AnimInfo = DataAsset->Animations[AnimationIndex];
		AutoPlayData.StartFrame = AnimInfo.StartFrame;
		AutoPlayData.EndFrame = AnimInfo.EndFrame;

		AutoPlayData.TimeOffset = TimeOffset;
		AutoPlayData.PlayRate = PlayRate;

		return true;
	}

	// Return default
	AutoPlayData = FCrowdBakerAutoPlayData();
	return false;
}

float UCrowdBakerInstancePlaybackLibrary::GetFrame(float Time, float StartFrame, float EndFrame, 
	float TimeOffset, float PlayRate, float SampleRate)
{	
	// Clamp inputs (just in case)
	Time = FMath::Max(Time, 0.f);
	TimeOffset = FMath::Max(TimeOffset, 0.f);
	PlayRate = FMath::Max(PlayRate, 0.f);

	const float Frame = (Time + TimeOffset) * (PlayRate * SampleRate);
	const float NumFrames = EndFrame - StartFrame + 1.f;
	return FMath::Fmod(Frame, NumFrames) + StartFrame;
}

bool UCrowdBakerInstancePlaybackLibrary::GetFrameDataFromDataAsset(const UCrowdBakerDataAsset* DataAsset, int32 AnimationIndex, float Time, FCrowdBakerFrameData& FrameData, float TimeOffset, float PlayRate)
{
	if (DataAsset && DataAsset->Animations.IsValidIndex(AnimationIndex))
	{
		// Copy Frame Range
		const FCrowdBakerAnimInfo& AnimInfo = DataAsset->Animations[AnimationIndex];
		
		FrameData.Frame = GetFrame(Time, AnimInfo.StartFrame, AnimInfo.EndFrame, 
			TimeOffset, PlayRate, DataAsset->SampleRate);

		// Previous Frame
		FrameData.PrevFrame = FMath::Clamp(FrameData.Frame - 1, AnimInfo.StartFrame, AnimInfo.EndFrame);

		return true;
	}

	// Return default
	FrameData = FCrowdBakerFrameData();
	return false;
}
