#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "FacingProcessor.generated.h"

UCLASS()
class MYPROJECT_API UFacingProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFacingProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
