#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "PushAnimToISMProcessor.generated.h"

UCLASS()
class MYPROJECT_API UPushAnimToISMProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UPushAnimToISMProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager,
	                     FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
