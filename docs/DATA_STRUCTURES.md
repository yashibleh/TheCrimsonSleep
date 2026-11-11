# Data structures → gameplay

All nine required structures are implemented in C++17 (`cpp/include`, `cpp/src`) and compiled into
`assets/game.wasm`. None of them is a demonstration: each one is on the hot path of a system the
player interacts with, and each is covered by assertions in `cpp/tests/tests.cpp`.

| # | Structure | C++ type | Where it decides something |
| - | --------- | -------- | -------------------------- |
| 1 | Graph (adjacency list) | `WorldGraph` | Room topology; locked doors change which edges are traversable |
| 2 | BFS | `BFSExplorer` | The Weaver's room-to-room pursuit and shortest-route queries |
| 3 | DFS | `DFSExplorer` | Which rooms are reachable right now; gates pursuit behind locked doors |
| 4 | Stack | `PuzzleActionStack` | Undo/rollback of code digits, fuses, valves, drawers and candles |
| 5 | Queue (FIFO) | `SoundEventQueue` | Noise events consumed in order by enemy hearing |
| 6 | Priority queue | `EnemyDecisionSystem` | Binary heap of weighted actions; the best action is popped each think tick |
| 7 | Linked list | `PlayerInventory` | The satchel: insertion-ordered nodes, capacity, quantity, removal |
| 8 | BST | `ClueBST` | The journal: clues keyed by id, in-order listing |
| 9 | Arrays | `RoomData`, `PlayerState`, `Enemy`, flat export blocks | Tile/decor/nav grids, stats, enemy array, lights/props/items, animation frames |

---

## 1. Graph — `WorldGraph`

`RoomNode nodes[10]` plus one contiguous `GraphEdge edges[]` block, built once from authors' door data
and finalised into per-node adjacency ranges (`finalize()`), so every neighbour lookup is
`edges[node.edgeStart .. +edgeCount]` rather than a pointer chase.

- Each edge carries the `doorId` that gates it. `edgePassable()` consults the live `DoorTable`, so
  unlocking the boiler door, restoring power, solving the mortuary lock or turning the valves changes
  the graph's traversability immediately — no rebuild of the topology is needed.
- `findEdge(a, b)` is what the pursuit tests assert against: a BFS hop must correspond to a real edge.
- The ten rooms and eighteen doors are declared in `cpp/src/levels.cpp`; `graph.finalize()` is called
  once the authoring pass is complete.

Complexity: O(V+E) space, O(deg) neighbour iteration.

## 2. BFS — `BFSExplorer`

`explore()` is a queue-based flood that only follows passable edges, recording visit order, distance
and parent for every room. `path()` reconstructs a route from those parents.

Gameplay use: `GameWorldManager::nextRoomToward(from, to)` runs a BFS from the enemy's room and walks
the parent chain back from the target to find the **next hop**. `EnemyController::updateAll()` uses
that hop whenever the **Weaver** is in another room and hunting: every 2.4 s it steps to the next room
on the shortest open-door route and enters at the door that faces the room it came from. Lock a door
behind you and the route is recomputed without that edge — if no route remains, the Weaver stops
advancing.

Also used by `nearestUnvisited`, `reachableCount` and `escapeRooms` (available to the UI layer through
the `tcs_debug_*` exports).

Complexity: O(V+E) per flood; the room count is ten, so this runs without measurable cost.

## 3. DFS — `DFSExplorer`

`explore()` performs a recursive-style depth-first flood (explicit stack), reporting visit order,
per-room depth, the number of connected regions and junction density.

Gameplay use: `GameWorldManager::refreshReachability()` runs a DFS from the player's room whenever a
room loads or a door opens, filling `reachMask[]` and `reachCount`. That mask gates the Weaver's
graph pursuit: it will only path into rooms that are actually connected to the player through doors
that are currently open. Sealing the ward (all doors locked) leaves `reachCount == 1` and the Weaver
cannot follow — the test suite asserts exactly this.

`countRegions()` and `deepestRoom()` are exposed for the map/journal layer.

Complexity: O(V+E).

## 4. Stack — `PuzzleActionStack`

A fixed-capacity LIFO of `PuzzleAction { kind, target, value, prevValue, x, y, tick }`.

- Every puzzle mutation pushes an entry: `ACTK_CODE_DIGIT`, `ACTK_FUSE`, `ACTK_VALVE`,
  `ACTK_CANDLE`, `ACTK_DRAWER`.
- `puzzleUndo()` pops the last action and restores `prevValue` — the code lock drops its last digit,
  the last fuse comes back out, the last valve returns to its previous position, the candle goes out.
  Drawer searches are deliberately not undoable (a searched drawer stays searched).
- Undo is reachable from the pause screen, so a mis-ordered valve chain is recoverable in play.

Complexity: O(1) push/pop.

## 5. Queue — `SoundEventQueue`

A ring buffer of `SoundEvent { x, y, intensity, kind, clock }` with a `read` bitmap per listener.

- Footsteps, sprinting, gunshots, door slams, breaker throws and scripted stings call
  `pushNoise(x, y, intensity, kind)`.
- `EnemyController::updateAll()` drains the queue **in FIFO order**: each enemy examines the head
  event, converts it to a noise target if it is within its hearing radius, then marks it read. The
  queue is popped only when every listener has read the head, so one gunshot is processed by every
  enemy exactly once and in the order it was made.
- The blind **Crawler** has the widest hearing radius and depends on this path entirely; the Hound
  leans on sight, and the Weaver ignores noise but uses the graph.

Complexity: O(1) amortised push/pop, O(listeners) per frame.

## 6. Priority queue — `EnemyDecisionSystem`

A binary max-heap of `EnemyDecision { action, weight, targetX, targetY }`.

Every ~0.22 s each living enemy submits candidate actions with weights derived from the situation
(distance, line of sight, alert level, health, noise power, home distance), and the heap returns the
highest-weighted one: `ACT_ATTACK`, `ACT_CHASE`, `ACT_INVESTIGATE`, `ACT_SEARCH`, `ACT_RETREAT`,
`ACT_RETURN`, `ACT_PATROL`, `ACT_IDLE`. The chosen action is mapped to one of the eight enemy states.

Because weights compete, behaviour shifts smoothly: a wounded enemy's retreat can outrank a chase, a
loud noise can outrank patrol, and a visible player always outranks investigation.

Complexity: O(log n) push/pop with n ≤ 8.

## 7. Linked list — `PlayerInventory`

Singly linked `InventoryNode { itemId, qty, next }` with `insert`, `find`, `remove`, `quantity`, `at`,
`size`, `isFull` (capacity 12 slots, stackable quantities).

- Insertion order is the display order in the satchel, so pickups appear where the player expects.
- `find`/`quantity` gate doors (`keyItem`), the breaker fuses, the valve handle, the cradle ending
  (locket + sedative), healing and reloading.
- The list is traversed by `api.cpp` to publish item ids/quantities into flat export arrays for the UI.

Complexity: O(n) search with n ≤ 12; O(1) insert at head.

## 8. BST — `ClueBST`

`ClueNode { clueId, roomId, left, right }` ordered by clue id.

- Reading a note, examining the organ, the wall writing, the case notes or the toe tag inserts a clue;
  duplicate pickups return false so the journal does not double-list.
- In-order traversal produces the journal's clue list (ordered by clue id, i.e. story order), and
  `search()` answers "do I already know this?" for the map/bestiary tabs.
- `height()` is asserted in tests to confirm the tree stays balanced enough for ten-plus clues.

Complexity: O(log n) average insert/search, O(n) traversal.

## 9. Arrays — tile grids, stats, entities

The simulation is array-based and cache-friendly, which is also what makes the flat memory export to
JavaScript trivial:

- `RoomData.tiles[40*24]`, `decor[]` and `navBlocked[]` — the collision/navigation truth. `navBlocked`
  is rebuilt from tiles + solid props + locked doors (`rebuildNav()`), and `NavSystem` uses it for
  circle collision, line-of-sight raycasts and enemy pathfinding.
- `PlayerState.stats[STAT_COUNT]` — health, stamina, battery, ammo, invulnerability, damage flash.
- `Enemy list[MAX_ENEMIES]` — fixed pool, exported as a 20-float stride block.
- `RoomLight lights[]`, `RoomProp props[]`, `RoomItem items[]`, `RoomDoor doors[]` — struct-of-arrays
  style blocks for the renderer.
- Animation frames are index tables in `js/render.js` driven by the C++ `anim`/`animPhase` counters.

`api.cpp` maps these arrays to `Float32Array`/`Uint8Array` views in `js/core.js`, so the renderer reads
the same bytes the simulation wrote.

---

## Supporting systems

- **NavigationSystem** — grid collision, circle-vs-tile resolution, line-of-sight raycasts and
  per-enemy A* path caching over `navBlocked`.
- **Save/load** — a binary blob (`TCS1` magic, trailing `0x54435345`) covering player, inventory,
  clues, puzzle progress, doors, visited rooms, enemies and story flags; loaded through
  `tcs_deserialize` from `localStorage`.
- **No STL, no libc** — `libc_shim.cpp` provides the few `mem*`/`str*`/`sqrtf` symbols required, which
  keeps the module small enough to load instantly and avoids dragging a runtime into WebAssembly.
