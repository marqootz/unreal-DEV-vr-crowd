#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AvoidanceTargetSubsystem.generated.h"

class UAvoidanceTargetComponent;

USTRUCT()
struct FAvoidanceTarget
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	float Radius = 300.f;
	float Strength = 800.f;
};

UCLASS()
class MYPROJECT_API UAvoidanceTargetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(UAvoidanceTargetComponent* Component);
	void Unregister(UAvoidanceTargetComponent* Component);

	void GetSnapshot(TArray<FAvoidanceTarget>& OutTargets) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UAvoidanceTargetComponent>> Components;
};
