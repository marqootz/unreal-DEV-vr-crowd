#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AvoidanceTargetComponent.generated.h"

UCLASS(ClassGroup = (Crowd), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UAvoidanceTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAvoidanceTargetComponent();

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	float Radius = 300.f;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	float Strength = 800.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
