#include "WandererTrait.h"
#include "MassCommonUtils.h"
#include "MassEntityManager.h"
#include "MassEntityTemplateRegistry.h"

void UWandererTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                   const UWorld& World) const
{
	BuildContext.AddFragment<FWandererFragment>();

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	const FConstSharedStruct ParamsFragment = EntityManager.GetOrCreateConstSharedFragment(Params);
	BuildContext.AddConstSharedFragment(ParamsFragment);

	const FConstSharedStruct SepFragment = EntityManager.GetOrCreateConstSharedFragment(SeparationParams);
	BuildContext.AddConstSharedFragment(SepFragment);
}
