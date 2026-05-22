#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "CrowdAnimProcessor.generated.h"

UCLASS()
class MYPROJECT_API UCrowdAnimProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UCrowdAnimProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
