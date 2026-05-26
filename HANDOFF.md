# CrowdSim — Handoff & Onboarding

UE 5.6 background-character crowd: ~100 agents wander a corridor, avoid each
other, and render as **VAT-animated instanced static meshes** (AnimToTexture
bone-mode) baked by the project's **CrowdBaker** plugin (a fork of the engine's
AnimToTexture). No Niagara — MassCrowd representation drives all LODs.

This doc covers two paths:
- **Path A** — clone and run/continue *this* project.
- **Path B** — lift the crowd system into *another* UE project.

> Convention notes worth knowing up front:
> - The C++ module is named **`MyProject`** (not "CrowdSim", despite the title).
> - `LogCrowdSim` is the log category.
> - See `CLAUDE.md` for the working agreement (build command, code conventions).

---

## 0. Prerequisites

| Requirement | Detail |
|---|---|
| Engine | **Unreal Engine 5.6** (locked at 5.6.1; `EngineAssociation` = `5.6`). Installed build at `C:\Program Files\Epic Games\UE_5.6`. |
| OS / toolchain | Windows + Visual Studio 2022 (v14.38 toolchain used here). Dev was done from WSL driving `cmd.exe`; native Windows works too. |
| IDE | Rider is primary; any UE-capable IDE is fine. |
| Python plugin | UE **Python Script Plugin** must be enabled to run the bake script. |

### Out-of-repo dependencies (the things a clone does NOT give you)

These are deliberately or unavoidably **not** in git. Request them from the
previous owner:

1. **CasualPack V1 source meshes** — a purchased asset pack. `.gitignore` tracks
   **only `Girl_001`** (the other 12 characters are ~100 MB each of duplicate
   textures). Consequence: of the 6 crowd characters, **only Girl1 is fully
   re-bakeable from a clone**; Man1/Man2/Girl2/Kid1/Kid2's rebound source meshes
   live under gitignored CasualPack folders.
2. **`retarget.blend`** — the Blender scene used to rebind CC meshes onto the
   Manny skeleton. *Per project notes it lives at*
   `G:\My Drive\GENERATE\Blender\characters\retarget.blend` (Google Drive —
   **verify this path with the owner**). Needed only to re-rig/re-skin a
   character, not to run the crowd.
3. **MCP tooling** (optional, dev-convenience) — Unreal MCP bridge + Blender MCP
   addon. See §7. Not required to build, run, or bake.

**The running crowd itself needs none of the above** — all baked outputs, configs,
plugin source, and C++ are committed.

---

## Path A — Run this project from a clone

1. **Clone** `git@github.com:marqootz/unreal-DEV-vr-crowd.git`, branch
   **`crowd-atl-pivot`** (the active branch; `main` predates the VAT pivot).
   Note: binary-heavy repo (one 52 MB texture trips GitHub's soft limit — see §8).
2. **Generate project files** — right-click `MyProject/MyProject.uproject` →
   *Generate Visual Studio project files* (installed-engine builds don't ship
   `GenerateProjectFiles.bat`).
3. **Build** `MyProjectEditor` (Win64 Development). Wrapper exists:
   ```
   run_build.bat
   ```
   which calls:
   ```
   "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" ^
     MyProjectEditor Win64 Development ^
     -project="<repo>\MyProject\MyProject.uproject" -waitmutex
   ```
   This compiles `MyProject` **and** the CrowdBaker plugin modules.
4. **Open** the project in the editor.
5. **Open level `/Game/test3`** (NOT "Untitled" — that was a transient,
   never-saved world; the crowd lives on `test3`).
6. **PIE.** You should see ~100 characters (Man1/Man2/Girl1/Girl2/Kid1/Kid2)
   walk along the corridor, face their direction of travel, softly avoid each
   other, and play a held standing pose when idle. Kids render shorter.

If the crowd is missing, confirm you're in `test3` and that the `MassSpawner`
actor's `count`/`entity_types` are populated (see §3).

---

## Path B — Port the crowd system into another UE project

The system has three separable layers. Copy what you need.

### B1. The CrowdBaker plugin (the bake tooling)
1. Copy **`MyProject/Plugins/CrowdBaker/`** into your project's `Plugins/`
   (skip `Binaries/` and `Intermediate/`).
2. CrowdBaker is **not listed in `.uproject`** — it auto-enables as a project
   plugin (`Installed:false`, default-enabled) and is pulled in by the module
   dependency below. If you prefer explicitness, add it to your `.uproject`.
3. It defines two modules: **`CrowdBakerRuntime`** (LoadingPhase `PreDefault`) and
   **`CrowdBakerEditor`** (Editor, `PreLoadingScreen`). The bake itself is editor-only.
4. The plugin's bundled `Content/Characters/Mannequin/.../VertexAnimation/` demo
   assets are **unused** (we bake bone-mode). Safe to delete — also drops the
   52 MB texture (§8).

### B2. The Mass runtime (wander + separation + VAT playback)
Copy these from `MyProject/Source/MyProject/` (header+source pairs):

| Class | Role |
|---|---|
| `WandererProcessor` / `WandererTrait` / `WandererFragment` | Corridor locomotion: archetype-weighted speed, lane offset/drift, walk↔idle state machine. The trait also adds the **separation** shared param. |
| `CrowdSeparationProcessor` | Boid-style soft repulsion (XY plane). O(N²). |
| `FacingProcessor` | Slerp yaw toward velocity (mesh faces -Y → -90° offset). |
| `CrowdAnimProcessor` / `CrowdAnimTrait` / `CrowdAnimFragment` | Advances per-instance anim time + walk-blend; applies per-character `RenderScale` to the transform. |
| `PushAnimToISMProcessor` | Writes `FCrowdBakerFrameData{Frame,PrevFrame}` (2 floats) into ISM per-instance custom data. |
| `PlayerAvoidanceProcessor` + `AvoidanceTarget*` | Optional: pushes crowd away from a registered player target. |

Then add the module deps to your `*.Build.cs`:
```
MassEntity, MassCommon, MassMovement, MassNavigation, MassSpawner,
MassRepresentation, MassCrowd, MassActors, MassLOD,
ZoneGraph, StateTreeModule, AnimToTexture, CrowdBakerRuntime
```
…and enable plugins in `.uproject`: `MassGameplay, MassCrowd, MassAI,
ZoneGraph, ZoneGraphAnnotations, StateTree, AnimToTexture`.

> **`MassEntity` is NOT a plugin in UE 5.5+** — it folded into engine core. Do
> not add it to `.uproject` Plugins; the `MassEntity` *module* dep above resolves it.

All processors self-register via `bAutoRegisterWithProcessingPhases = true` — no
manual registration. Execution order: **Movement group** runs
`Wanderer → CrowdSeparation (ExecuteAfter Wanderer) → Facing → CrowdAnim`; the
engine's **Representation** group then pushes transforms to the ISM; finally
`PushAnimToISM` runs **after Representation** (game-thread) to write custom data.

### B3. Content (the VAT material + a baked character)
1. Copy the master material **`/Game/ATL/M_CrowdBaker_Character`** (a **layered**
   material using `ML_BoneAnimation`).
2. Either copy a baked character folder (`/Game/ATL/<Char>/`) or bake your own
   (§4).
3. Build a `MassEntityConfigAsset` per character with the traits in §3.
4. Place a `MassSpawner` with your configs; set `count`.

---

## 3. Runtime architecture (data flow)

```
Spawn (MassSpawner, count=100, 6 configs)
  │
  ▼  per tick, Movement group:
WandererProcessor      → integrates velocity (corridor/lane/state machine)
CrowdSeparationProcessor → adds soft repulsion impulse (radius 120cm, XY)
FacingProcessor        → slerps yaw toward velocity
CrowdAnimProcessor     → CurrentTime += dt·timeScale; eases WalkBlend;
                         writes RenderScale into FTransformFragment scale
  │
  ▼  Representation group (engine):
UMassUpdateISMProcessor → pushes FTransformFragment (incl. scale) to the ISM
  │
  ▼  after Representation, game-thread:
PushAnimToISMProcessor  → picks Walk(0)/Idle(1) by WalkBlend vs threshold,
                          samples DataAsset → FCrowdBakerFrameData{Frame,PrevFrame},
                          ISMInfo.AddBatchedCustomData (2 floats per instance)
  │
  ▼
M_CrowdBaker_Character (in ML_BoneAnimation layer) reads
PerInstanceCustomData[0..1] + bone textures → displaces verts in the vertex shader
```

**Per-character config (`MassEntityConfig_<Char>`)** — the traits that matter:
- **Crowd Anim State** (`CrowdAnimTrait`): `Params.DataAsset` → `DA_<Char>_BoneAnimation`;
  `WalkAnimIndex=0`, `IdleAnimIndex=1`; **`RenderScale`** (1.0 adults, **0.85 kids**).
- **Wanderer** (`WandererTrait`): locomotion archetypes + corridor params; also
  carries the `FCrowdSeparationParams` (radius 120 / strength 2000 / maxImpulse 800).
- **MassCrowdVisualizationTrait**: `static_mesh_instance_desc.meshes[0].mesh` →
  `SM_<Char>_BoneAnimation`, with `material_overrides=[None]` so the **SM's own
  section materials** are used — those MUST be the VAT material instances.

---

## 4. Adding / re-baking a character

Entry point: **`MyProject/Scripts/bake_atl_character.py`**, run inside the editor
(Python plugin). Core call:

```python
import bake_atl_character as bake
bake.bake_character(
    name="Man1",
    skel_mesh_path="/Game/CasualPackV1/Man_001/Meshes/Mannequin/SKM_Man1_Mannequin",
    walk_anim_path="/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd",
    idle_anim_path="/Game/CasualPackV1/Demo/Characters/Mannequins/Anims/Unarmed/MM_Idle",
    precision_bits=16,   # always 16 for these characters (8-bit wrecks subtle poses)
)
```

Outputs land under `/Game/ATL/<Name>/`: `SM_*_BoneAnimation`, `T_*_BonePos/Rot/Weight`,
`DA_*_BoneAnimation` (re-runnable), `MI_*_BoneAnimation`.

**Prerequisite: a Manny-skeleton-bound skeletal mesh.** The bake reads
`SKM_<Char>_Mannequin` (the CC mesh rebound to `SK_Mannequin` in Blender). If you
only have a raw CC mesh, you must first do the **Blender rebind** (§5).

> **Idle is currently a held walk-frame, not MM_Idle.** Real MM_Idle "crumples to
> a ball" in this VAT path (root cause undiagnosed — see §6). The shipped configs
> bake `[walk full, walk frame-0 held]` as idle. Don't re-bake MM_Idle expecting
> it to work without solving that first.

---

## 5. Blender rebind (only when re-rigging)

Needed to bind a child/CC mesh onto the adult `SK_Mannequin`. Source of truth is
`retarget.blend` (§0). Pipeline (proven on Man1 + Girl1):

1. Export `SK_<Char>_UE5.fbx` from UE first (`AssetExportTask`, FBX_2013).
2. In `retarget.blend` (has the `root` Manny armature): import FBX, split mesh by
   material into Body/Clothes/Hair, run the **anatomical-segment skinning** script
   (head→child-head segments, side affinity, branching joints, exclude
   `ik_*`/`*_twist_*`/control bones; hair → 100% `head`; limit-4 + normalize).
3. Export selection (`root` + 3 meshes) with axes **`-Z forward, Y up`**,
   `use_armature_deform_only=True`, `add_leaf_bones=False`, `bake_anim=False`.
4. Import into UE **assigning the existing `SK_Mannequin` skeleton** (drag-drop
   manually — MCP import tends to crash on these FBXs).

**Critical gotchas (learned the hard way):**
- **Armature object must be named `root`** before export, or UE rejects with
  "reimport to add joint."
- **`ARMATURE_AUTO` (bone heat) fails** on clothed CC meshes (disconnected islands
  + no under-clothes geometry). Use the scripted anatomical skinning, not auto-weights.
- **Unparent from FBX import Empties + apply scale** first — they carry 0.01 scale
  that silently zeroes weights.
- **Child meshes (Kid1/Kid2) must be scaled UP to adult height** (head-top → ~1.80 m,
  pivot at feet) **before** skinning, or head verts bind to spine/neck. They then
  render adult-height in UE — which is why kids use `RenderScale=0.85` at runtime
  (we compensate at render time instead of re-baking shorter).

---

## 6. Known gotchas (load-bearing)

- **VAT material is LAYERED.** `M_CrowdBaker_Character` uses the `ML_BoneAnimation`
  layer; all VAT params (NumFrames, NumBones, frame routing) are **layer-scoped**.
  Wire MIs with `update_material_instance_from_data_asset(da, mi, LAYER_PARAMETER)`.
  Using `GLOBAL_PARAMETER` → the layer ignores it → frozen/broken animation.
- **`NumFrames` mismatch breaks playback, not just looks.** The shader maps
  `Frame→V` as `Frame/NumFrames`; a stale value samples wrong texture rows.
- **Two different `SK_Mannequin` assets exist** with the same name: the project's
  (`/Game/CasualPackV1/Demo/Characters/Mannequins/Meshes/SK_Mannequin`, has our
  rebinds + Casual Pack anims) vs the AnimToTexture plugin's bundled one. CrowdBaker
  rejects an anim whose skeleton ≠ the mesh's. Use **project-native** anims.
- **ISM needs ≥2 per-instance custom-data floats** (Frame, PrevFrame). Motion blur
  is intentionally disabled (`PrevFrame = Frame`).
- **Mass processors touching Actor/world APIs need `bRequiresGameThreadExecution=true`**
  (e.g. `PushAnimToISMProcessor`).
- **World Partition `is_spatially_loaded` flips re-home the actor's external package** —
  only flip on a real saved level, then `save_current_level()`, or you lose the actor.
- **`mass.PrintEntityFragments` lies about POD fragments** — use `mass.LogArchetypes`
  to verify real archetype composition.

---

## 7. Current state & open items

**Working:** 6-character crowd on `test3`, per-instance VAT walk, soft mutual
avoidance, facing, held-frame idle, kid down-scaling (0.85).

**Open / known limitations:**
- **Breathing idle UNSOLVED.** MM_Idle crumples in this VAT path; idle is a frozen
  walk-frame. Cracking it needs real visual VAT debugging.
- **Separation is soft & O(N²).** Fine at ~100 agents; needs a spatial hash to scale
  to thousands. No hard collision — agents can briefly overlap.
- **Kid personal-space** uses the adult 120 cm radius (not scaled with `RenderScale`).
- **Scratch assets** in the tree: `/Game/ATL/MM_Idle_Lossless`, `Test_MannyMI_Man1Tex`
  (deletable).

---

## 8. Tooling, repo hygiene, MCP (optional)

- **Large files / LFS.** `CrowdBaker/.../VertexAnimation/TX_VertexNormal.uasset`
  (52 MB) exceeds GitHub's 50 MB soft limit. It's unused demo content (we bake
  bone-mode) — prune the `VertexAnimation/` folder, or adopt Git LFS for `*.uasset`.
- **`.gitignore`** excludes engine + plugin `Binaries/Intermediate`, `__pycache__`,
  and CasualPack characters except `Girl_001`.
- **MCP (dev convenience, not required).** Claude Code drove the editor and Blender
  via MCP. For Windows DCC tools launched from WSL, the MCP subprocess must run as a
  **Windows `.exe`** (WSL loopback ≠ Windows loopback). Blender connector binds
  `127.0.0.1:9876`; control lives in *Preferences → Add-ons → Blender MCP* (no
  sidebar tab). Restart Claude Code after editing `~/.claude.json`.

---

*Knowledge in this doc was distilled from working notes; verify out-of-repo paths
(`retarget.blend`, CasualPack availability) with the previous owner before relying
on them.*
