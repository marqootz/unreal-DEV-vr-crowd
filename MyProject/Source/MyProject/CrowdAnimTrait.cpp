#include "CrowdAnimTrait.h"
#include "MassCommonUtils.h"
#include "MassEntityManager.h"
#include "MassEntityTemplateRegistry.h"

void UCrowdAnimTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                    const UWorld& World) const
{
	FCrowdAnimFragment& Frag = BuildContext.AddFragment_GetRef<FCrowdAnimFragment>();
	Frag.TimeOffset = FMath::FRandRange(0.f, 2.f);
	Frag.CurrentTime = Frag.TimeOffset;
	Frag.WalkBlend = 0.f;

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	const FConstSharedStruct ParamsFragment = EntityManager.GetOrCreateConstSharedFragment(Params);
	BuildContext.AddConstSharedFragment(ParamsFragment);
}
