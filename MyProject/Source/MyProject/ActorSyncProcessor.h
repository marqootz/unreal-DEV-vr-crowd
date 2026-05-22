#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "ActorSyncProcessor.generated.h"

UCLASS()
class MYPROJECT_API UActorSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UActorSyncProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
