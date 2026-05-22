#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "WandererProcessor.generated.h"

UCLASS()
class MYPROJECT_API UWandererProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UWandererProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
