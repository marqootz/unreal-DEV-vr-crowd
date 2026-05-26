#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrowdBakerMaterialIntrospection.generated.h"

class UMaterialFunctionInterface;
class UMaterialExpression;
class UMaterialExpressionMaterialFunctionCall;

UCLASS()
class CROWDBAKEREDITOR_API UCrowdBakerMaterialIntrospection : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static TArray<UMaterialExpression*> GetFunctionExpressions(UMaterialFunctionInterface* Function);

	// Same accessor for UMaterial.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static TArray<UMaterialExpression*> GetMaterialExpressions(class UMaterial* Material);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetExpressionClassName(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetExpressionDesc(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetScalarParameterName(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetStaticSwitchParameterName(UMaterialExpression* Expression);

	// Generic parameter-name accessor — works for any UMaterialExpressionParameter subclass.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetParameterName(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool GetStaticBoolDefaultValue(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString GetNamedRerouteName(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static int32 GetCustomDataIndex(UMaterialExpression* Expression);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static UMaterialFunctionInterface* GetMaterialFunctionFromCall(UMaterialExpression* Expression);

	// Returns input pin names in expression-input order.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static TArray<FString> GetInputNames(UMaterialExpression* Expression);

	// Returns source expressions per input (parallel to GetInputNames). May contain nulls for unconnected pins.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static TArray<UMaterialExpression*> GetInputSources(UMaterialExpression* Expression);

	// Connect Source.OutputIndex -> Dest.InputIndex (by input index from GetInputNames order).
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool ConnectByInputIndex(UMaterialExpression* Source, int32 SourceOutputIndex, UMaterialExpression* Dest, int32 DestInputIndex);

	// Force-override a static switch on a MaterialInstanceConstant. Works for global, layer, and blend params.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool ForceSetStaticSwitch(class UMaterialInstanceConstant* MIC, FName ParameterName, bool Value);

	// Diagnostic: returns 1 if the switch resolves to True, 0 if False, -1 if parameter not found at any association.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static int32 GetEffectiveStaticSwitchTriState(class UMaterialInstance* MI, FName ParameterName);

	// Returns the effective scalar value for a parameter at any association (global/layer/blend).
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static float GetEffectiveScalar(class UMaterialInstance* MI, FName ParameterName);

	// Returns the effective texture value for a parameter at any association.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static class UTexture* GetEffectiveTexture(class UMaterialInstance* MI, FName ParameterName);

	// Force-override scalar with association sweep.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool ForceSetScalar(class UMaterialInstanceConstant* MIC, FName ParameterName, float Value);

	// Force-override texture with association sweep.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool ForceSetTexture(class UMaterialInstanceConstant* MIC, FName ParameterName, class UTexture* Value);

	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FLinearColor GetEffectiveVector(class UMaterialInstance* MI, FName ParameterName);

	// Re-parent a MaterialInstanceConstant.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static bool SetMIParent(class UMaterialInstanceConstant* MIC, class UMaterialInterface* NewParent);

	// Compare a single texel position across two rows of a Texture2D source.
	// Returns a per-row summary: "min,max,mean" per channel for each row.
	// Use to detect ref-pose bakes (where row N == row 0 for all N).
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString DescribeTextureRowDelta(class UTexture2D* Tex, int32 RowA, int32 RowB);

	// Sample a single pixel from the texture source. Returns "R,G,B,A" decimal.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString SampleTexturePixel(class UTexture2D* Tex, int32 X, int32 Y);

	// Dump a few skel mesh render vertex positions to verify the position buffer holds varied data.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString DescribeSkelMeshVertices(class USkeletalMesh* Mesh, int32 LODIndex, int32 NumSamples);

	// Return all bone names from a skeletal mesh (RefSkeleton ordering = bake's mesh bone index).
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static TArray<FString> GetSkelMeshBoneNames(class USkeletalMesh* Mesh);

	// Dump static mesh vertex positions to compare coord-space with skel-mesh.
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString DescribeStaticMeshVertices(class UStaticMesh* Mesh, int32 LODIndex, int32 NumSamples);

	// Sample static mesh vertex's UV at a specific channel (to verify bone-weight UV1 wasn't overwritten).
	UFUNCTION(BlueprintCallable, Category = "CrowdBaker|MaterialDebug")
	static FString SampleStaticMeshUV(class UStaticMesh* Mesh, int32 LODIndex, int32 UVChannel, int32 VertexIndex);
};
