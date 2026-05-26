"""Retarget CasualPack's bundled Mannequin walk + idle onto Man1 only (focused test).
After MESH_TO_MESH auto-align on RTG_Man1_UE5."""
import unreal

walk_path = "/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd"
idle_path = "/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/MM_Idle"
src_mesh_path = "/Game/CasualPackV1/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple"

src = unreal.load_asset(src_mesh_path)
man1_mesh = unreal.load_asset("/Game/CasualPackV1/Man_001/Meshes/SK_Man1_UE5")
rtg_man1 = unreal.load_asset("/Game/CasualPackV1/Man_001/Rigs/RTG_Man1_UE5")

ar = unreal.AssetRegistryHelpers.get_asset_registry()
walk_ad = ar.get_asset_by_object_path(f"{walk_path}.{walk_path.split('/')[-1]}")
idle_ad = ar.get_asset_by_object_path(f"{idle_path}.{idle_path.split('/')[-1]}")
assets = [walk_ad, idle_ad]

r = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    assets, src, man1_mesh, rtg_man1, prefix="Man1_")
print(f"Man1: {len(r)} retargeted")
