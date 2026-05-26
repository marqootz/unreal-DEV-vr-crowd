# CrowdSim — Claude Code Working Agreement

## Project
Unreal Engine 5.6 C++ project. Goal: background character crowds using Mass Entity +
MassCrowd, rendered via a LOD pipeline (SkeletalMesh near → animated-ISM at distance
via AnimToTexture). No Niagara — MassCrowd representation handles all LODs.
Windows dev host. Rider is the primary IDE; Claude Code handles agentic edits and builds.

Active branch: `crowd-atl-pivot`. **`HANDOFF.md`** (repo root) is the full onboarding /
porting guide and out-of-repo dependency checklist — read it for anything not covered here.

## Engine & tooling
- UE 5.6 (locked — do not bump without asking)
- Engine path: `C:\Program Files\Epic Games\UE_5.6`
- Project file: `MyProject/MyProject.uproject`
- Primary module: `MyProject/Source/MyProject/`
- Build target: `MyProjectEditor`
- Build command (run from repo root via cmd.exe; wrapper `run_build.bat` already exists):
  `"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" MyProjectEditor Win64 Development -project="C:\LOCAL_DEV\UNREAL\crowd_test_cpp\MyProject\MyProject.uproject" -waitmutex`
- From WSL, invoke via `cmd.exe /c "C:\LOCAL_DEV\UNREAL\crowd_test_cpp\run_build.bat"` — direct quoted paths through `cmd.exe /c` from bash get mangled.
- Regenerate project files (only needed after adding/removing .cpp/.h): right-click the .uproject in Explorer, or run UBT with `-projectfiles`. Installed-engine builds don't ship `GenerateProjectFiles.bat`.

Always run the build after non-trivial C++ changes. Read the error output, fix, repeat.
Do not declare a task done until the build is clean.

## Enabled plugins (authoritative list — edit .uproject to match)
MassGameplay, MassCrowd, MassAI, ZoneGraph, ZoneGraphAnnotations, StateTree, AnimToTexture
(`ModelingToolsEditorMode` is the only editor-only extra.)

Note: `MassEntity` is NOT a plugin in UE 5.5+ — it was folded into engine core. Do not
add it back to `.uproject` Plugins; it resolves via the `MassEntity` module dep below.

`CrowdBaker` (our fork of AnimToTexture) is a **project-local plugin** at
`MyProject/Plugins/CrowdBaker/` and is intentionally NOT in `.uproject` — it auto-enables
as a default-enabled project plugin and is pulled in via the `CrowdBakerRuntime` module dep.
Modules: `CrowdBakerRuntime` (PreDefault) + `CrowdBakerEditor` (editor, bake tooling).

## Module dependencies
In `MyProject.Build.cs` PublicDependencyModuleNames:
MassEntity, MassCommon, MassMovement, MassNavigation, MassSpawner,
MassRepresentation, MassCrowd, MassActors, MassLOD, ZoneGraph, StateTreeModule,
AnimToTexture, CrowdBakerRuntime

## Crowd implementation (current state)
- **Crowd level: `/Game/test3`** — PIE there ("Untitled" was a transient, never-saved
  world; don't use it). A `MassSpawner` spawns count=100 from 6 `MassEntityConfig_<Char>`
  (Man1, Man2, Girl1, Girl2, Kid1, Kid2).
- **Tick pipeline** (Movement group): `WandererProcessor` (corridor locomotion + state
  machine) → `CrowdSeparationProcessor` (soft boid repulsion, XY, O(N²), radius 120cm) →
  `FacingProcessor` (yaw→velocity) → `CrowdAnimProcessor` (advances anim time + walk-blend;
  writes per-char `RenderScale` into the transform). Engine Representation then pushes the
  transform to the ISM; `PushAnimToISMProcessor` runs **after Representation** (game-thread)
  to write `FCrowdBakerFrameData{Frame,PrevFrame}` (2 floats) to ISM per-instance custom data.
  All processors self-register (`bAutoRegisterWithProcessingPhases`).
- **Per-char config traits:** `CrowdAnimTrait` (DataAsset + anim indices + `RenderScale`),
  `WandererTrait` (locomotion + carries `FCrowdSeparationParams`), `MassCrowdVisualizationTrait`
  (mesh → `SM_<Char>_BoneAnimation`, `material_overrides=[None]` so the SM's own VAT section
  materials are used).
- **Baking a character:** `MyProject/Scripts/bake_atl_character.py` (run in-editor, Python
  plugin). Outputs to `/Game/ATL/<Char>/`. Always bake **16-bit precision**.
- **Kids render adult-height** (baked on the adult skeleton) and are down-scaled at runtime
  via `FCrowdAnimParams.RenderScale = 0.85`, applied to the ISM transform (mesh origin at
  feet → shrinks downward, stays grounded).
- **Idle is a held walk-frame, NOT MM_Idle** — MM_Idle "crumples to a ball" in this VAT path
  (root cause undiagnosed). Don't re-bake MM_Idle expecting it to work.

## Code conventions
- C++ for all gameplay logic, parsing, data model work.
- Blueprints only for: actor composition, visual/material config, editor-only tools,
  designer-facing knobs. Never for runtime simulation logic.
- Header + source pairs. No single-file header-only patterns.
- Prefer `TObjectPtr<T>` over raw `T*` for UPROPERTY members (5.x standard).
- Mass fragments: POD structs, no UObject references, inherit `FMassFragment`.
- Mass processors: explicit `ConfigureQueries`, avoid touching the world directly —
  go through fragments and subsystems.
- Use `UE_LOG(LogCrowdSim, ...)` — declare `LogCrowdSim` in the module .cpp.
- No `using namespace` in headers. No STL containers in UPROPERTY types.

## File layout
- `MyProject/Source/MyProject/` — game module: Wanderer/Separation/Facing/CrowdAnim/
  PushAnimToISM/PlayerAvoidance processors + their traits/fragments.
- `MyProject/Plugins/CrowdBaker/` — VAT bake plugin (fork of AnimToTexture).
- `MyProject/Scripts/` — in-editor Python (`bake_atl_character.py`, `retarget_anims.py`).
- `MyProject/Content/ATL/<Char>/` — baked VAT outputs (SM, T_*Pos/Rot/Weight, DA, MI).
- `MyProject/Content/MassEntityConfig_<Char>.uasset` — per-character spawn configs.

## Working style
- Minimal diffs. Do not rewrite a file to change five lines.
- When adding a new class, create both .h and .cpp, then regenerate project files.
- Ask before: adding a new plugin, changing engine version, adding a third-party dep,
  modifying .uproject beyond the declared plugin list.
- Do not add Blueprint-callable functions unless asked — keep the C++ surface lean.
- Do not invent Mass APIs. If unsure whether a fragment/processor API exists in 5.6,
  say so and check `Engine/Plugins/Runtime/MassEntity/` or the CitySample reference.
- No speculative abstractions. No "future-proofing" layers. Build what's needed now.

## Known friction points
- MassCrowd docs are thin. CitySample is the reference implementation — consult it.
- Mass API has shifted across 5.2/5.3/5.4/5.5/5.6. Treat older web examples as suspect.
- UHT errors are often misleading. If a UCLASS/UPROPERTY error looks nonsensical,
  the real issue is usually a missing include, missing module dep, or malformed macro.
- **VAT material `M_CrowdBaker_Character` is LAYERED** (`ML_BoneAnimation`). All VAT params
  are layer-scoped — wire MIs with `update_material_instance_from_data_asset(da, mi,
  LAYER_PARAMETER)`. `GLOBAL_PARAMETER` is silently ignored → frozen/broken animation.
- **`NumFrames` mismatch breaks playback, not just looks** — the shader maps `Frame→V` as
  `Frame/NumFrames`; a stale value samples wrong texture rows. Re-sync MI scalars after a re-bake.
- **Two `SK_Mannequin` assets share the name**: project's (`/Game/CasualPackV1/Demo/.../
  SK_Mannequin`, has our rebinds + Casual Pack anims) vs the AnimToTexture plugin's bundled
  one. CrowdBaker rejects an anim whose skeleton ≠ the mesh's — use project-native anims.
- **`mass.PrintEntityFragments` lies about POD fragments** — use `mass.LogArchetypes`.

## Out of scope (do not build)
- Mass <-> external data pipelines (that's a separate project, ArkLogPlayer).
- Custom navigation beyond ZoneGraph.
- Networked/replicated crowds.
- UI beyond basic debug overlays.

## Communication
Terse. Assumption-light. Flag weak assumptions, missing info, and risks explicitly
before recommending. No hedging filler. If a request is ambiguous, ask.
