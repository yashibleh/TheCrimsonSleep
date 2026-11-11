// Native test suite for THE CRIMSON SLEEP core.
// Build: g++ -std=c++17 -O1 -Icpp/include cpp/src/*.cpp cpp/tests/tests.cpp -o build/tests
#include <cstdio>
#include <cmath>
#include "GameWorldManager.hpp"
#include "WorldGraph.hpp"
#include "BFSExplorer.hpp"
#include "DFSExplorer.hpp"
#include "SoundEventQueue.hpp"
#include "EnemyDecisionSystem.hpp"
#include "PlayerInventory.hpp"
#include "ClueBST.hpp"
#include "PuzzleActionStack.hpp"
#include "NavigationSystem.hpp"
#include "levels.hpp"
#include "Messages.hpp"

using namespace tcs;

static int g_pass = 0;
static int g_fail = 0;
static const char* g_section = "";

#define SECTION(n) do { g_section = n; printf("\n== %s ==\n", n); } while (0)
#define CHECK(cond, msg) do { \
  if (cond) { g_pass++; printf("  PASS  %s\n", msg); } \
  else { g_fail++; printf("  FAIL  %s (line %d)\n", msg, __LINE__); } \
} while (0)
#define CHECK_EQ(a, b, msg) do { \
  long long _a = (long long)(a), _b = (long long)(b); \
  if (_a == _b) { g_pass++; printf("  PASS  %s [%lld]\n", msg, _a); } \
  else { g_fail++; printf("  FAIL  %s (got %lld, want %lld, line %d)\n", msg, _a, _b, __LINE__); } \
} while (0)

static i32 findProp(i32 room, i32 type, i32 id) {
  for (i32 i = 0; i < g_world.rooms[room].propCount; ++i) {
    const RoomProp& p = g_world.rooms[room].props[i];
    if (p.type == type && p.id == id) return i;
  }
  return -1;
}

static i32 findItem(i32 room, i32 itemId) {
  for (i32 i = 0; i < g_world.rooms[room].itemCount; ++i)
    if (g_world.rooms[room].items[i].itemId == itemId && !g_world.rooms[room].items[i].taken) return i;
  return -1;
}

static void pressInteractOn(i32 promptId, i32 target) {
  g_world.prompt.id = promptId;
  g_world.prompt.labelId = promptId;
  g_world.prompt.target = target;
  g_world.interact();
}

static void openAllDoors() {
  for (i32 i = 0; i < MAX_ROOMS * MAX_ROOM_DOORS; ++i) g_world.doorTable.unlocked[i] = 1;
  g_world.rebuildNav();
}

static void testGraphAndExplorers() {
  SECTION("WorldGraph / BFS / DFS");
  g_world.init();
  CHECK_EQ(g_world.graph.nodeCount, ROOM_COUNT, "graph has one node per room");
  CHECK_EQ(g_world.graph.degree(R_WARD3), 1, "ward connects to exactly one neighbour");
  CHECK_EQ(g_world.graph.degree(R_CORRIDOR), 4, "corridor is the hub with four edges");

  BFSExplorer bfs;
  BFSResult r;
  bfs.explore(g_world.graph, g_world.doorTable, R_WARD3, r);
  i32 lockedReach = r.count;
  CHECK(lockedReach >= 4, "locked world still reaches the main wing");
  CHECK(r.dist[R_WARD3] == 0, "BFS distance to self is zero");

  openAllDoors();
  bfs.explore(g_world.graph, g_world.doorTable, R_WARD3, r);
  CHECK_EQ(r.count, ROOM_COUNT, "all ten rooms reachable once doors are open");
  CHECK(r.dist[R_SLEEP] > r.dist[R_CORRIDOR], "sleep chamber is far from the ward");

  i32 path[MAX_ROOMS];
  i32 len = bfs.path(g_world.graph, g_world.doorTable, R_WARD3, R_SLEEP, path, MAX_ROOMS);
  CHECK(len >= 6, "BFS path to the final chamber exists");
  CHECK_EQ(path[0], R_WARD3, "path starts at the ward");
  CHECK_EQ(path[len - 1], R_SLEEP, "path ends at the sleep chamber");

  i32 escape[MAX_ROOMS];
  len = bfs.escapeRooms(g_world.graph, g_world.doorTable, R_CORRIDOR, R_WARD3, escape, MAX_ROOMS);
  CHECK(len > 0, "escape rooms computed away from a threat");

  DFSExplorer dfs;
  DFSResult d;
  dfs.explore(g_world.graph, g_world.doorTable, R_WARD3, d);
  CHECK_EQ(d.count, ROOM_COUNT, "DFS visits every reachable room");
  CHECK_EQ(dfs.countRegions(g_world.graph, g_world.doorTable), 1, "world is one connected region");
  i32 deepest = dfs.deepestRoom(g_world.graph, g_world.doorTable, R_WARD3);
  CHECK(deepest >= 0 && deepest < ROOM_COUNT, "deepest room resolves");
  CHECK(d.branchRooms > 0, "DFS detects junction rooms");

  // locked doors restrict the graph again
  g_world.init();
  BFSResult r2;
  bfs.explore(g_world.graph, g_world.doorTable, R_WARD3, r2);
  CHECK(r2.dist[R_MORGUE] == -1, "morgue is unreachable behind the code lock");
  CHECK_EQ(g_world.graph.routeCost(g_world.doorTable, R_WARD3, R_MORGUE), -1, "graph routing honours the locked door");
}

static void testSoundQueueAndDecisions() {
  SECTION("SoundEventQueue (FIFO) / EnemyDecisionSystem (priority queue)");
  SoundEventQueue q;
  q.reset();
  CHECK(q.empty(), "queue starts empty");
  q.push(10, 10, 0.4f, 1);
  q.push(20, 20, 0.9f, 2);
  q.push(30, 30, 0.2f, 0);
  CHECK_EQ(q.size(), 3, "three events queued");
  CHECK_EQ((i32)q.front().x, 10, "front is the oldest event (FIFO)");
  q.markRead(0);
  CHECK(!q.unread(0), "listener 0 has consumed the front event");
  CHECK(q.unread(2), "listener 2 has not");
  q.pop();
  CHECK_EQ((i32)q.front().x, 20, "pop advances in insertion order");
  q.clock = 10.f;
  q.expire(5.f);
  CHECK_EQ(q.size(), 0, "stale events expire");

  EnemyDecisionSystem ds;
  ds.reset();
  ds.push(ACT_PATROL, 10.f, 0, 0);
  ds.push(ACT_CHASE, 90.f, 0, 0);
  ds.push(ACT_INVESTIGATE, 45.f, 0, 0);
  CHECK_EQ(ds.size(), 3, "three decisions ranked");
  CHECK_EQ(ds.top().action, ACT_CHASE, "highest score is on top");
  ds.updateScore(ACT_INVESTIGATE, 99.f);
  CHECK_EQ(ds.top().action, ACT_INVESTIGATE, "priority queue re-heapifies on score change");
  EnemyDecision first = ds.pop();
  CHECK_EQ(first.action, ACT_INVESTIGATE, "pop returns the top decision");
  CHECK_EQ(ds.top().action, ACT_CHASE, "remaining decisions stay ordered");
}

static void testInventoryAndJournal() {
  SECTION("PlayerInventory (linked list) / ClueBST");
  PlayerInventory inv;
  inv.reset();
  CHECK_EQ(inv.countItems(), 0, "inventory starts empty");
  CHECK_EQ(inv.insert(IT_AMMO, 6), 6, "ammo stack inserted");
  CHECK_EQ(inv.insert(IT_AMMO, 4), 4, "ammo stacks into the same node");
  CHECK_EQ(inv.quantity(IT_AMMO), 10, "stack quantity is 10");
  CHECK_EQ(inv.countItems(), 1, "one inventory node");
  inv.insert(IT_BANDAGE, 2);
  inv.insert(IT_REVOLVER, 1);
  CHECK_EQ(inv.countItems(), 3, "three inventory nodes");
  CHECK(inv.find(IT_REVOLVER) != 0, "linked list search finds the revolver");
  CHECK_EQ(inv.find(IT_AMMO)->quantity, 10, "node data accessible through the list");
  CHECK_EQ(inv.at(0)->itemId, IT_REVOLVER, "at(0) is the most recently inserted node");
  CHECK(inv.hasCategory(CAT_HEAL), "category scan finds healing items");
  inv.updateQuantity(IT_BANDAGE, -1);
  CHECK_EQ(inv.quantity(IT_BANDAGE), 1, "quantity update works");
  CHECK(inv.remove(IT_BANDAGE, 1), "remove consumes the last bandage");
  CHECK(inv.find(IT_BANDAGE) == 0, "bandage node unlinked when emptied");
  CHECK_EQ(inv.countItems(), 2, "two nodes remain after removal");
  const ItemDef* def = inv.table.find(IT_REVOLVER);
  CHECK(def && def->category == CAT_WEAPON, "item table resolves definitions");

  ClueBST bst;
  bst.reset();
  CHECK(bst.insert(12, R_CHAPEL), "insert clue 12");
  CHECK(bst.insert(4, R_NURSE), "insert clue 4");
  CHECK(bst.insert(9, R_MORGUE), "insert clue 9");
  CHECK(bst.insert(1, R_WARD3), "insert clue 1");
  CHECK(!bst.insert(9, R_MORGUE), "duplicate clue rejected");
  CHECK(bst.search(9) != 0, "search finds a clue");
  CHECK(bst.search(77) == 0, "search misses an unknown clue");
  CHECK_EQ(bst.foundCount(), 4, "four unique clues");
  i32 ids[MAX_CLUES];
  i32 n = bst.inOrder(ids, MAX_CLUES);
  CHECK_EQ(n, 4, "in-order traversal yields all clues");
  CHECK(ids[0] < ids[1] && ids[1] < ids[2] && ids[2] < ids[3], "in-order traversal is sorted");
  CHECK(bst.remove(4), "clue removal");
  CHECK(bst.search(4) == 0, "removed clue is gone");
  CHECK_EQ(bst.foundCount(), 3, "three clues after removal");
  CHECK(bst.height() >= 2 && bst.height() <= 3, "tree height stays sane");

  PuzzleActionStack st;
  st.reset();
  CHECK(st.empty(), "action stack empty");
  st.push(ACTK_VALVE, 2, 1, 0, 0, 0, 1.f);
  st.push(ACTK_VALVE, 0, 2, 1, 0, 0, 2.f);
  CHECK_EQ(st.size(), 2, "two actions pushed");
  CHECK_EQ(st.peek()->target, 0, "peek sees the newest action");
  PuzzleAction a;
  CHECK(st.pop(a), "pop succeeds");
  CHECK_EQ(a.prevValue, 1, "popped action carries rollback data");
  CHECK_EQ(st.size(), 1, "stack shrank");
}

static void testNavigation() {
  SECTION("NavigationSystem (tile BFS + line of sight)");
  NavSystem nav;
  nav.reset();
  u8 grid[CELLS];
  for (i32 i = 0; i < CELLS; ++i) grid[i] = 0;
  // wall column with a single gap
  for (i32 y = 0; y < ROOM_H; ++y) grid[y * ROOM_W + 20] = 1;
  for (i32 y = 10; y <= 12; ++y) grid[y * ROOM_W + 20] = 0;
  i32 path[CELLS];
  i32 len = nav.findCellPath(grid, 5 * TILE, 10 * TILE, 30 * TILE, 10 * TILE, path, CELLS);
  CHECK(len > 15, "BFS path routes through the gap");
  CHECK_EQ(path[0] % ROOM_W, 5, "path starts at the source cell");
  CHECK_EQ(path[len - 1] % ROOM_W, 30, "path ends at the target cell");
  bool usesGap = false;
  for (i32 i = 0; i < len; ++i) {
    if (path[i] % ROOM_W == 20 && path[i] / ROOM_W >= 10 && path[i] / ROOM_W <= 12) usesGap = true;
  }
  CHECK(usesGap, "path crosses the wall exactly at the gap");
  CHECK(!nav.losClear(grid, 5 * TILE, 5 * TILE, 30 * TILE, 5 * TILE), "LOS blocked by the wall");
  CHECK(nav.losClear(grid, 5 * TILE, 11 * TILE, 15 * TILE, 11 * TILE), "LOS clear in the open");

  for (i32 y = 0; y < ROOM_H; ++y) grid[y * ROOM_W + 20] = 1;
  CHECK_EQ(nav.findCellPath(grid, 5 * TILE, 5 * TILE, 30 * TILE, 5 * TILE, path, CELLS), 0,
           "sealed room returns no path");
}

static void testPlayerMovementAndItems() {
  SECTION("Player movement, collision, items, clues");
  g_world.init();
  CHECK_EQ(g_world.player.stats[STAT_HP], 100.f, "player starts at full health");
  CHECK_EQ(g_world.room, R_WARD3, "game starts in ward 3");

  f32 startX = g_world.player.x;
  for (i32 i = 0; i < 40; ++i) g_world.update(1.f / 60.f, IN_RIGHT);
  CHECK(g_world.player.x > startX + 20.f, "player accelerates and moves right");
  CHECK(g_world.player.stats[STAT_NOISE] > 0.f, "movement produces noise");

  // hold right into the east wall and confirm collision stops the player
  for (i32 i = 0; i < 600; ++i) g_world.update(1.f / 60.f, IN_RIGHT);
  CHECK(g_world.player.x < 38.f * TILE, "wall collision stops the player");

  // sprint drains stamina
  g_world.player.x = 10.f * TILE;
  g_world.player.y = 10.f * TILE;
  f32 staminaBefore = g_world.player.stats[STAT_STAMINA];
  for (i32 i = 0; i < 60; ++i) g_world.update(1.f / 60.f, IN_RIGHT | IN_SPRINT);
  CHECK(g_world.player.stats[STAT_STAMINA] < staminaBefore, "sprinting drains stamina");

  // flashlight toggle and battery drain
  g_world.player.setFlag(PF_FLASHLIGHT_ON, true);
  f32 battery = g_world.player.stats[STAT_BATTERY];
  for (i32 i = 0; i < 120; ++i) g_world.update(1.f / 60.f, 0);
  CHECK(g_world.player.stats[STAT_BATTERY] < battery, "flashlight drains the battery");

  // item pickup through the interaction handler
  g_world.init();
  i32 itemIdx = findItem(R_WARD3, IT_BANDAGE);
  CHECK(itemIdx >= 0, "bandage exists in the ward");
  pressInteractOn(PROMPT_TAKE, -1000 - itemIdx);
  CHECK(g_world.inv.find(IT_BANDAGE) != 0, "pickup inserts into the inventory linked list");
  CHECK(g_world.rooms[R_WARD3].items[itemIdx].taken, "world item marked taken");

  // note reading inserts into the clue BST
  i32 noteIdx = findProp(R_WARD3, PROP_NOTE, 0);
  CHECK(noteIdx >= 0, "ward note exists");
  pressInteractOn(PROMPT_READ_NOTE, noteIdx);
  CHECK(g_world.clues.search(CLUE_MARA_NOTE) != 0, "reading a note records a clue in the BST");

  // hiding in a locker
  g_world.loadRoom(R_CORRIDOR, 12.f * TILE, 10.5f * TILE, false);
  i32 locker = findProp(R_CORRIDOR, PROP_LOCKER, 0);
  CHECK(locker >= 0, "corridor locker exists");
  pressInteractOn(PROMPT_HIDE, locker);
  CHECK(g_world.player.hasFlag(PF_HIDING), "player hides");
  pressInteractOn(PROMPT_LEAVE_HIDE, locker);
  CHECK(!g_world.player.hasFlag(PF_HIDING), "player leaves the hiding spot");
}

static void testDoorsAndGraphUpdates() {
  SECTION("Doors, keys and graph traversability");
  g_world.init();
  g_world.loadRoom(R_CORRIDOR, 20.f * TILE, 10.5f * TILE, false);
  i32 closetDoor = -1;
  for (i32 i = 0; i < g_world.rooms[R_CORRIDOR].doorCount; ++i) {
    if (g_world.rooms[R_CORRIDOR].doors[i].targetRoom == R_CLOSET) closetDoor = i;
  }
  CHECK(closetDoor >= 0, "closet door exists in the corridor");
  CHECK_EQ(g_world.rooms[R_CORRIDOR].doors[closetDoor].keyItem, IT_WARD_KEY, "closet door needs the ward key");
  CHECK_EQ(g_world.doorTable.unlocked[g_world.rooms[R_CORRIDOR].doors[closetDoor].id], 0, "door starts locked");

  pressInteractOn(PROMPT_LOCKED_DOOR, closetDoor);
  CHECK_EQ(g_world.doorTable.unlocked[g_world.rooms[R_CORRIDOR].doors[closetDoor].id], 0, "locked door refuses to open");
  CHECK_EQ(g_world.room, R_CORRIDOR, "player stays in the corridor");

  g_world.inv.insert(IT_WARD_KEY, 1);
  pressInteractOn(PROMPT_LOCKED_DOOR, closetDoor);
  CHECK_EQ(g_world.doorTable.unlocked[g_world.rooms[R_CORRIDOR].doors[closetDoor].id], 1, "key opens the door");
  i32 linkId = g_world.rooms[R_CORRIDOR].doors[closetDoor].linkId;
  CHECK_EQ(g_world.doorTable.unlocked[linkId], 1, "both sides of the door unlock");

  BFSResult r;
  g_world.bfs.explore(g_world.graph, g_world.doorTable, R_WARD3, r);
  CHECK(r.dist[R_CLOSET] >= 0, "opening the door updates graph reachability (BFS)");
  CHECK(g_world.graph.routeCost(g_world.doorTable, R_WARD3, R_CLOSET) >= 0, "graph route available after unlock");

  pressInteractOn(PROMPT_USE_DOOR, closetDoor);
  CHECK_EQ(g_world.room, R_CLOSET, "walking through the door loads the closet");
}

static void testPuzzles() {
  SECTION("Puzzle systems (valves, fuses, code, candles)");
  g_world.init();
  g_world.loadRoom(R_BOILER, 20.f * TILE, 10.f * TILE, false);

  i32 v0 = findProp(R_BOILER, PROP_VALVE, 0);
  i32 v1 = findProp(R_BOILER, PROP_VALVE, 1);
  i32 v2 = findProp(R_BOILER, PROP_VALVE, 2);
  CHECK(v0 >= 0 && v1 >= 0 && v2 >= 0, "three boiler valves exist");
  pressInteractOn(PROMPT_VALVE, v0);
  CHECK_EQ(g_world.valveProgress, 0, "valve refuses without the handle");
  g_world.inv.insert(IT_VALVE_HANDLE, 1);
  pressInteractOn(PROMPT_VALVE, v0);
  CHECK_EQ(g_world.valveProgress, 0, "wrong valve order resets the sequence");
  pressInteractOn(PROMPT_VALVE, v2);
  CHECK_EQ(g_world.valveProgress, 1, "first valve in the order is accepted");
  pressInteractOn(PROMPT_VALVE, v0);
  CHECK_EQ(g_world.valveProgress, 2, "second valve accepted");
  pressInteractOn(PROMPT_VALVE, v1);
  CHECK_EQ(g_world.valveProgress, 3, "third valve completes the sequence");
  CHECK(g_world.player.hasFlag(PF_VALVES_DONE), "valves flag set");
  CHECK_EQ(g_world.actions.size(), 3, "each valve turn pushed to the action stack");
  CHECK(g_world.graph.routeCost(g_world.doorTable, R_BOILER, R_FLOOD) >= 0, "steam gate opens in the graph");

  // undo stack rolls the puzzle back
  g_world.puzzleUndo();
  CHECK_EQ(g_world.valveProgress, 2, "action stack rollback reverts the last valve");
  CHECK(!g_world.player.hasFlag(PF_VALVES_DONE), "rollback clears the completion flag");

  // re-complete for the fuse puzzle
  pressInteractOn(PROMPT_VALVE, v1);
  i32 breaker = findProp(R_BOILER, PROP_BREAKER, 0);
  CHECK(breaker >= 0, "breaker box exists");
  g_world.inv.insert(IT_FUSE_BLUE, 1);
  g_world.inv.insert(IT_FUSE_RED, 1);
  g_world.inv.insert(IT_FUSE_BRASS, 1);
  pressInteractOn(PROMPT_BREAKER, breaker);
  CHECK_EQ(g_world.fuseMask, 1, "first fuse inserted");
  pressInteractOn(PROMPT_BREAKER, breaker);
  pressInteractOn(PROMPT_BREAKER, breaker);
  CHECK_EQ(g_world.fuseMask, 7, "all three fuses inserted");
  CHECK(g_world.player.hasFlag(PF_POWER_ON), "power restored");

  // code lock
  g_world.loadRoom(R_CHAPEL, 20.f * TILE, 6.f * TILE, false);
  g_world.codeReset();
  g_world.codeDigit(0);
  g_world.codeDigit(3);
  g_world.codeDigit(1);
  CHECK_EQ(g_world.codeSolved, 0, "code not solved on three digits");
  g_world.codeDigit(7);
  CHECK_EQ(g_world.codeSolved, 1, "code 0317 solves the lock");
  CHECK_EQ(g_world.doorTable.unlocked[g_world.rooms[R_CHAPEL].doors[0].id], 1, "code unlocks the morgue door");
  CHECK(g_world.graph.routeCost(g_world.doorTable, R_CHAPEL, R_MORGUE) >= 0, "graph now routes into the morgue");

  // candle ritual
  g_world.loadRoom(R_CHAPEL, 20.f * TILE, 10.f * TILE, false);
  i32 order[4] = { 3, 0, 1, 2 };
  i32 clueBefore = g_world.clues.foundCount();
  for (i32 i = 0; i < 4; ++i) {
    i32 c = findProp(R_CHAPEL, PROP_RITUAL_CANDLE, order[i]);
    CHECK(c >= 0, "ritual candle exists");
    pressInteractOn(PROMPT_CANDLE, c);
  }
  CHECK_EQ(g_world.candleProgress, 4, "candle sequence completed in order");
  CHECK_EQ(g_world.clues.foundCount(), clueBefore + 1, "secret clue granted");
  CHECK_EQ(g_world.secretCount, 1, "secret counter increments");
}

static void testCombatAndEnemyAI() {
  SECTION("Combat and enemy AI");
  g_world.init();
  g_world.loadRoom(R_CORRIDOR, 20.f * TILE, 10.5f * TILE, false);
  g_world.enemies.clear();
  i32 e = g_world.enemies.spawn(E_CRAWLER, g_world.room, 22.f * TILE, 10.5f * TILE);
  CHECK(e >= 0, "crawler spawned");
  Enemy& en = g_world.enemies.list[e];
  en.dirX = -1.f;
  en.dirY = 0.f;

  // detection
  for (i32 i = 0; i < 20; ++i) g_world.update(1.f / 60.f, 0);
  CHECK(en.alert > 0.f, "enemy alert rises when it sees the player");
  CHECK(en.state == ES_CHASE || en.state == ES_ATTACK || en.state == ES_INVESTIGATE,
        "enemy transitions to pursuit after detection");

  // hearing
  g_world.enemies.clear();
  i32 h = g_world.enemies.spawn(E_CRAWLER, g_world.room, 26.f * TILE, 10.5f * TILE);
  Enemy& he = g_world.enemies.list[h];
  he.dirX = -1.f;
  he.dirY = 0.f;
  g_world.player.setFlag(PF_FLASHLIGHT_ON, false);
  g_world.player.setFlag(PF_HIDING, false);
  g_world.pushNoise(g_world.player.x, g_world.player.y, 1.f, 0);
  for (i32 i = 0; i < 30; ++i) g_world.update(1.f / 60.f, 0);
  CHECK(he.alert > 5.f || he.state == ES_INVESTIGATE || he.state == ES_CHASE,
        "enemy reacts to a loud noise (sound-event queue)");
  CHECK_EQ(g_world.sounds.size(), 0, "FIFO queue drains as listeners consume events");

  // shooting: damage, ammo use, noise
  g_world.enemies.clear();
  i32 t = g_world.enemies.spawn(E_HOUND, g_world.room, 22.f * TILE, 10.5f * TILE);
  g_world.player.setFlag(PF_HAS_REVOLVER, true);
  g_world.player.weapon = 2;
  g_world.player.stats[STAT_AMMO_IN_MAG] = 6.f;
  g_world.player.x = 20.f * TILE;
  g_world.player.y = 10.5f * TILE;
  g_world.player.dirX = 1.f;
  g_world.player.dirY = 0.f;
  g_world.enemies.list[t].dirX = -1.f;
  f32 hpBefore = g_world.enemies.list[t].hp;
  g_world.doAttack();
  CHECK(g_world.player.stats[STAT_AMMO_IN_MAG] == 5.f, "firing consumes a round");
  CHECK(g_world.enemies.list[t].hp < hpBefore, "bullet damages the hound");
  CHECK(g_world.sounds.size() > 0, "gunshot pushes a sound event");

  // melee with the crowbar
  g_world.player.weapon = 1;
  g_world.player.setFlag(PF_HAS_CROWBAR, true);
  g_world.enemies.list[t].x = g_world.player.x + 40.f;
  g_world.enemies.list[t].y = g_world.player.y;
  f32 hp2 = g_world.enemies.list[t].hp;
  g_world.doAttack();
  CHECK(g_world.enemies.list[t].hp < hp2, "crowbar swing damages a nearby enemy");

  // the player can die and the state machine reports it
  g_world.loadRoom(R_CORRIDOR, 20.f * TILE, 10.5f * TILE, false);
  g_world.player.stats[STAT_HP] = 8.f;
  g_world.player.stats[STAT_INVULN] = 0.f;
  g_world.hurtPlayer(50.f, g_world.player.x + 20.f, g_world.player.y);
  CHECK_EQ(g_world.state, GS_DEAD, "lethal damage ends the run");
  CHECK(g_world.player.hasFlag(PF_DEAD), "dead flag set");

  // weaver is unkillable but staggers
  g_world.init();
  g_world.loadRoom(R_FLOOD, 20.f * TILE, 10.5f * TILE, false);
  g_world.enemies.clear();
  i32 w = g_world.enemies.spawn(E_WEAVER, g_world.room, 24.f * TILE, 10.5f * TILE);
  g_world.player.setFlag(PF_HAS_REVOLVER, true);
  g_world.player.weapon = 2;
  g_world.player.stats[STAT_AMMO_IN_MAG] = 6.f;
  g_world.player.dirX = 1.f;
  g_world.doAttack();
  CHECK_EQ(g_world.enemies.list[w].hp, g_world.enemies.list[w].maxHp, "the Weaver cannot be shot dead");
  CHECK(g_world.enemies.list[w].staggerTimer > 0.f, "the Weaver staggers when hit");
}

static void testSaveLoadAndEndings() {
  SECTION("Save/load and endings");
  g_world.init();
  g_world.loadRoom(R_NURSE, 10.f * TILE, 10.f * TILE, false);
  g_world.player.stats[STAT_HP] = 63.f;
  g_world.player.setFlag(PF_POWER_ON, true);
  g_world.inv.insert(IT_WARD_KEY, 1);
  g_world.inv.insert(IT_BANDAGE, 3);
  g_world.clues.insert(CLUE_NURSE_MEMO, R_NURSE);
  g_world.clues.insert(CLUE_WARD_LOG, R_CORRIDOR);
  g_world.fuseMask = 5;
  g_world.valveProgress = 2;

  i32 size = 0;
  g_world.serialize();
  size = g_world.saveSize;
  CHECK(size > 64, "serialized save has content");
  static char copy[16384];
  for (i32 i = 0; i < size; ++i) copy[i] = g_world.saveBuf[i];

  g_world.init();
  CHECK_EQ(g_world.player.stats[STAT_HP], 100.f, "fresh state after restart");
  CHECK(g_world.deserialize(copy, size), "save loads");
  CHECK_EQ(g_world.room, R_NURSE, "room restored");
  CHECK_EQ((i32)g_world.player.stats[STAT_HP], 63, "health restored");
  CHECK(g_world.player.hasFlag(PF_POWER_ON), "flags restored");
  CHECK(g_world.inv.find(IT_WARD_KEY) != 0, "inventory restored through the linked list");
  CHECK_EQ(g_world.inv.quantity(IT_BANDAGE), 3, "stack quantities restored");
  CHECK_EQ(g_world.clues.foundCount(), 2, "journal clues restored through the BST");
  CHECK_EQ(g_world.fuseMask, 5, "puzzle state restored");
  CHECK_EQ(g_world.valveProgress, 2, "valve progress restored");

  // endings
  g_world.init();
  g_world.loadRoom(R_SLEEP, 20.f * TILE, 12.f * TILE, false);
  i32 cradle = findProp(R_SLEEP, PROP_CRADLE, 0);
  CHECK(cradle >= 0, "cradle exists in the final chamber");
  pressInteractOn(PROMPT_CRADLE, cradle);
  CHECK_EQ(g_world.state, GS_PLAYING, "cradle does nothing without the locket");
  g_world.inv.insert(IT_LOCKET, 1);
  pressInteractOn(PROMPT_CRADLE, cradle);
  CHECK_EQ(g_world.state, GS_ENDING, "locket alone triggers the sleep ending");
  CHECK_EQ(g_world.endingId, 2, "sleep ending id");

  g_world.init();
  g_world.loadRoom(R_SLEEP, 20.f * TILE, 12.f * TILE, false);
  g_world.inv.insert(IT_LOCKET, 1);
  g_world.inv.insert(IT_SEDATIVE, 1);
  pressInteractOn(PROMPT_CRADLE, cradle);
  CHECK_EQ(g_world.endingId, 1, "locket and sedative trigger the waking ending");
}

static void testLevelIntegrity() {
  SECTION("Level integrity");
  g_world.init();
  CHECK_EQ(g_world.rooms[R_WARD3].propCount > 0, 1, "ward is decorated");
  CHECK(g_world.rooms[R_CHAPEL].propCount > 8, "chapel has authored props");
  i32 notes = 0, interactive = 0, lights = 0, items = 0, doors = 0;
  for (i32 r = 0; r < ROOM_COUNT; ++r) {
    lights += g_world.rooms[r].lightCount;
    items += g_world.rooms[r].itemCount;
    doors += g_world.rooms[r].doorCount;
    for (i32 p = 0; p < g_world.rooms[r].propCount; ++p) {
      const RoomProp& pr = g_world.rooms[r].props[p];
      if (pr.type == PROP_NOTE) notes++;
      if (pr.interactive) interactive++;
    }
    // every room must have walkable floor
    i32 walkable = 0;
    for (i32 c = 0; c < CELLS; ++c) if (!tileSolid(g_world.rooms[r].tiles[c])) walkable++;
    CHECK(walkable > 120, "room has a navigable floor area");
  }
  printf("  [info] notes=%d interactive=%d lights=%d items=%d doors=%d\n", notes, interactive, lights, items, doors);
  CHECK(notes >= 12, "at least twelve readable documents");
  CHECK(interactive >= 25, "plenty of interactive props");
  CHECK(lights >= 20, "rooms are lit deliberately");
  CHECK(items >= 15, "items are distributed through the world");
  CHECK(doors >= 16, "doors connect the world");

  // every spawn point sits on walkable ground
  for (i32 r = 0; r < ROOM_COUNT; ++r) {
    EnemySpawnDef defs[8];
    i32 n = enemySpawnsFor(r, defs, 8);
    for (i32 i = 0; i < n; ++i) {
      CHECK(!g_world.pointBlocked(r, defs[i].x, defs[i].y), "enemy spawn is on walkable ground");
      CHECK(!g_world.pointBlocked(r, defs[i].ax, defs[i].ay), "patrol anchor is walkable");
    }
  }
  // doors sit next to walkable floor and lead to valid rooms
  for (i32 r = 0; r < ROOM_COUNT; ++r) {
    RoomData& rd = g_world.rooms[r];
    for (i32 d = 0; d < rd.doorCount; ++d) {
      CHECK(rd.doors[d].targetRoom >= 0 && rd.doors[d].targetRoom < ROOM_COUNT,
            "door targets a real room");
      CHECK(!g_world.pointBlocked(rd.doors[d].targetRoom, rd.doors[d].spawnX, rd.doors[d].spawnY),
            "door spawn point is walkable");
    }
  }
}

static void testGraphReachabilityAndPursuit() {
  SECTION("Graph reachability, BFS routes, DFS regions and Weaver pursuit");
  g_world.init();
  g_world.loadRoom(R_WARD3, 10.f * TILE, 10.f * TILE, false);

  CHECK(g_world.graph.nodeCount >= ROOM_COUNT, "every room is a graph node");
  CHECK(g_world.reachCount > 0, "reachability is computed when a room loads");
  CHECK_EQ(g_world.reachMask[R_WARD3], 1, "the player's room is reachable from itself");

  // open every door: the whole sanatorium becomes one traversable graph
  for (i32 rm = 0; rm < ROOM_COUNT; ++rm) {
    for (i32 d = 0; d < g_world.rooms[rm].doorCount; ++d) {
      RoomDoor& door = g_world.rooms[rm].doors[d];
      door.open = 1;
      door.locked = 0;
      g_world.doorTable.unlocked[door.id] = 1;
    }
  }
  g_world.refreshReachability();
  CHECK_EQ(g_world.reachCount, ROOM_COUNT, "all ten rooms are reachable with the doors open");

  i32 hop = g_world.nextRoomToward(R_WARD3, R_SLEEP);
  CHECK(hop >= 0 && hop != R_WARD3, "BFS finds a first step towards the cradle room");
  CHECK(g_world.graph.findEdge(R_WARD3, hop) >= 0, "the BFS hop is a real graph edge");

  i32 path[MAX_ROOMS];
  i32 len = g_world.bfs.path(g_world.graph, g_world.doorTable, R_WARD3, R_SLEEP, path, MAX_ROOMS);
  CHECK(len >= 3, "BFS returns a multi-room route across the sanatorium");
  CHECK_EQ(path[0], R_WARD3, "the BFS route starts where asked");
  CHECK_EQ(path[len - 1], R_SLEEP, "the BFS route ends at the target");

  DFSResult dfsRes;
  g_world.dfs.explore(g_world.graph, g_world.doorTable, R_WARD3, dfsRes);
  CHECK_EQ(dfsRes.count, ROOM_COUNT, "DFS reaches every room from the ward");
  CHECK_EQ(g_world.dfs.countRegions(g_world.graph, g_world.doorTable), 1,
           "DFS sees a single connected region");

  // the Weaver walks the graph towards the player instead of teleporting
  g_world.init();
  g_world.loadRoom(R_WARD3, 10.f * TILE, 10.f * TILE, false);
  g_world.player.setFlag(PF_WEAVER_AWAKE, true);
  for (i32 rm = 0; rm < ROOM_COUNT; ++rm) {
    for (i32 d = 0; d < g_world.rooms[rm].doorCount; ++d) {
      RoomDoor& door = g_world.rooms[rm].doors[d];
      door.open = 1;
      door.locked = 0;
      g_world.doorTable.unlocked[door.id] = 1;
    }
  }
  g_world.refreshReachability();
  g_world.enemies.awakeWeaver(g_world, R_CHAPEL, 20.f * TILE, 6.f * TILE);
  i32 wIdx = -1;
  for (i32 i = 0; i < g_world.enemies.count; ++i) {
    Enemy& e = g_world.enemies.list[i];
    if (e.alive && e.type == E_WEAVER) wIdx = i;
  }
  CHECK(wIdx >= 0, "the Weaver spawns when woken");
  if (wIdx >= 0) {
    Enemy& weaver = g_world.enemies.list[wIdx];
    weaver.room = R_CHAPEL;
    weaver.alert = 100.f;
    weaver.graphTimer = 0.f;
    BFSResult before;
    g_world.bfs.explore(g_world.graph, g_world.doorTable, R_CHAPEL, before);
    i32 startDist = before.dist[R_WARD3];
    CHECK(startDist > 0, "the chapel is a real route away from the ward");
    for (i32 step = 0; step < 30; ++step) g_world.enemies.updateAll(g_world, 0.1f);
    CHECK(weaver.room != R_CHAPEL, "the Weaver leaves its room to hunt along the graph");
    BFSResult after;
    g_world.bfs.explore(g_world.graph, g_world.doorTable, weaver.room, after);
    CHECK(after.dist[R_WARD3] >= 0 && after.dist[R_WARD3] < startDist,
          "every hop closes the BFS distance to the player's room");
  }

  // seal the doors: DFS marks the far wing unreachable and BFS has no route
  g_world.init();
  g_world.loadRoom(R_WARD3, 10.f * TILE, 10.f * TILE, false);
  for (i32 rm = 0; rm < ROOM_COUNT; ++rm) {
    for (i32 d = 0; d < g_world.rooms[rm].doorCount; ++d) {
      RoomDoor& door = g_world.rooms[rm].doors[d];
      door.open = 0;
      door.locked = 1;
      g_world.doorTable.unlocked[door.id] = 0;
    }
  }
  g_world.refreshReachability();
  CHECK_EQ(g_world.reachCount, 1, "a sealed ward reaches only itself");
  CHECK_EQ(g_world.reachMask[R_SLEEP], 0, "DFS marks the far wing unreachable");
  CHECK_EQ(g_world.nextRoomToward(R_WARD3, R_SLEEP), -1, "no BFS route through locked doors");
}

int main() {
  printf("THE CRIMSON SLEEP - native core tests\n");
  testGraphAndExplorers();
  testSoundQueueAndDecisions();
  testInventoryAndJournal();
  testNavigation();
  testPlayerMovementAndItems();
  testDoorsAndGraphUpdates();
  testPuzzles();
  testCombatAndEnemyAI();
  testSaveLoadAndEndings();
  testLevelIntegrity();
  testGraphReachabilityAndPursuit();
  printf("\n----------------------------------------\n");
  printf("PASSED: %d   FAILED: %d\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
