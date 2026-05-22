#include "AvoidanceTargetComponent.h"
#include "AvoidanceTargetSubsystem.h"

UAvoidanceTargetComponent::UAvoidanceTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAvoidanceTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UAvoidanceTargetSubsystem* Sub = World->GetSubsystem<UAvoidanceTargetSubsystem>())
		{
			Sub->Register(this);
		}
	}
}

void UAvoidanceTargetComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UAvoidanceTargetSubsystem* Sub = World->GetSubsystem<UAvoidanceTargetSubsystem>())
		{
			Sub->Unregister(this);
		}
	}
	Super::EndPlay(Reason);
}
