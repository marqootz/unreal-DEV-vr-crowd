#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "CrowdAnimFragment.h"
#include "CrowdAnimTrait.generated.h"

UCLASS(meta = (DisplayName = "Crowd Anim State"))
class MYPROJECT_API UCrowdAnimTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	                           const UWorld& World) const override;

	UPROPERTY(EditAnywhere, Category = "CrowdAnim")
	FCrowdAnimParams Params;
};
