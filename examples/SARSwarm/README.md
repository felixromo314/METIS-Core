# SARSwarm — Search & Rescue drone swarm with Metis-Core + raylib

Proof of concept: 4 UAVs take off from a mothership and search for a castaway drifting with the sea current.
The policy is a **multi-head DQN trained with Metis-Core: one head = one drone**.
The drones cooperate by sharing, over a radio mesh network, the areas they have already scanned, and they are
evaluated against a classic **lawnmower** (parallel track) search simulated in the same environment.

## Build
Same layout as the other Metis-Core examples (e.g. `SwarmDefenseTIE`, `DQN_Pong`):

```
examples/
  Libs/Metis-Core/includes, Debug/, ReleaseCUDA/   <- Metis-Core headers, .lib and .dll
  Libs/raylib/include, lib/                         <- raylib 5.5 (MSVC x64, DLL)
  Debug/, ReleaseCUDA/                              <- output folder of every example (+ LibTorch/CUDA DLLs)
  SARSwarm/SARSwarm.sln
```

1. Open `SARSwarm.sln` with Visual Studio 2022 (toolset v143, x64).
2. Configuration `Debug|x64` (CPU) or `ReleaseCUDA|x64` (GPU).
3. Pre-build copies `Libs/Metis-Core/<Config>/*` to `examples/<Config>/`; post-build copies `raylib.dll` and the
   logo. The executable is written to `examples/<Config>/` and runs from there, so `sarSwarm.ai`,
   `trainingLogSAR.txt` and `lawnmower_baseline.txt` are saved in that folder.
4. The LibTorch/CUDA DLLs needed by `METIS-Core.dll` must be in `examples/<Config>/` (see `INSTALL.md`).

## Usage
| Menu | |
|---|---|
| `1` TRAIN | `MultiHeadsAgentTrainerDQN` from scratch. Every 50 episodes, if the success count improves, `sarSwarm.ai` is saved and `trainingLogSAR.txt` is written. `ESC` stops and saves `sarSwarm_last.ai`. |
| `2` PLAY | Loads `sarSwarm.ai`; the network decides the heading of every drone. **Arrow keys move the castaway.** |
| `3` LOAD MODEL | Reads `sarSwarm.ai`. |
| `4` CONTINUE TRAINING | Loads `sarSwarm_last.ai` (or `sarSwarm.ai`) and keeps training. |

The side panel shows a **CONTROLS** table where the active option of every key is highlighted:

| Key | Mode | Effect |
|---|---|---|
| `0` / `1`-`4` | Train, Play | Heat map shown: union of all drones, or what a single drone knows |
| `C` | Train, Play | Radio range circles |
| `N` | Train, Play | Show / hide the castaway (ground truth) |
| `V` | Train | Draw every step / every 10 / every 25 steps / turbo (every 500) |
| `H` | Play | AI ↔ greedy heuristic (baseline) |
| `SPACE`, `+`/`-`, `R` | Play | Pause, simulation speed, restart |
| `ESC` | Train / Play | Stop training and save / back to menu |

On the first start the lawnmower baseline is simulated once (100 episodes, progress bar) and cached in
`lawnmower_baseline.txt`. It is recomputed automatically when a scenario parameter changes.

## Scenario (meters / seconds) — `src/SimConfig.h`
| Parameter | Value |
|---|---|
| Search area | 4 × 3 km, 50 m cells (80×60) |
| Step | 1 simulated second; castaway survival 1800 s (30 min) |
| Drone | 20 m/s, 30°/s turn rate, staggered take-off every 8 s |
| EO/IR sensor | 150 m radius, detection probability 0.85 per step |
| Drone-to-drone radio | 1200 m; >80 % of the range = **yellow** blinking link; no link = red warning |
| T mark (heat map) | a scanned cell gets heat 1.0 and cools down 1/600 per step → black after 10 min |
| Castaway | own motion: Ornstein-Uhlenbeck (θ=0.01, max 2 m/s; 4 m/s when pushed by the user) |
| Sea current | random every episode: uniform direction, 0.2–0.6 m/s; drawn with animated arrows and a compass rose |

### Cooperation
Every drone keeps **its own heat map**. In each step: all maps cool down → every drone marks its footprint
(T=1) on its own map → the radio graph and its islands are computed (BFS, multi-hop) → inside each island the maps
are merged (maximum). A drone that loses the link stops receiving the map and the positions of the others.

### Actions (per head)
8 headings (E, SE, S, SW, W, NW, N, NE). `isValidAction` masks the headings that would take the drone out of the
world within 150 m.

### Observation (151 inputs)
One block of 37 values per drone (head *i* learns to use block *i*) + 3 global values:

| In the block | Count | Content |
|---|---|---|
| 0–1 | 2 | Position x/4000, y/3000 |
| 2–3 | 2 | Own heading as cos, sin |
| 4–19 | 16 | "Coldness" (1 − heat) of **its own map** in 8 sectors × 2 rings (0–600 m, 600–1800 m), aligned with the 8 actions |
| 20–34 | 15 | Each of the 3 teammates: dx, dy, cos(heading), sin(heading), connected — only if it is in the same radio island, otherwise all 0 |
| 35 | 1 | Isolated (no direct link) |
| 36 | 1 | Time outside the full network (normalized, saturates at 2 min) |

Globals: episode time (step/1800) and sea current (vx, vy)/0.6. Angles are encoded as cos/sin to avoid the
0°/360° discontinuity. The `TSARSTATE` struct (620 bytes) stores the already computed observation, and
`serizalizeState` only copies it.

### Reward per head (per step)
- **Useful exploration**: Σ(1 − heat) of the scanned cells, measured on the **global team map**, divided by 3 and
  multiplied by the fraction of the team that receives it (a drone alone contributes 0). Re-scanning what a
  teammate just scanned pays nothing.
- +0.25 × mean useful exploration of the teammates; the positive part is normalized to [0, 1].
- **Re-scan penalty**: −0.5 × Σ heat of the cells that *enter* the sensor / 3 (white = strong, red = medium, almost black = very light).
- Sensor footprint overlap with a connected teammate: up to −0.3.
- **Communications**: outside the full network the penalty grows with time:
  severity × (−0.2 → −1.0 in 120 s), with severity 1 alone, 2/3 in a pair, 1/3 in a trio. Connected but in the
  yellow zone: up to −0.2. Any split of the swarm earns strictly less than staying together.
- Time −0.01, hitting the border −0.3.
- Terminal: found +5 (the finder) / +3 (the rest); castaway dies −3 for everybody.

## Evaluation against the lawnmower baseline
`LawnmowerBaseline` simulates a classic parallel-track search with its own environment instance: each drone owns a
1 km strip covered in 4 tracks 250 m apart. The panel shows two per-episode charts (dots per episode, yellow 20-episode
moving average) against the lawnmower (white dashed line = mean, band = 25–75 percentile):
- **Redundancy %**: share of the cells entering a sensor that the team had scanned less than 5 min before.
- **Time to contact**: seconds until detection (a failure counts as 1800 s).

Reference values (300 episodes, same scenarios, measured headless):

| Policy | Success | Mean time | Median | Redundancy |
|---|---|---|---|---|
| Random (≈ untrained network) | 66 % | 1178 s | 1281 s | 65.5 % |
| Greedy heuristic (per-drone, no coordination) | 96 % | 381 s | 239 s | 29.3 % |
| Lawnmower | 99 % | 429 s | 357 s | 23.4 % |

Note: the lawnmower keeps the full radio network only ~17 % of the time (its strips break the 1.2 km mesh), so only
the mission metrics are compared, not the rewards.

## Notes on the Metis-Core API
This project uses the current headers (v0.4.x): `addHead(name, agent, maxActionsPerHead)`,
`setNumberInputsOutputs(numInputs)`, `MultiHeadAgent::loadIAModel(name, bGPU)`. The castaway is passed to
`training(env, brain, castaway)` as the sparring agent; its `getActionProcedural` returns `CASTAWAY_DRIFT` and
`applyAction` ignores it (the drift happens in its `update`).

Changing the number of inputs (`NET_INPUTS`) invalidates every saved `.ai` model.
