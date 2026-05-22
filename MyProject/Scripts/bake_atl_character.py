"""Bake AnimToTexture data for a single character.

Run inside the UE editor (Python plugin enabled). The MCP bridge calls this as
a one-shot when prepping a new character variant for the crowd.

Usage from the MCP Python tool:
    exec(open(r"C:\\LOCAL_DEV\\UNREAL\\crowd_test_cpp\\MyProject\\Scripts\\bake_atl_character.py").read())
    bake_character(
        name="Girl1",
        skel_mesh_path="/Game/CasualPackV1/Girl_001/Meshes/SK_Girl1_UE5",
        walk_anim_path="/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd",
        idle_anim_path="/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/MM_Idle",
        skel_lod=2,  # use a low-res LOD for the Quest crowd
    )

Output assets land under /Game/ATL/<Name>/:
    SM_<Name>_BoneAnimation       (static mesh with bone-anim UV channel)
    T_<Name>_BonePos              (bone position texture)
    T_<Name>_BoneRot              (bone rotation texture)
    T_<Name>_BoneWeight           (per-vertex bone weights)
    DA_<Name>_BoneAnimation       (the source UAnimToTextureDataAsset; re-runnable)
    MI_<Name>_BoneAnimation       (material instance based on engine ML_BoneAnimation)
"""

import unreal

ATL_ROOT = "/Game/ATL"


def _ensure_dir(content_path: str) -> None:
    eal = unreal.EditorAssetLibrary
    if not eal.does_directory_exist(content_path):
        eal.make_directory(content_path)


def _create_or_load_data_asset(package_path: str, asset_name: str) -> unreal.AnimToTextureDataAsset:
    full = f"{package_path}/{asset_name}"
    existing = unreal.load_asset(full)
    if existing:
        return existing
    aTools = unreal.AssetToolsHelpers.get_asset_tools()
    da = aTools.create_asset(asset_name, package_path, unreal.AnimToTextureDataAsset, None)
    return da


def _create_or_load_material_instance(package_path: str, asset_name: str,
                                       parent_material_path: str) -> unreal.MaterialInstanceConstant:
    full = f"{package_path}/{asset_name}"
    existing = unreal.load_asset(full)
    if existing:
        return existing
    aTools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = aTools.create_asset(asset_name, package_path, unreal.MaterialInstanceConstant, factory)
    parent_mat = unreal.load_asset(parent_material_path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, parent_mat)
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
    parent_material_path: str = "/AnimToTexture/Materials/M_BoneAnimation.M_BoneAnimation",
) -> dict:
    """Bake walk+idle for `name`. Returns a dict of created asset paths."""
    out_dir = f"{ATL_ROOT}/{name}"
    _ensure_dir(out_dir)

    skel_mesh = unreal.load_asset(skel_mesh_path)
    if not skel_mesh:
        raise RuntimeError(f"Could not load skeletal mesh at {skel_mesh_path}")

    # 1. Create the static mesh target (one-time conversion).
    sm_name = f"SM_{name}_BoneAnimation"
    sm_full = f"{out_dir}/{sm_name}"
    static_mesh = unreal.load_asset(sm_full)
    if not static_mesh:
        static_mesh = unreal.AnimToTextureBPLibrary.convert_skeletal_mesh_to_static_mesh(
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
    # NB: SetLodBuildSettings re-runs the mesh build internally, so no explicit Build call needed.

    # 2. Build the data asset.
    da_name = f"DA_{name}_BoneAnimation"
    da: unreal.AnimToTextureDataAsset = _create_or_load_data_asset(out_dir, da_name)

    da.set_editor_property("skeletal_mesh", skel_mesh)
    da.set_editor_property("skeletal_lod_index", skel_lod)
    da.set_editor_property("static_mesh", static_mesh)
    da.set_editor_property("static_lod_index", 0)
    da.set_editor_property("uv_channel", uv_channel)
    da.set_editor_property("max_height", 4096)
    da.set_editor_property("max_width", 4096)
    da.set_editor_property("mode", unreal.AnimToTextureMode.BONE)
    da.set_editor_property("precision",
        unreal.AnimToTexturePrecision.EIGHT_BITS if precision_bits == 8
        else unreal.AnimToTexturePrecision.SIXTEEN_BITS)
    da.set_editor_property("num_bone_influences", unreal.AnimToTextureNumBoneInfluences.TWO)
    da.set_editor_property("sample_rate", sample_rate)
    da.set_editor_property("auto_play", True)
    da.set_editor_property("animation_index", 0)  # walk = 0, idle = 1

    walk = unreal.load_asset(walk_anim_path)
    idle = unreal.load_asset(idle_anim_path)
    if not walk or not idle:
        raise RuntimeError("Failed to load anim sequences")

    walk_info = unreal.AnimToTextureAnimSequenceInfo()
    walk_info.set_editor_property("enabled", True)
    walk_info.set_editor_property("anim_sequence", walk)
    walk_info.set_editor_property("use_custom_range", False)

    idle_info = unreal.AnimToTextureAnimSequenceInfo()
    idle_info.set_editor_property("enabled", True)
    idle_info.set_editor_property("anim_sequence", idle)
    idle_info.set_editor_property("use_custom_range", False)

    da.set_editor_property("anim_sequences", [walk_info, idle_info])

    # Note: AnimationToTexture() creates the bone textures inside the data asset's
    # package directory if the slots are nullptr. We don't pre-create them.

    unreal.EditorAssetLibrary.save_loaded_asset(da)

    # 3. Bake.
    ok = unreal.AnimToTextureBPLibrary.animation_to_texture(da)
    if not ok:
        raise RuntimeError("AnimationToTexture returned false (check log)")

    # 4. Material instance hooked to baked textures.
    mi_name = f"MI_{name}_BoneAnimation"
    mi = _create_or_load_material_instance(out_dir, mi_name, parent_material_path)
    unreal.AnimToTextureBPLibrary.update_material_instance_from_data_asset(
        da, mi, unreal.MaterialParameterAssociation.LAYER_PARAMETER
    )

    # 5. Save everything.
    for path in [
        f"{out_dir}/{sm_name}",
        f"{out_dir}/{da_name}",
        f"{out_dir}/T_{name}_BonePos",
        f"{out_dir}/T_{name}_BoneRot",
        f"{out_dir}/T_{name}_BoneWeight",
        f"{out_dir}/{mi_name}",
    ]:
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)

    result = {
        "static_mesh": f"{out_dir}/{sm_name}",
        "data_asset":  f"{out_dir}/{da_name}",
        "bone_pos":    f"{out_dir}/T_{name}_BonePos",
        "bone_rot":    f"{out_dir}/T_{name}_BoneRot",
        "bone_weight": f"{out_dir}/T_{name}_BoneWeight",
        "material":    f"{out_dir}/{mi_name}",
    }
    unreal.log(f"[bake_atl_character] {name} -> {result}")
    return result
