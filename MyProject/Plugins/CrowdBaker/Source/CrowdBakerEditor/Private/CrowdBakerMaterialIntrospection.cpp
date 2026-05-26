#include "CrowdBakerMaterialIntrospection.h"

#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialFunction.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstance.h"
#include "MaterialEditingLibrary.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Misc/MemStack.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

TArray<UMaterialExpression*> UCrowdBakerMaterialIntrospection::GetFunctionExpressions(UMaterialFunctionInterface* Function)
{
	TArray<UMaterialExpression*> Out;
#if WITH_EDITORONLY_DATA
	if (!Function)
	{
		return Out;
	}
	UMaterialFunction* MF = Cast<UMaterialFunction>(Function);
	if (!MF)
	{
		return Out;
	}
	TConstArrayView<TObjectPtr<UMaterialExpression>> Exprs = MF->GetExpressions();
	Out.Reserve(Exprs.Num());
	for (const TObjectPtr<UMaterialExpression>& Expr : Exprs)
	{
		Out.Add(Expr.Get());
	}
#endif
	return Out;
}

TArray<UMaterialExpression*> UCrowdBakerMaterialIntrospection::GetMaterialExpressions(UMaterial* Material)
{
	TArray<UMaterialExpression*> Out;
#if WITH_EDITORONLY_DATA
	if (!Material)
	{
		return Out;
	}
	TConstArrayView<TObjectPtr<UMaterialExpression>> Exprs = Material->GetExpressions();
	Out.Reserve(Exprs.Num());
	for (const TObjectPtr<UMaterialExpression>& Expr : Exprs)
	{
		Out.Add(Expr.Get());
	}
#endif
	return Out;
}

FString UCrowdBakerMaterialIntrospection::GetExpressionClassName(UMaterialExpression* Expression)
{
	return Expression ? Expression->GetClass()->GetName() : FString();
}

FString UCrowdBakerMaterialIntrospection::GetExpressionDesc(UMaterialExpression* Expression)
{
#if WITH_EDITORONLY_DATA
	return Expression ? Expression->Desc : FString();
#else
	return FString();
#endif
}

FString UCrowdBakerMaterialIntrospection::GetScalarParameterName(UMaterialExpression* Expression)
{
	if (UMaterialExpressionScalarParameter* Scalar = Cast<UMaterialExpressionScalarParameter>(Expression))
	{
		return Scalar->ParameterName.ToString();
	}
	return FString();
}

FString UCrowdBakerMaterialIntrospection::GetStaticSwitchParameterName(UMaterialExpression* Expression)
{
	if (UMaterialExpressionStaticSwitchParameter* Sw = Cast<UMaterialExpressionStaticSwitchParameter>(Expression))
	{
		return Sw->ParameterName.ToString();
	}
	return FString();
}

FString UCrowdBakerMaterialIntrospection::GetParameterName(UMaterialExpression* Expression)
{
	if (UMaterialExpressionParameter* P = Cast<UMaterialExpressionParameter>(Expression))
	{
		return P->ParameterName.ToString();
	}
	return FString();
}

bool UCrowdBakerMaterialIntrospection::GetStaticBoolDefaultValue(UMaterialExpression* Expression)
{
	if (UMaterialExpressionStaticBoolParameter* B = Cast<UMaterialExpressionStaticBoolParameter>(Expression))
	{
		return B->DefaultValue;
	}
	return false;
}

FString UCrowdBakerMaterialIntrospection::GetNamedRerouteName(UMaterialExpression* Expression)
{
#if WITH_EDITORONLY_DATA
	if (UMaterialExpressionNamedRerouteDeclaration* Decl = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expression))
	{
		return Decl->Name.ToString();
	}
	if (UMaterialExpressionNamedRerouteUsage* Use = Cast<UMaterialExpressionNamedRerouteUsage>(Expression))
	{
		return Use->Declaration ? Use->Declaration->Name.ToString() : FString(TEXT("<unbound>"));
	}
#endif
	return FString();
}

int32 UCrowdBakerMaterialIntrospection::GetCustomDataIndex(UMaterialExpression* Expression)
{
	if (UMaterialExpressionPerInstanceCustomData* CD = Cast<UMaterialExpressionPerInstanceCustomData>(Expression))
	{
		return CD->DataIndex;
	}
	return -1;
}

TArray<FString> UCrowdBakerMaterialIntrospection::GetInputNames(UMaterialExpression* Expression)
{
	TArray<FString> Out;
#if WITH_EDITORONLY_DATA
	if (!Expression)
	{
		return Out;
	}
	for (FExpressionInputIterator It{ Expression }; It; ++It)
	{
		Out.Add(Expression->GetInputName(It.Index).ToString());
	}
#endif
	return Out;
}

TArray<UMaterialExpression*> UCrowdBakerMaterialIntrospection::GetInputSources(UMaterialExpression* Expression)
{
	TArray<UMaterialExpression*> Out;
#if WITH_EDITORONLY_DATA
	if (!Expression)
	{
		return Out;
	}
	for (FExpressionInputIterator It{ Expression }; It; ++It)
	{
		Out.Add(It.Input ? It.Input->Expression : nullptr);
	}
#endif
	return Out;
}

bool UCrowdBakerMaterialIntrospection::ConnectByInputIndex(UMaterialExpression* Source, int32 SourceOutputIndex, UMaterialExpression* Dest, int32 DestInputIndex)
{
#if WITH_EDITORONLY_DATA
	if (!Source || !Dest)
	{
		return false;
	}
	FExpressionInput* In = Dest->GetInput(DestInputIndex);
	if (!In)
	{
		return false;
	}
	In->Expression = Source;
	In->OutputIndex = SourceOutputIndex;
	// Set mask from source output
	if (Source->Outputs.IsValidIndex(SourceOutputIndex))
	{
		const FExpressionOutput& Out = Source->Outputs[SourceOutputIndex];
		In->Mask = Out.Mask;
		In->MaskR = Out.MaskR;
		In->MaskG = Out.MaskG;
		In->MaskB = Out.MaskB;
		In->MaskA = Out.MaskA;
	}
	Dest->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCrowdBakerMaterialIntrospection::ForceSetStaticSwitch(UMaterialInstanceConstant* MIC, FName ParameterName, bool Value)
{
#if WITH_EDITORONLY_DATA
	if (!MIC)
	{
		return false;
	}
	// Try Global first, then LayerParameter at index 0 — matches how MaterialEditingLibrary does it.
	MIC->SetStaticSwitchParameterValueEditorOnly(
		FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::GlobalParameter, INDEX_NONE), Value);
	MIC->SetStaticSwitchParameterValueEditorOnly(
		FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::LayerParameter, 0), Value);
	UMaterialEditingLibrary::UpdateMaterialInstance(MIC);
	return true;
#else
	return false;
#endif
}

float UCrowdBakerMaterialIntrospection::GetEffectiveScalar(UMaterialInstance* MI, FName ParameterName)
{
	if (!MI) return 0.f;
	float Val = 0.f;
	const EMaterialParameterAssociation Assocs[] = { EMaterialParameterAssociation::GlobalParameter, EMaterialParameterAssociation::LayerParameter, EMaterialParameterAssociation::BlendParameter };
	for (EMaterialParameterAssociation Assoc : Assocs)
	{
		const int32 Idx = (Assoc == EMaterialParameterAssociation::GlobalParameter) ? INDEX_NONE : 0;
		if (MI->GetScalarParameterValue(FMaterialParameterInfo(ParameterName, Assoc, Idx), Val))
		{
			return Val;
		}
	}
	return 0.f;
}

UTexture* UCrowdBakerMaterialIntrospection::GetEffectiveTexture(UMaterialInstance* MI, FName ParameterName)
{
	if (!MI) return nullptr;
	UTexture* Tex = nullptr;
	const EMaterialParameterAssociation Assocs[] = { EMaterialParameterAssociation::GlobalParameter, EMaterialParameterAssociation::LayerParameter, EMaterialParameterAssociation::BlendParameter };
	for (EMaterialParameterAssociation Assoc : Assocs)
	{
		const int32 Idx = (Assoc == EMaterialParameterAssociation::GlobalParameter) ? INDEX_NONE : 0;
		if (MI->GetTextureParameterValue(FMaterialParameterInfo(ParameterName, Assoc, Idx), Tex))
		{
			return Tex;
		}
	}
	return nullptr;
}

bool UCrowdBakerMaterialIntrospection::ForceSetScalar(UMaterialInstanceConstant* MIC, FName ParameterName, float Value)
{
#if WITH_EDITORONLY_DATA
	if (!MIC) return false;
	MIC->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::GlobalParameter, INDEX_NONE), Value);
	MIC->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::LayerParameter, 0), Value);
	UMaterialEditingLibrary::UpdateMaterialInstance(MIC);
	return true;
#else
	return false;
#endif
}

bool UCrowdBakerMaterialIntrospection::ForceSetTexture(UMaterialInstanceConstant* MIC, FName ParameterName, UTexture* Value)
{
#if WITH_EDITORONLY_DATA
	if (!MIC) return false;
	MIC->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::GlobalParameter, INDEX_NONE), Value);
	MIC->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::LayerParameter, 0), Value);
	UMaterialEditingLibrary::UpdateMaterialInstance(MIC);
	return true;
#else
	return false;
#endif
}

FString UCrowdBakerMaterialIntrospection::SampleStaticMeshUV(UStaticMesh* Mesh, int32 LODIndex, int32 UVChannel, int32 VertexIndex)
{
#if WITH_EDITORONLY_DATA
	if (!Mesh) return TEXT("null");
	if (!Mesh->IsMeshDescriptionValid(LODIndex)) return TEXT("no mesh desc");
	const FMeshDescription* MD = Mesh->GetMeshDescription(LODIndex);
	if (!MD) return TEXT("no mesh desc");
	FStaticMeshConstAttributes Attrs(*MD);
	TVertexInstanceAttributesConstRef<FVector2f> UVs = Attrs.GetVertexInstanceUVs();
	if (UVChannel >= UVs.GetNumChannels()) return FString::Printf(TEXT("only %d UV channels"), UVs.GetNumChannels());
	// Find a VertexInstance referencing this VertexID
	const FVertexID VID(VertexIndex);
	if (!MD->IsVertexValid(VID)) return TEXT("invalid VID");
	const TArrayView<const FVertexInstanceID> Instances = MD->GetVertexVertexInstanceIDs(VID);
	if (Instances.Num() == 0) return TEXT("no instances");
	const FVector2f UV = UVs.Get(Instances[0], UVChannel);
	return FString::Printf(TEXT("UV%d for vert %d (instance %d) = (%.5f, %.5f)"),
		UVChannel, VertexIndex, Instances[0].GetValue(), UV.X, UV.Y);
#else
	return TEXT("editor-only");
#endif
}

FString UCrowdBakerMaterialIntrospection::DescribeStaticMeshVertices(UStaticMesh* Mesh, int32 LODIndex, int32 NumSamples)
{
#if WITH_EDITORONLY_DATA
	if (!Mesh) return TEXT("null mesh");
	if (!Mesh->IsMeshDescriptionValid(LODIndex)) return TEXT("no mesh desc");
	const FMeshDescription* MD = Mesh->GetMeshDescription(LODIndex);
	if (!MD) return TEXT("no mesh desc");
	const auto VertexPositions = MD->GetVertexPositions();
	const int32 NumVerts = MD->Vertices().Num();
	FString Out = FString::Printf(TEXT("LOD%d numVerts=%d  samples:"), LODIndex, NumVerts);
	for (int32 i = 0; i < FMath::Min(NumSamples, 10); ++i)
	{
		const int32 Idx = (NumSamples > 0 && i < NumSamples) ? (i * (NumVerts / FMath::Max(NumSamples, 1))) : i;
		if (Idx >= NumVerts) break;
		const FVector3f P = VertexPositions[FVertexID(Idx)];
		Out += FString::Printf(TEXT(" v[%d]=(%.1f,%.1f,%.1f)"), Idx, P.X, P.Y, P.Z);
	}
	return Out;
#else
	return TEXT("editor-only");
#endif
}

TArray<FString> UCrowdBakerMaterialIntrospection::GetSkelMeshBoneNames(USkeletalMesh* Mesh)
{
	TArray<FString> Out;
	if (!Mesh) return Out;
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	const int32 Num = Ref.GetNum();
	Out.Reserve(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		Out.Add(Ref.GetBoneName(i).ToString());
	}
	return Out;
}

FString UCrowdBakerMaterialIntrospection::DescribeSkelMeshVertices(USkeletalMesh* Mesh, int32 LODIndex, int32 NumSamples)
{
	if (!Mesh) return TEXT("null mesh");
	const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
	if (!RenderData) return TEXT("no render data");
	if (!RenderData->LODRenderData.IsValidIndex(LODIndex)) return FString::Printf(TEXT("LOD %d out of range"), LODIndex);
	const FSkeletalMeshLODRenderData& LOD = RenderData->LODRenderData[LODIndex];
	const int32 NumVerts = LOD.GetNumVertices();
	FString Out = FString::Printf(TEXT("LOD%d numVerts=%d  samples:"), LODIndex, NumVerts);
	for (int32 i = 0; i < FMath::Min(NumSamples, 10); ++i)
	{
		const int32 Idx = (NumSamples > 0 && i < NumSamples) ? (i * (NumVerts / FMath::Max(NumSamples, 1))) : i;
		if (Idx >= NumVerts) break;
		const FVector3f P = LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Idx);
		Out += FString::Printf(TEXT(" v[%d]=(%.1f,%.1f,%.1f)"), Idx, P.X, P.Y, P.Z);
	}
	return Out;
}

FString UCrowdBakerMaterialIntrospection::SampleTexturePixel(UTexture2D* Tex, int32 X, int32 Y)
{
#if WITH_EDITORONLY_DATA
	if (!Tex) return TEXT("null");
	const int32 W = Tex->Source.GetSizeX();
	const int32 H = Tex->Source.GetSizeY();
	if (X < 0 || X >= W || Y < 0 || Y >= H)
	{
		return FString::Printf(TEXT("out of range (W=%d H=%d)"), W, H);
	}
	TArray64<uint8> RawData;
	Tex->Source.GetMipData(RawData, 0, 0, 0, nullptr);
	const int32 Bpp = Tex->Source.GetBytesPerPixel();
	const int64 Off = ((int64)Y * W + X) * Bpp;
	if (Off + Bpp > RawData.Num())
	{
		return TEXT("source buffer truncated");
	}
	FString Out = TEXT("");
	for (int32 c = 0; c < FMath::Min(Bpp, 4); ++c)
	{
		Out += FString::Printf(TEXT("%d%s"), RawData[Off + c], (c < FMath::Min(Bpp, 4)-1) ? TEXT(",") : TEXT(""));
	}
	return Out;
#else
	return TEXT("editor-only");
#endif
}

FString UCrowdBakerMaterialIntrospection::DescribeTextureRowDelta(UTexture2D* Tex, int32 RowA, int32 RowB)
{
#if WITH_EDITORONLY_DATA
	if (!Tex)
	{
		return TEXT("null texture");
	}
	const int32 W = Tex->Source.GetSizeX();
	const int32 H = Tex->Source.GetSizeY();
	const int32 PlatW = Tex->GetSizeX();
	const int32 PlatH = Tex->GetSizeY();
	if (RowA < 0 || RowA >= H || RowB < 0 || RowB >= H)
	{
		return FString::Printf(TEXT("row out of range (SrcH=%d PlatformH=%d)"), H, PlatH);
	}
	TArray64<uint8> RawData;
	Tex->Source.GetMipData(RawData, 0, 0, 0, nullptr);
	const int32 Bpp = Tex->Source.GetBytesPerPixel();
	if (RawData.Num() < (int64)(H) * W * Bpp)
	{
		return FString::Printf(TEXT("got %lld bytes, need %lld (W=%d H=%d bpp=%d)"), (long long)RawData.Num(), (long long)((int64)H*W*Bpp), W, H, Bpp);
	}

	// Compute per-channel sum-of-absolute-differences between the two rows.
	int64 DiffSum[4] = {0,0,0,0};
	int64 MaxDiff[4] = {0,0,0,0};
	const uint8* P = RawData.GetData();
	for (int32 x = 0; x < W; ++x)
	{
		for (int32 c = 0; c < FMath::Min(Bpp, 4); ++c)
		{
			const int32 A = P[(RowA * W + x) * Bpp + c];
			const int32 B = P[(RowB * W + x) * Bpp + c];
			const int32 D = FMath::Abs(A - B);
			DiffSum[c] += D;
			MaxDiff[c] = FMath::Max<int64>(MaxDiff[c], D);
		}
	}
	return FString::Printf(TEXT("SrcW=%d SrcH=%d PlatW=%d PlatH=%d bpp=%d  rowA=%d rowB=%d  sumDiff=[%lld,%lld,%lld,%lld]  maxDiff=[%lld,%lld,%lld,%lld]"),
		W, H, PlatW, PlatH, Bpp, RowA, RowB,
		(long long)DiffSum[0], (long long)DiffSum[1], (long long)DiffSum[2], (long long)DiffSum[3],
		(long long)MaxDiff[0], (long long)MaxDiff[1], (long long)MaxDiff[2], (long long)MaxDiff[3]);
#else
	return TEXT("editor-only");
#endif
}

bool UCrowdBakerMaterialIntrospection::SetMIParent(UMaterialInstanceConstant* MIC, UMaterialInterface* NewParent)
{
#if WITH_EDITORONLY_DATA
	if (!MIC || !NewParent) return false;
	MIC->SetParentEditorOnly(NewParent, /*RecacheShader=*/true);
	UMaterialEditingLibrary::UpdateMaterialInstance(MIC);
	return true;
#else
	return false;
#endif
}

FLinearColor UCrowdBakerMaterialIntrospection::GetEffectiveVector(UMaterialInstance* MI, FName ParameterName)
{
	if (!MI) return FLinearColor::Black;
	FLinearColor Out = FLinearColor::Black;
	const EMaterialParameterAssociation Assocs[] = { EMaterialParameterAssociation::GlobalParameter, EMaterialParameterAssociation::LayerParameter, EMaterialParameterAssociation::BlendParameter };
	for (EMaterialParameterAssociation Assoc : Assocs)
	{
		const int32 Idx = (Assoc == EMaterialParameterAssociation::GlobalParameter) ? INDEX_NONE : 0;
		if (MI->GetVectorParameterValue(FMaterialParameterInfo(ParameterName, Assoc, Idx), Out))
		{
			return Out;
		}
	}
	return FLinearColor::Black;
}

int32 UCrowdBakerMaterialIntrospection::GetEffectiveStaticSwitchTriState(UMaterialInstance* MI, FName ParameterName)
{
	if (!MI)
	{
		return -1;
	}
	bool bValue = false;
	FGuid Guid;
	if (MI->GetStaticSwitchParameterValue(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::GlobalParameter, INDEX_NONE), bValue, Guid))
	{
		return bValue ? 1 : 0;
	}
	if (MI->GetStaticSwitchParameterValue(FMaterialParameterInfo(ParameterName, EMaterialParameterAssociation::LayerParameter, 0), bValue, Guid))
	{
		return bValue ? 1 : 0;
	}
	return -1;
}

UMaterialFunctionInterface* UCrowdBakerMaterialIntrospection::GetMaterialFunctionFromCall(UMaterialExpression* Expression)
{
#if WITH_EDITORONLY_DATA
	if (UMaterialExpressionMaterialFunctionCall* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Expression))
	{
		return Call->MaterialFunction;
	}
#endif
	return nullptr;
}
