#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "WandererFragment.h"
#include "CrowdSeparationProcessor.h"
#include "WandererTrait.generated.h"

UCLASS(meta = (DisplayName = "Wanderer"))
class MYPROJECT_API UWandererTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	                           const UWorld& World) const override;

	UPROPERTY(EditAnywhere, Category = "Wander")
	FWandererParams Params;

	UPROPERTY(EditAnywhere, Category = "Wander")
	FCrowdSeparationParams SeparationParams;
};
