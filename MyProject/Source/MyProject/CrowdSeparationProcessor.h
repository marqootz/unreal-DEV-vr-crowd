#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "CrowdSeparationProcessor.generated.h"

USTRUCT()
struct FCrowdSeparationParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	// Distance under which entities push apart (cm).
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float Radius = 120.f;

	// Repulsion strength (cm/sec² at contact).
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float Strength = 2000.f;

	// Cap on per-frame velocity-magnitude change from separation (cm/sec).
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float MaxImpulseCmPerSec = 800.f;
};

UCLASS()
class MYPROJECT_API UCrowdSeparationProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UCrowdSeparationProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
private:
	FMassEntityQuery EntityQuery;
};
