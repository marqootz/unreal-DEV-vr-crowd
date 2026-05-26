// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetDefinition_CrowdBaker.h"

#include "CrowdBakerBPLibrary.h"
#include "CrowdBakerDataAsset.h"
#include "AssetViewUtils.h"
#include "ContentBrowserMenuContexts.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ObjectEditorUtils.h"
#include "Styling/SlateIconFinder.h"
#include "ToolMenus.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "UAssetDefinition_CrowdBaker"

FText UAssetDefinition_CrowdBaker::GetAssetDisplayName() const
{
	return LOCTEXT("CrowdBakerAssetActions", "CrowdBaker");
}

FLinearColor UAssetDefinition_CrowdBaker::GetAssetColor() const
{
	return FColor::Blue;
}

TSoftClassPtr<UObject> UAssetDefinition_CrowdBaker::GetAssetClass() const
{
	return UCrowdBakerDataAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_CrowdBaker::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Animation };
	return Categories;
}

// Menu Extensions
//--------------------------------------------------------------------

namespace MenuExtension_CrowdBaker
{
	void RunCrowdBaker(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		TArray<UCrowdBakerDataAsset*> SelectedCrowdBakerObjects = Context->LoadSelectedObjects<UCrowdBakerDataAsset>();

		for (auto ObjIt = SelectedCrowdBakerObjects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			UCrowdBakerDataAsset* DataAsset = *ObjIt;
			// Create UVs and Textures
			if (UCrowdBakerBPLibrary::AnimationToTexture(*ObjIt))
			{
				// Update Material Instances (if Possible)
				if (UStaticMesh* StaticMesh = DataAsset->GetStaticMesh())
				{
					for (FStaticMaterial& StaticMaterial : StaticMesh->GetStaticMaterials())
					{
						if (UMaterialInstanceConstant* MaterialInstanceConstant = Cast<UMaterialInstanceConstant>(StaticMaterial.MaterialInterface))
						{
							UCrowdBakerBPLibrary::UpdateMaterialInstanceFromDataAsset(DataAsset, MaterialInstanceConstant, EMaterialParameterAssociation::LayerParameter);
						}
					}
				}
			}
		}
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []{ 
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UCrowdBakerDataAsset::StaticClass());

			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
			Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InSection))
				{
					const TAttribute<FText> Label = LOCTEXT("CrowdBaker_Run", "Run Animation To Texture");
					const TAttribute<FText> ToolTip = LOCTEXT("CrowdBaker_RunTooltip", "Creates Vertex Animation Textures (VAT)");
					const FSlateIcon Icon = FSlateIcon();

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&RunCrowdBaker);

					InSection.AddMenuEntry(TEXT("CrowdBaker_RunAnimationToTexture"), Label, ToolTip, Icon, UIAction);
				}
			}));
		}));
	});
};

//--------------------------------------------------------------------
// Menu Extensions

#undef LOCTEXT_NAMESPACE
