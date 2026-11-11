// GameWorldManager: owns the entire simulation - rooms, player, items, doors,
// puzzles, scripting, save/load. The renderer is a read-only client of this.
#pragma once
#include "core.hpp"
#include "Content.hpp"
#include "WorldGraph.hpp"
#include "BFSExplorer.hpp"
#include "DFSExplorer.hpp"
#include "PlayerInventory.hpp"
#include "ClueBST.hpp"
#include "PuzzleActionStack.hpp"
#include "SoundEventQueue.hpp"
#include "PlayerState.hpp"
#include "EnemyController.hpp"
#include "NavigationSystem.hpp"

namespace tcs {

// Input bit layout shared with the JavaScript input layer.
enum InputBits : i32 {
  IN_UP      = 1 << 0,
  IN_DOWN    = 1 << 1,
  IN_LEFT    = 1 << 2,
  IN_RIGHT   = 1 << 3,
  IN_ATTACK  = 1 << 4,
  IN_INTERACT= 1 << 5,
  IN_LIGHT   = 1 << 6,
  IN_SPRINT  = 1 << 7,
  IN_RELOAD  = 1 << 8,
  IN_HEAL    = 1 << 9,
  IN_SWAP    = 1 << 10,
  IN_USE     = 1 << 11,
  IN_HIDE    = 1 << 12
};

struct RoomLight {
  f32 x, y, radius, intensity, flicker, phase;
  f32 r, g, b;
};

struct RoomProp {
  i32 type;
  f32 x, y, w, h;
  i32 id;
  i32 state;
  i32 labelId;
  i32 clueId;
  i32 itemId;
  f32 hideX, hideY;
  u8 solid;
  u8 discovered;
  u8 interactive;
};

struct RoomDoor {
  i32 id;
  i32 linkId;      // matching door in the target room
  f32 x, y;
  i32 w, h;
  i32 targetRoom;
  f32 spawnX, spawnY;
  i32 keyItem;
  i32 dir;          // 0 up, 1 right, 2 down, 3 left (sprite orientation)
  i32 labelId;
  u8 open;
  u8 locked;
};

struct RoomItem {
  i32 itemId;
  f32 x, y;
  i32 qty;
  i32 glow;
  u8 taken;
  f32 bob;
};

struct RoomData {
  u8 tiles[CELLS];
  u8 decor[CELLS];
  u8 navBlocked[CELLS];  // tiles + solid props + locked doors, rebuilt on change
  i32 palette;
  i32 nameId;
  i32 floorIndex;
  i32 ambient;      // 0 completely dark .. 100 lit
  i32 music;        // music mood id
  i32 lightCount;
  RoomLight lights[MAX_LIGHTS];
  i32 propCount;
  RoomProp props[MAX_PROPS];
  i32 doorCount;
  RoomDoor doors[MAX_ROOM_DOORS];
  i32 itemCount;
  RoomItem items[MAX_ITEMS];
};

struct PromptState {
  i32 id;
  i32 target;
  i32 labelId;
  f32 x, y;
};

enum GameState : i32 {
  GS_PLAYING = 0,
  GS_DEAD = 1,
  GS_ENDING = 2
};

class GameWorldManager {
public:
  RoomData rooms[MAX_ROOMS];
  WorldGraph graph;
  BFSExplorer bfs;
  DFSExplorer dfs;
  PlayerInventory inv;
  ClueBST clues;
  PuzzleActionStack actions;
  SoundEventQueue sounds;
  EnemyController enemies;
  PlayerState player;
  DoorTable doorTable;
  NavSystem nav;

  u8 visited[MAX_ROOMS];
  u8 reachMask[MAX_ROOMS];  // DFS: rooms reachable from here through open doors
  i32 reachCount;
  u8 storyFlags[16];
  i32 room;
  f32 tick;
  f32 roomTime;
  i32 state;
  i32 endingId;
  i32 objective;
  i32 objectiveProgress;
  f32 danger;             // 0..1 nearest-threat pressure
  f32 chaseHeat;

  // puzzle state
  i32 valveProgress;
  i32 valveOrder[3];
  i32 fuseMask;
  i32 codeDigits[4];
  i32 codeProgress;
  i32 codeSolved;
  i32 candleProgress;
  i32 candleOrder[4];
  i32 weaverPhase;

  PromptState prompt;
  i32 messageId;
  f32 messageTimer;
  i32 subtitleId;
  f32 subtitleTimer;

  GameEvent events[MAX_EVENTS];
  i32 eventCount;

  Rng rng;
  f32 flashTimer;
  f32 shakeTimer;
  f32 shakeStrength;

  char saveBuf[16384];
  i32 saveSize;

  i32 deaths;
  i32 checkpoints;
  i32 totalKills;
  i32 secretCount;

  // lifecycle ------------------------------------------------------------
  void init();
  void newGame();
  void loadRoom(i32 id, f32 spawnX, f32 spawnY, bool preserveEnemies);
  void update(f32 dt, i32 input);

  // subsystems -----------------------------------------------------------
  void updatePlayer(f32 dt, i32 input);
  void updatePrompt();
  void updateTriggers();
  void updateScripted(f32 dt);
  void updateAssetBobbing(f32 dt);
  void interact();
  void doAttack();
  void fireRevolver();
  void meleeSwing();
  void useHeal();
  void useItemById(i32 itemId);
  void codeDigit(i32 digit);
  void codeReset();
  void puzzleUndo();
  i32 uiRequest;   // renderer-side panel requests (code keypad)
  void tryReload();
  void swapWeapon();
  void takeItem(i32 roomIdx, i32 itemIdx);
  void unlockDoor(i32 doorIdx);
  void travelThroughDoor(i32 doorIdx);
  void hurtPlayer(f32 dmg, f32 fromX, f32 fromY);
  void pushEvent(i32 type, f32 x, f32 y, f32 a, f32 b);
  void pushNoise(f32 x, f32 y, f32 intensity, i32 kind);
  void notify(i32 messageId);
  void addClue(i32 clueId, i32 roomId);
  void solveCode();
  i32 roomTileAt(i32 roomIdx, i32 cx, i32 cy) const;
  bool tileBlocked(i32 roomIdx, i32 cx, i32 cy) const;
  bool pointBlocked(i32 roomIdx, f32 x, f32 y) const;
  void moveCircle(i32 roomIdx, f32& x, f32& y, f32 dx, f32 dy, f32 radius);
  void rebuildNav();
  void refreshReachability();
  i32 nextRoomToward(i32 from, i32 to);
  bool propBlocks(i32 roomIdx, f32 x, f32 y, f32 radius) const;
  i32 promptLabelForProp(const RoomProp& p) const;

  // save / load ----------------------------------------------------------
  void serialize();
  bool deserialize(const char* data, i32 len);
  void writeInt(i32& cursor, i32 value);
  i32 readInt(i32& cursor);
  void writeFloat(i32& cursor, f32 value);
  f32 readFloat(i32& cursor);

  // derived queries used by the UI layer
  i32 computeObjective() const;
  f32 proximityThreat() const;
};

extern GameWorldManager g_world;

} // namespace tcs
