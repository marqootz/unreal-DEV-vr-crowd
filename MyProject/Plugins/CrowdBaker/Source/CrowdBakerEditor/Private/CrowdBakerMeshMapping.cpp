// Copyright Epic Games, Inc. All Rights Reserved.
#include "CrowdBakerMeshMapping.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"

namespace CrowdBaker_Private
{

void FSourceVertexData::Update(const FVector3f& SourceVertex,
	const TArray<FVector3f>& DriverVertices, const TArray<FIntVector3>& DriverTriangles, const TArray<VertexSkinWeightMax>& DriverSkinWeights, 
	const int32 NumDrivers, const float Sigma)
{	
	const int32 NumDriverTriangles = DriverTriangles.Num();
	const int32 NDriverTriangles = FMath::Clamp(NumDrivers, 1, NumDriverTriangles);

	// Get Distances from Vertex to Each Triangle (ClosestPoint)
	TArray<TPair<float, int32>> SortedDistances;
	TArray<FVector3f> ClosestPoints;
	{
		SortedDistances.SetNumUninitialized(NumDriverTriangles);
		ClosestPoints.SetNumUninitialized(NumDriverTriangles);

		for (int32 DriverTriangleIndex = 0; DriverTriangleIndex < NumDriverTriangles; DriverTriangleIndex++)
		{
			const FIntVector3& DriverTriangle = DriverTriangles[DriverTriangleIndex];
			const FVector3f& A = DriverVertices[DriverTriangle.X];
			const FVector3f& B = DriverVertices[DriverTriangle.Y];
			const FVector3f& C = DriverVertices[DriverTriangle.Z];

			// ClosestPoint To Triangle 
			ClosestPoints[DriverTriangleIndex] = FindClosestPointToTriangle(SourceVertex, A, B, C);

			// Distance To Triangle
			const float Distance = FVector3f::Distance(SourceVertex, ClosestPoints[DriverTriangleIndex]);
			SortedDistances[DriverTriangleIndex] = TPair<float, int32>(Distance, DriverTriangleIndex);
		};

		// Sort By Distance
		SortedDistances.Sort();
	}

	// Get Inverse Distance from Vertex to N-Closest Triangles
	TArray<float> NWeights;
	{
		TArray<FVector3f> NClosestPoints;
		NClosestPoints.SetNumUninitialized(NDriverTriangles);

		for (int32 Index = 0; Index < NDriverTriangles; Index++)
		{
			const int32& DriverTriangleIndex = SortedDistances[Index].Value;
			NClosestPoints[Index] = ClosestPoints[DriverTriangleIndex];
		}

		CrowdBaker_Private::InverseDistanceWeights(SourceVertex, NClosestPoints, NWeights, Sigma);
	}

	DriverTriangleData.Reserve(NDriverTriangles);
	for (int32 Index = 0; Index < NDriverTriangles; Index++)
	{
		const int32& DriverTriangleIndex = SortedDistances[Index].Value;

		if (NWeights[Index] > UE_KINDA_SMALL_NUMBER)
		{
			const FVector3f& ClosestPoint = ClosestPoints[DriverTriangleIndex];
			const FIntVector3& DriverTriangle = DriverTriangles[DriverTriangleIndex];
			const FVector3f& A = DriverVertices[DriverTriangle.X];
			const FVector3f& B = DriverVertices[DriverTriangle.Y];
			const FVector3f& C = DriverVertices[DriverTriangle.Z];

			FSourceVertexDriverTriangleData TriangleData;
			TriangleData.InverseDistanceWeight = NWeights[Index];
			TriangleData.Triangle = DriverTriangle;
			TriangleData.BarycentricCoords = BarycentricCoordinates(ClosestPoint, A, B, C);
			TriangleData.TangentLocalIndex = GetTriangleTangentLocalIndex(ClosestPoint, A, B, C);
			TriangleData.InvMatrix = GetTriangleMatrix(ClosestPoint, A, B, C, TriangleData.TangentLocalIndex).Inverse();

			// Interpolate SkinWeights with Barycentric Coords
			const TArray<VertexSkinWeightMax> SkinWeights = { DriverSkinWeights[TriangleData.Triangle.X], DriverSkinWeights[TriangleData.Triangle.Y], DriverSkinWeights[TriangleData.Triangle.Z] };
			const TArray<float> BarycentricWeights = { TriangleData.BarycentricCoords.X, TriangleData.BarycentricCoords.Y, TriangleData.BarycentricCoords.Z };
			InterpolateVertexSkinWeights(SkinWeights, BarycentricWeights, TriangleData.SkinWeights);

			DriverTriangleData.Add(TriangleData);
		}
	}
}


void FSourceMeshToDriverMesh::Update(const UStaticMesh* StaticMesh, const int32 StaticMeshLODIndex, 
	const USkeletalMesh* SkeletalMesh, const int32 SkeletalMeshLODIndex, 
	const int32 NumDrivers, const float Sigma)
{
	check(StaticMesh);
	check(SkeletalMesh);

	// Get StaticMesh Vertices
	const int32 NumSourceVertices = GetVertices(StaticMesh, StaticMeshLODIndex, SourceVertices, SourceNormals);

	// Get SkeletalMesh Vertices
	const int32 NumDriverVertices = GetVertices(SkeletalMesh, SkeletalMeshLODIndex, DriverVertices);

	// Get SkeletalMesh Triangles
	const int32 NumDriverTriangles = GetTriangles(SkeletalMesh, SkeletalMeshLODIndex, DriverTriangles);

	// Get SkeletalMesh SkinWeights
	GetSkinWeights(SkeletalMesh, SkeletalMeshLODIndex, DriverSkinWeights);

	// Allocate
	SourceVerticesData.SetNumZeroed(NumSourceVertices); // note this is initializing values as zero
	
	// Get SourceVertex -> DriverTriangle Data
	ParallelFor(NumSourceVertices, [&](int32 SourceVertexIndex)
	{	
		// Create Mapping from StaticMesh Vertex to SkeletalMesh Triangles
		SourceVerticesData[SourceVertexIndex].Update(SourceVertices[SourceVertexIndex], 
			DriverVertices, DriverTriangles, DriverSkinWeights, NumDrivers, Sigma);

		// UE_LOG(LogTemp, Warning, TEXT("Vertex: %i NumTriangles: %i."), SourceVertexIndex, SourceVerticesData[SourceVertexIndex].DriverTriangleData.Num());

	});	// end ParallelFor
}

int32 FSourceMeshToDriverMesh::GetNumSourceVertices() const
{
	return SourceVerticesData.Num();
}

int32 FSourceMeshToDriverMesh::GetSourceVertices(TArray<FVector3f>& OutVertices) const
{
	OutVertices = SourceVertices;
	return OutVertices.Num();
}

int32 FSourceMeshToDriverMesh::GetSourceNormals(TArray<FVector3f>& OutNormals) const
{
	OutNormals = SourceNormals;
	return OutNormals.Num();
}

void FSourceMeshToDriverMesh::DeformVerticesAndNormals(const TArray<FVector3f>& InDriverVertices,
	TArray<FVector3f>& OutVertices, TArray<FVector3f>& OutNormals) const
{
	// Source Vertices
	const int32 NumSourceVertices = SourceVerticesData.Num();
	OutVertices.SetNumZeroed(NumSourceVertices);
	OutNormals.SetNumZeroed(NumSourceVertices);

	// Driver Triangles
	const int32 NumDriverTriangles = DriverTriangles.Num();

	// Deform Source Vertices and Normals
	ParallelFor(NumSourceVertices, [&](int32 SourceVertexIndex)
	{
		const FVector3f& SourceVertex = SourceVertices[SourceVertexIndex];
		const FVector3f& SourceNormal = SourceNormals[SourceVertexIndex];

		for (const FSourceVertexDriverTriangleData& TriangleData: SourceVerticesData[SourceVertexIndex].DriverTriangleData)
		{
			const FVector3f& A = InDriverVertices[TriangleData.Triangle.X];
			const FVector3f& B = InDriverVertices[TriangleData.Triangle.Y];
			const FVector3f& C = InDriverVertices[TriangleData.Triangle.Z];

			// Get Driver Triangle Point At Barycentric
			const FVector3f& BarycentricCoords = TriangleData.BarycentricCoords;
			const FVector3f Point = PointAtBarycentricCoordinates(A, B, C, BarycentricCoords);

			// Grt Driver Triangle Matrix
			const uint8& TangentLocalIndex = TriangleData.TangentLocalIndex;
			const FMatrix44f& DriverTriangleInvMatrix = TriangleData.InvMatrix;
			const FMatrix44f DriverTriangleMatrix = GetTriangleMatrix(Point, A, B, C, TangentLocalIndex);

			// Get SourceVertex -> DriverTriangle InverseDistanceWeight
			const float& InverseDistanceWeight = TriangleData.InverseDistanceWeight;

			// Tranform Weighted Source Vertex and Normal
			OutVertices[SourceVertexIndex] += DriverTriangleMatrix.TransformPosition(DriverTriangleInvMatrix.TransformPosition(SourceVertex)) * InverseDistanceWeight;
			OutNormals[SourceVertexIndex] += DriverTriangleMatrix.TransformVector(DriverTriangleInvMatrix.TransformVector(SourceNormal)) * InverseDistanceWeight;
		}

	}); // end ParallelFor
}

void FSourceMeshToDriverMesh::ProjectSkinWeights(TArray<VertexSkinWeightMax>& OutSkinWeights) const
{
	// CrowdBaker change: bypass the distance-based driver-triangle blend. That
	// blend pulls in body bones for hair vertices when the static mesh has
	// topologically-separate sections (head + hair shell). For each static
	// mesh vertex find the nearest source skel mesh vertex and copy its skin
	// weights directly — preserves authored section-aware weights.
	const int32 NumSourceVertices = SourceVerticesData.Num();
	const int32 NumDriverVertices = DriverVertices.Num();
	OutSkinWeights.SetNumUninitialized(NumSourceVertices);

	if (NumDriverVertices == 0)
	{
		return;
	}

	// Brute-force nearest-vertex per source vertex. The fast-path index-equality
	// optimization is removed for now to keep diagnostic behavior pure: every
	// source vertex picks its provably-nearest driver vertex.
	ParallelFor(NumSourceVertices, [&](int32 SourceIdx)
	{
		const FVector3f& SourcePos = SourceVertices[SourceIdx];
		int32 BestDriverIdx = 0;
		float BestDistSq = TNumericLimits<float>::Max();
		for (int32 DriverIdx = 0; DriverIdx < NumDriverVertices; ++DriverIdx)
		{
			const float DistSq = FVector3f::DistSquared(SourcePos, DriverVertices[DriverIdx]);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestDriverIdx = DriverIdx;
			}
		}
		OutSkinWeights[SourceIdx] = DriverSkinWeights[BestDriverIdx];

		// Diagnostic: log the first few hair-area vertex assignments so we can
		// see what bone indices are being picked. "Hair" heuristic: high Z
		// values (head/hair territory).
		if (SourceIdx < 20 || (SourcePos.Z > 160.f && SourceIdx % 50 == 0))
		{
			FString IndicesStr;
			FString WeightsStr;
			for (int32 i = 0; i < MAX_TOTAL_INFLUENCES && i < 4; ++i)
			{
				IndicesStr += FString::Printf(TEXT("%d,"), OutSkinWeights[SourceIdx].MeshBoneIndices[i]);
				WeightsStr += FString::Printf(TEXT("%d,"), OutSkinWeights[SourceIdx].BoneWeights[i]);
			}
			UE_LOG(LogTemp, Display, TEXT("[ProjectSkinWeights] src[%d] pos=(%.1f,%.1f,%.1f) -> driver[%d] (dist=%.3f) bones=[%s] weights=[%s]"),
				SourceIdx, SourcePos.X, SourcePos.Y, SourcePos.Z, BestDriverIdx, FMath::Sqrt(BestDistSq), *IndicesStr, *WeightsStr);
		}
	});
}


} // end namespace CrowdBaker_Private
