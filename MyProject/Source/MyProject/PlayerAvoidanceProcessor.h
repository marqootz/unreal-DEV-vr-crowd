#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "PlayerAvoidanceProcessor.generated.h"

UCLASS()
class MYPROJECT_API UPlayerAvoidanceProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UPlayerAvoidanceProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
