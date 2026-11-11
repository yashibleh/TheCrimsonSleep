# Test results

Run: 11 November 2026, in the project worktree, on the final source.

## 1. Core build

```
$ ZIG=$HOME/toolchain/zig-linux-x86_64-0.13.0/zig ./build.sh
built assets/game.wasm (364675 bytes, 95 exports)
BUILD_EXIT=0
```

## 2. Native C++ core tests — 238 passed, 0 failed

```
$ g++ -std=c++17 -O1 -Icpp/include cpp/src/*.cpp cpp/tests/tests.cpp -o build/tests && ./build/tests
...
PASSED: 238   FAILED: 0
TEST_EXIT=0
```

`cpp/tests/tests.cpp` exercises the real simulation, not mocks:

| Section | What is asserted |
| --- | --- |
| Graph + explorers | nodes/edges exist, adjacency degrees, BFS routes, DFS regions, locked tournaments |
| Graph reachability & pursuit | `reachCount == 10` with doors open, sealing doors leaves `reachCount == 1`, no BFS route through locked doors, the Weaver closes BFS distance each hop |
| Sound queue & decisions | FIFO order preserved, every listener reads each event once, heap returns the highest-weighted action |
| Inventory & journal | insert/stack/find/remove/quantity, capacity limit, BST insert/search/in-order/height |
| Navigation | grid collision, circle blocking, line of sight through walls |
| Player movement & items | sprint stamina, torch battery, pickups, weapon swap, reload, healing |
| Doors & graph updates | key/valve/power/code requirements, unlocking changes traversability |
| Puzzles | code `0317`, valve order, three fuses, candle order, action-stack undo of each |
| Combat & enemy AI | all eight states reachable, damage/kill/stagger, Weaver is unkillable but staggered, chase and search transitions |
| Save/load & endings | binary round-trip (`TCS1`), both endings (id 1 waking, id 2 sleep) |
| Level integrity | 10 rooms, 18 doors, ≥12 notes, 27 lights, 22 items, walkable spawn points and doors |

## 3. Integration tests — 28 passed, 0 failed

`tools/web_test.mjs` boots the **real site** in jsdom with a real canvas backend, runs the actual
`js/main.js` loop against the actual `assets/game.wasm`, drives it with real keyboard events and DOM
clicks, and writes screenshots of the rendered frames to `artifacts/`.

```
$ node tools/web_test.mjs
WEB TESTS: 28/28 passed
WEB_EXIT=0
```

Covered: core loads and exposes its API · boot screen first · logo asset present · any key → menu ·
Continue disabled with no save · New Game starts in Ward 3 and writes a checkpoint · player moves with
the keyboard · footsteps push noise · HUD health renders · sprint drains stamina · satchel opens and
Escape closes it · journal + map tabs open · interaction prompt appears near an item · the bandage
enters the **C++** inventory · corridor loads with a living enemy · keypad digits reach the C++ code
lock and solve it · damage lowers HUD health · death screen appears · restart from the last candle
restores the checkpoint (health back to 100) · the cradle exists · interacting with it reaches an
ending · no uncaught runtime errors.

## 4. In-browser verification (real Chrome)

The site was served on `0.0.0.0:3000` and driven through the cloud browser:

| Check | Result |
| --- | --- |
| Boot screen renders the generated `assets/logo.png` title card, legible | pass |
| Boot → menu via key press; menu lists New Game / Continue / Settings / Credits / Quit, Continue disabled | pass |
| New Game loads Ward 3 with HUD, objective, narration and flashlight cone | pass |
| No console or network errors during play | pass |
| Screenshots | `artifacts/live-01-boot.png`, `live-02-menu.png`, `live-03-ward.png` |

Frames from the headless integration run (`artifacts/shot-01` … `shot-15`) cover boot, menu, ward,
satchel (empty and filled), journal, map, corridor, boiler, chapel, morgue, sleep chamber, keypad,
death and ending.

## 5. Defects found during verification and fixed

1. **Logo rasterised with no fonts** — the title drew as placeholder boxes. Fixed by registering real
   Cinzel/EB Garamond TTFs in `tools/logo.mjs` and regenerating `assets/logo.png`.
2. **Boot footer overlapped "PRESS ANY KEY"** — the footer sat inside an animated (transformed)
   container, so `position: fixed` anchored it to the logo block. Moved it to the screen level.
3. **Narration printed twice** — C++ set the subtitle *and* emitted the same line as a quest toast.
   The event handler now drops the duplicate when the subtitle channel already owns the line.
4. **Objective line collided with the notification stack** — moved the toasts below the objective row.
5. **Large props beyond 44 px were not interactable** — the prompt search clamped to a fixed
   44-px radius, so the cradle, breaker and organ could not be used from their visible edge. Prompt
   search now uses per-item (44 px) and per-prop (authored reach) bounds.
6. **The Weaver teleported through locked doors** — it was placed into the player's room every frame,
   bypassing the world graph. It now walks the graph: BFS picks the next room on the shortest
   open-door route, and DFS reachability (`reachMask`) gates whether it may pursue at all. Locking a
   door behind you now actually breaks the pursuit.

## 6. Limitations / not verified

- Audio is synthesised by `js/audio.js`; its correctness was checked by inspection and by the absence
  of runtime errors, **not** by listening — no audio device is available in this environment.
- The renderer was verified from captured frames (both headless and in-browser), not on a physical
  monitor; colour and contrast judgements are the author's.
- No mobile/touch testing; the game is keyboard-only by design.
- The integration harness needs jsdom and a canvas backend installed outside the project
  (`npm i jsdom @napi-rs/canvas` in a scratch directory); the shipped game itself has zero
  dependencies.
