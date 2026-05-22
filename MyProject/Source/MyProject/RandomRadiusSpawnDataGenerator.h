#pragma once

#include "CoreMinimal.h"
#include "MassEntitySpawnDataGeneratorBase.h"
#include "RandomRadiusSpawnDataGenerator.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Random Radius SpawnPoints Generator"))
class MYPROJECT_API URandomRadiusSpawnDataGenerator : public UMassEntitySpawnDataGeneratorBase
{
	GENERATED_BODY()

public:
	virtual void Generate(UObject& QueryOwner,
	                      TConstArrayView<FMassSpawnedEntityType> EntityTypes,
	                      int32 Count,
	                      FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Query", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float Radius = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Query")
	bool bProjectToGround = false;

	UPROPERTY(EditAnywhere, Category = "Query")
	float GroundZ = 0.f;
};
