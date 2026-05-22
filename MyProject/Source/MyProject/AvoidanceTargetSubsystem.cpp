#include "AvoidanceTargetSubsystem.h"
#include "AvoidanceTargetComponent.h"
#include "GameFramework/Actor.h"

void UAvoidanceTargetSubsystem::Register(UAvoidanceTargetComponent* Component)
{
	if (Component)
	{
		Components.AddUnique(Component);
	}
}

void UAvoidanceTargetSubsystem::Unregister(UAvoidanceTargetComponent* Component)
{
	Components.Remove(Component);
}

void UAvoidanceTargetSubsystem::GetSnapshot(TArray<FAvoidanceTarget>& OutTargets) const
{
	OutTargets.Reset(Components.Num());
	for (const TObjectPtr<UAvoidanceTargetComponent>& Comp : Components)
	{
		if (!Comp) continue;
		const AActor* Owner = Comp->GetOwner();
		if (!Owner) continue;

		FAvoidanceTarget T;
		T.Location = Owner->GetActorLocation();
		T.Radius = Comp->Radius;
		T.Strength = Comp->Strength;
		OutTargets.Add(T);
	}
}
