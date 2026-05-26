// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdBakerEditorModule.h"

#include "AssetToolsModule.h"
#include "MessageLogModule.h"

#define LOCTEXT_NAMESPACE "CrowdBakerEditor"

DEFINE_LOG_CATEGORY(LogCrowdBakerEditor);

void FCrowdBakerEditorModule::StartupModule()
{
	// Register Log
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowFilters = true;
	InitOptions.bShowPages = false;
	InitOptions.bAllowClear = true;
	MessageLogModule.RegisterLogListing("CrowdBakerLog", LOCTEXT("CrowdBakerLog", "CrowdBaker Log"), InitOptions);
}

void FCrowdBakerEditorModule::ShutdownModule()
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.UnregisterLogListing("CrowdBakerLog");
}

IMPLEMENT_MODULE(FCrowdBakerEditorModule, CrowdBakerEditor)

#undef LOCTEXT_NAMESPACE
