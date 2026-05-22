#include "RandomRadiusSpawnDataGenerator.h"
#include "MassSpawnLocationProcessor.h"
#include "GameFramework/Actor.h"

void URandomRadiusSpawnDataGenerator::Generate(UObject& QueryOwner,
                                               TConstArrayView<FMassSpawnedEntityType> EntityTypes,
                                               int32 Count,
                                               FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const
{
	if (Count <= 0)
	{
		FinishedGeneratingSpawnPointsDelegate.Execute(TArray<FMassEntitySpawnDataGeneratorResult>());
		return;
	}

	TArray<FMassEntitySpawnDataGeneratorResult> Results;
	BuildResultsFromEntityTypes(Count, EntityTypes, Results);

	FVector Origin = FVector::ZeroVector;
	if (const AActor* OwnerActor = Cast<AActor>(&QueryOwner))
	{
		Origin = OwnerActor->GetActorLocation();
	}

	FRandomStream RandomStream(GetRandomSelectionSeed());

	for (FMassEntitySpawnDataGeneratorResult& Result : Results)
	{
		Result.SpawnDataProcessor = UMassSpawnLocationProcessor::StaticClass();
		Result.SpawnData.InitializeAs<FMassTransformsSpawnData>();
		FMassTransformsSpawnData& Transforms = Result.SpawnData.GetMutable<FMassTransformsSpawnData>();

		Transforms.Transforms.Reserve(Result.NumEntities);
		for (int32 i = 0; i < Result.NumEntities; ++i)
		{
			const float Angle = RandomStream.FRandRange(0.f, 2.f * PI);
			const float R = Radius * FMath::Sqrt(RandomStream.FRand());
			FVector Offset(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, 0.f);

			FTransform T;
			T.SetLocation(Origin + Offset);
			if (bProjectToGround)
			{
				FVector Loc = T.GetLocation();
				Loc.Z = GroundZ;
				T.SetLocation(Loc);
			}
			Transforms.Transforms.Add(T);
		}
	}

	FinishedGeneratingSpawnPointsDelegate.Execute(Results);
}
