# CrowdSim — Claude Code Working Agreement

## Project
Unreal Engine 5.6 C++ project. Goal: background character crowds using Mass Entity +
MassCrowd, rendered via a LOD pipeline (SkeletalMesh near → animated-ISM at distance
via AnimToTexture). No Niagara — MassCrowd representation handles all LODs.
Windows dev host. Rider is the primary IDE; Claude Code handles agentic edits and builds.

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

## Module dependencies
In `MyProject.Build.cs` PublicDependencyModuleNames:
MassEntity, MassCommon, MassMovement, MassNavigation, MassSpawner,
MassRepresentation, MassCrowd, MassActors, ZoneGraph, StateTreeModule, AnimToTexture

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

## Out of scope (do not build)
- Mass <-> external data pipelines (that's a separate project, ArkLogPlayer).
- Custom navigation beyond ZoneGraph.
- Networked/replicated crowds.
- UI beyond basic debug overlays.

## Communication
Terse. Assumption-light. Flag weak assumptions, missing info, and risks explicitly
before recommending. No hedging filler. If a request is ambiguous, ask.
