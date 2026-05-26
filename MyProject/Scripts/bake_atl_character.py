"""Bake skeletal animations into VAT textures for a single character via the CrowdBaker plugin.

CrowdBaker is our fork of UE's AnimToTexture plugin (MyProject/Plugins/CrowdBaker/).
The key difference: the skin-weight projection uses the source skeletal mesh's
per-vertex weights directly (nearest-vertex lookup) instead of the upstream
plugin's distance-based driver-triangle blend. This preserves section
separation — e.g. hair vertices keep their head-only weights instead of bleeding
into body bones.

Run inside the UE editor (Python plugin enabled). Outputs land under
/Game/ATL/<Name>/:
    SM_<Name>_BoneAnimation       (static mesh with bone-anim UV channel)
    T_<Name>_BonePos              (bone position texture)
    T_<Name>_BoneRot              (bone rotation texture)
    T_<Name>_BoneWeight           (per-vertex bone weights, section-aware)
    DA_<Name>_BoneAnimation       (source UCrowdBakerDataAsset; re-runnable)
    MI_<Name>_BoneAnimation       (material instance based on engine ML_BoneAnimation)
"""

import unreal

ATL_ROOT = "/Game/ATL"


def _ensure_dir(content_path: str) -> None:
    eal = unreal.EditorAssetLibrary
    if not eal.does_directory_exist(content_path):
        eal.make_directory(content_path)


def _create_or_load_data_asset(package_path: str, asset_name: str) -> "unreal.CrowdBakerDataAsset":
    full = f"{package_path}/{asset_name}"
    existing = unreal.load_asset(full)
    if existing:
        return existing
    aTools = unreal.AssetToolsHelpers.get_asset_tools()
    return aTools.create_asset(asset_name, package_path, unreal.CrowdBakerDataAsset, None)


def _duplicate_mi_from_manny(out_dir: str, name: str) -> "unreal.MaterialInstanceConstant":
    """Duplicate the working Manny MI (AutoPlay=Off, UseDynamicParameters=Off) and
    rebind to this character's bone textures. The MI parents the engine
    ML_BoneAnimation layer; we don't touch that side of the pipeline.
    """
    mi_path = f"{out_dir}/MI_{name}_BoneAnimation"
    if unreal.EditorAssetLibrary.does_asset_exist(mi_path):
        unreal.EditorAssetLibrary.delete_asset(mi_path)
    unreal.EditorAssetLibrary.duplicate_asset("/Game/ATL/Manny/MI_Manny_BoneAnimation", mi_path)
    mi = unreal.load_asset(mi_path)
    mel = unreal.MaterialEditingLibrary
    for slot, tex_path in [
        ("BonePositionTexture", f"{out_dir}/T_{name}_BonePos"),
        ("BoneRotationTexture", f"{out_dir}/T_{name}_BoneRot"),
        ("BoneWeightsTexture",  f"{out_dir}/T_{name}_BoneWeight"),
    ]:
        mel.set_material_instance_texture_parameter_value(mi, slot, unreal.load_asset(tex_path))
    unreal.EditorAssetLibrary.save_asset(mi_path, only_if_is_dirty=False)
    return mi


def bake_character(
    name: str,
    skel_mesh_path: str,
    walk_anim_path: str,
    idle_anim_path: str,
    skel_lod: int = 0,
    precision_bits: int = 8,
    sample_rate: float = 30.0,
    uv_channel: int = 1,
    num_bone_influences: "unreal.CrowdBakerNumBoneInfluences" = None,
) -> dict:
    """Bake walk+idle for `name`. Returns a dict of created asset paths."""
    if num_bone_influences is None:
        num_bone_influences = unreal.CrowdBakerNumBoneInfluences.FOUR

    out_dir = f"{ATL_ROOT}/{name}"
    _ensure_dir(out_dir)

    skel_mesh = unreal.load_asset(skel_mesh_path)
    if not skel_mesh:
        raise RuntimeError(f"Could not load skeletal mesh at {skel_mesh_path}")

    # 1. Static mesh target (one-time conversion). CrowdBaker preserves source
    # vertex order so the nearest-vertex skin-weight lookup is effectively O(1).
    sm_name = f"SM_{name}_BoneAnimation"
    sm_full = f"{out_dir}/{sm_name}"
    static_mesh = unreal.load_asset(sm_full)
    if not static_mesh:
        static_mesh = unreal.CrowdBakerBPLibrary.convert_skeletal_mesh_to_static_mesh(
            skel_mesh, sm_full, skel_lod
        )
        if not static_mesh:
            raise RuntimeError("ConvertSkeletalMeshToStaticMesh failed (check log)")

    # Disable auto-generated lightmap UVs — they collide with the ATL UV channel.
    ses = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for lod_idx in range(ses.get_lod_count(static_mesh)):
        bs = ses.get_lod_build_settings(static_mesh, lod_idx)
        bs.set_editor_property("generate_lightmap_u_vs", False)
        ses.set_lod_build_settings(static_mesh, lod_idx, bs)
    static_mesh.set_editor_property("light_map_coordinate_index", 0)

    # 2. Data asset.
    da_name = f"DA_{name}_BoneAnimation"
    da: "unreal.CrowdBakerDataAsset" = _create_or_load_data_asset(out_dir, da_name)
    da.set_editor_property("skeletal_mesh", skel_mesh)
    da.set_editor_property("skeletal_lod_index", skel_lod)
    da.set_editor_property("static_mesh", static_mesh)
    da.set_editor_property("static_lod_index", 0)
    da.set_editor_property("uv_channel", uv_channel)
    da.set_editor_property("max_height", 4096)
    da.set_editor_property("max_width", 4096)
    da.set_editor_property("mode", unreal.CrowdBakerMode.BONE)
    da.set_editor_property("precision",
        unreal.CrowdBakerPrecision.EIGHT_BITS if precision_bits == 8
        else unreal.CrowdBakerPrecision.SIXTEEN_BITS)
    da.set_editor_property("num_bone_influences", num_bone_influences)
    da.set_editor_property("sample_rate", sample_rate)
    da.set_editor_property("auto_play", False)
    da.set_editor_property("animation_index", 0)  # walk=0, idle=1

    walk = unreal.load_asset(walk_anim_path)
    idle = unreal.load_asset(idle_anim_path)
    if not walk or not idle:
        raise RuntimeError("Failed to load anim sequences")

    walk_info = unreal.CrowdBakerAnimSequenceInfo()
    walk_info.set_editor_property("enabled", True)
    walk_info.set_editor_property("anim_sequence", walk)
    walk_info.set_editor_property("use_custom_range", False)

    idle_info = unreal.CrowdBakerAnimSequenceInfo()
    idle_info.set_editor_property("enabled", True)
    idle_info.set_editor_property("anim_sequence", idle)
    idle_info.set_editor_property("use_custom_range", False)

    da.set_editor_property("anim_sequences", [walk_info, idle_info])

    # Pre-create texture targets by duplicating engine sample textures (avoids
    # null-slot assertion at AnimToTextureUtils.h:161 / our fork's equivalent).
    def _ensure_tex(slot_attr: str, tex_name: str, donor_path: str) -> None:
        full = f"{out_dir}/{tex_name}"
        if not unreal.EditorAssetLibrary.does_asset_exist(full):
            unreal.EditorAssetLibrary.duplicate_asset(donor_path, full)
        da.set_editor_property(slot_attr, unreal.load_asset(full))

    _ensure_tex("bone_position_texture", f"T_{name}_BonePos",
                "/AnimToTexture/Characters/Mannequin/Textures/BoneAnimation/TX_BonePosition")
    _ensure_tex("bone_rotation_texture", f"T_{name}_BoneRot",
                "/AnimToTexture/Characters/Mannequin/Textures/BoneAnimation/TX_BoneRotation")
    _ensure_tex("bone_weight_texture", f"T_{name}_BoneWeight",
                "/AnimToTexture/Characters/Mannequin/Textures/BoneAnimation/TX_BoneWeight")

    unreal.EditorAssetLibrary.save_loaded_asset(da)

    # 3. Bake (CrowdBaker path: section-aware skin weights).
    ok = unreal.CrowdBakerBPLibrary.animation_to_texture(da)
    if not ok:
        raise RuntimeError("CrowdBakerBPLibrary.animation_to_texture returned false")

    # 4. Material instance.
    mi = _duplicate_mi_from_manny(out_dir, name)

    # 5. Save.
    for path in [
        f"{out_dir}/{sm_name}",
        f"{out_dir}/{da_name}",
        f"{out_dir}/T_{name}_BonePos",
        f"{out_dir}/T_{name}_BoneRot",
        f"{out_dir}/T_{name}_BoneWeight",
        f"{out_dir}/MI_{name}_BoneAnimation",
    ]:
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)

    result = {
        "static_mesh": f"{out_dir}/{sm_name}",
        "data_asset":  f"{out_dir}/{da_name}",
        "bone_pos":    f"{out_dir}/T_{name}_BonePos",
        "bone_rot":    f"{out_dir}/T_{name}_BoneRot",
        "bone_weight": f"{out_dir}/T_{name}_BoneWeight",
        "material":    f"{out_dir}/MI_{name}_BoneAnimation",
    }
    unreal.log(f"[bake_atl_character] {name} -> {result}")
    return result
