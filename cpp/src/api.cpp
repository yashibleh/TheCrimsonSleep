// api.cpp - the WebAssembly boundary. The JavaScript renderer only ever reads
// these flat blocks; all gameplay decisions happen in C++.
#include "GameWorldManager.hpp"
#include "levels.hpp"
#include "Messages.hpp"
#include "BFSExplorer.hpp"
#include "DFSExplorer.hpp"

using namespace tcs;

static f32 g_player[32];
static f32 g_enemies[MAX_ENEMIES * 20];
static f32 g_lights[MAX_LIGHTS * 9];
static f32 g_props[MAX_PROPS * 9];
static f32 g_doors[MAX_ROOM_DOORS * 8];
static f32 g_items[MAX_ITEMS * 6];
static f32 g_prompt[6];
static f32 g_events[MAX_EVENTS * 5];
static f32 g_mapNodes[MAX_ROOMS * 6];
static f32 g_mapEdges[MAX_ROOMS * 30];
static f32 g_code[8];

static void syncPlayer() {
  PlayerState& p = g_world.player;
  g_player[0] = p.x;
  g_player[1] = p.y;
  g_player[2] = p.vx;
  g_player[3] = p.vy;
  g_player[4] = p.dirX;
  g_player[5] = p.dirY;
  g_player[6] = p.stats[STAT_HP];
  g_player[7] = p.stats[STAT_HP_MAX];
  g_player[8] = p.stats[STAT_STAMINA];
  g_player[9] = p.stats[STAT_STAMINA_MAX];
  g_player[10] = p.stats[STAT_BATTERY];
  g_player[11] = p.stats[STAT_BATTERY_MAX];
  g_player[12] = p.stats[STAT_AMMO_IN_MAG];
  g_player[13] = p.stats[STAT_AMMO_MAX_MAG];
  g_player[14] = (f32)p.weapon;
  g_player[15] = p.hasFlag(PF_FLASHLIGHT_ON) ? 1.f : 0.f;
  g_player[16] = p.hasFlag(PF_HIDING) ? 1.f : 0.f;
  g_player[17] = p.stats[STAT_INVULN];
  g_player[18] = p.attackTimer;
  g_player[19] = p.attackCooldown;
  g_player[20] = p.walkPhase;
  g_player[21] = p.stats[STAT_NOISE];
  g_player[22] = p.stats[STAT_DAMAGE_FLASH];
  g_player[23] = p.hasFlag(PF_SPRINTING) ? 1.f : 0.f;
  g_player[24] = p.flags[PF_DEAD] ? 1.f : 0.f;
  g_player[25] = (f32)p.room;
  g_player[26] = (f32)p.hideProp;
  g_player[27] = (f32)p.kills;
  g_player[28] = p.animTime;
  g_player[29] = p.reloadTimer;
  g_player[30] = (f32)g_world.inv.quantity(IT_AMMO);
  g_player[31] = p.heartbeat;
}

static void syncEnemies() {
  for (i32 i = 0; i < MAX_ENEMIES * 20; ++i) g_enemies[i] = 0.f;
  for (i32 i = 0; i < g_world.enemies.count && i < MAX_ENEMIES; ++i) {
    Enemy& e = g_world.enemies.list[i];
    f32* b = &g_enemies[i * 20];
    b[0] = e.x;
    b[1] = e.y;
    b[2] = (f32)e.type;
    b[3] = (f32)e.state;
    b[4] = e.hp;
    b[5] = e.maxHp;
    b[6] = e.dirX;
    b[7] = e.dirY;
    b[8] = e.alert;
    b[9] = e.attackTimer;
    b[10] = e.animPhase;
    b[11] = (f32)e.seesPlayer;
    b[12] = e.stateTimer;
    b[13] = (f32)e.room;
    b[14] = e.hitFlash;
    b[15] = e.anim;
    b[16] = (f32)e.alive;
    b[17] = e.lastKnownX;
    b[18] = e.lastKnownY;
    b[19] = (f32)e.alive;
  }
}

static void syncRoomBlocks() {
  RoomData& r = g_world.rooms[g_world.room];
  for (i32 i = 0; i < MAX_LIGHTS * 9; ++i) g_lights[i] = 0.f;
  for (i32 i = 0; i < r.lightCount && i < MAX_LIGHTS; ++i) {
    f32* b = &g_lights[i * 9];
    b[0] = r.lights[i].x;
    b[1] = r.lights[i].y;
    b[2] = r.lights[i].radius;
    b[3] = r.lights[i].intensity;
    b[4] = r.lights[i].flicker;
    b[5] = r.lights[i].phase;
    b[6] = r.lights[i].r;
    b[7] = r.lights[i].g;
    b[8] = r.lights[i].b;
  }

  for (i32 i = 0; i < MAX_PROPS * 9; ++i) g_props[i] = 0.f;
  for (i32 i = 0; i < r.propCount && i < MAX_PROPS; ++i) {
    f32* b = &g_props[i * 9];
    b[0] = r.props[i].x;
    b[1] = r.props[i].y;
    b[2] = r.props[i].w;
    b[3] = r.props[i].h;
    b[4] = (f32)r.props[i].type;
    b[5] = (f32)r.props[i].id;
    b[6] = (f32)r.props[i].state;
    b[7] = (f32)r.props[i].labelId;
    b[8] = (f32)r.props[i].interactive;
  }

  for (i32 i = 0; i < MAX_ROOM_DOORS * 8; ++i) g_doors[i] = 0.f;
  for (i32 i = 0; i < r.doorCount && i < MAX_ROOM_DOORS; ++i) {
    f32* b = &g_doors[i * 8];
    b[0] = r.doors[i].x;
    b[1] = r.doors[i].y;
    b[2] = (f32)g_world.doorTable.unlocked[r.doors[i].id];
    b[3] = (f32)r.doors[i].locked;
    b[4] = (f32)r.doors[i].dir;
    b[5] = (f32)r.doors[i].targetRoom;
    b[6] = (f32)r.doors[i].keyItem;
    b[7] = (f32)r.doors[i].labelId;
  }

  for (i32 i = 0; i < MAX_ITEMS * 6; ++i) g_items[i] = 0.f;
  for (i32 i = 0; i < r.itemCount && i < MAX_ITEMS; ++i) {
    f32* b = &g_items[i * 6];
    b[0] = r.items[i].x;
    b[1] = r.items[i].y;
    b[2] = (f32)r.items[i].itemId;
    b[3] = (f32)r.items[i].qty;
    b[4] = (f32)r.items[i].glow;
    b[5] = (f32)r.items[i].taken;
  }
}

static void syncPrompt() {
  g_prompt[0] = (f32)g_world.prompt.id;
  g_prompt[1] = (f32)g_world.prompt.labelId;
  g_prompt[2] = (f32)g_world.prompt.target;
  g_prompt[3] = g_world.prompt.x;
  g_prompt[4] = g_world.prompt.y;
  g_prompt[5] = (f32)g_world.prompt.id;
}

static void syncEvents() {
  for (i32 i = 0; i < MAX_EVENTS * 5; ++i) g_events[i] = 0.f;
  for (i32 i = 0; i < g_world.eventCount && i < MAX_EVENTS; ++i) {
    f32* b = &g_events[i * 5];
    b[0] = g_world.events[i].type;
    b[1] = g_world.events[i].x;
    b[2] = g_world.events[i].y;
    b[3] = g_world.events[i].a;
    b[4] = g_world.events[i].b;
  }
}

static void syncMap() {
  WorldGraph& g = g_world.graph;
  for (i32 i = 0; i < MAX_ROOMS * 6; ++i) g_mapNodes[i] = 0.f;
  for (i32 i = 0; i < g.nodeCount; ++i) {
    f32* b = &g_mapNodes[i * 6];
    b[0] = (f32)g.nodes[i].id;
    b[1] = (f32)g.nodes[i].nameId;
    b[2] = g.nodes[i].cx;
    b[3] = g.nodes[i].cy;
    b[4] = (f32)g.nodes[i].palette;
    b[5] = (f32)g.nodes[i].floorIndex;
  }
  for (i32 i = 0; i < MAX_ROOMS * 30; ++i) g_mapEdges[i] = 0.f;
  i32 n = 0;
  for (i32 i = 0; i < g.nodeCount; ++i) {
    for (i32 k = 0; k < g.nodes[i].edgeCount; ++k) {
      const GraphEdge& e = g.edges[g.nodes[i].edgeStart + k];
      if (e.to < i) continue;
      if (n >= MAX_ROOMS * 10) break;
      f32* b = &g_mapEdges[n * 3];
      b[0] = (f32)i;
      b[1] = (f32)e.to;
      b[2] = g_world.doorTable.unlocked[e.doorId >= 0 ? e.doorId : 0] ? 1.f : 0.f;
      n++;
    }
  }
  g_mapEdges[MAX_ROOMS * 30 - 1] = (f32)n;
}

static void syncCode() {
  for (i32 i = 0; i < 8; ++i) g_code[i] = 0.f;
  for (i32 i = 0; i < 4; ++i) g_code[i] = (f32)g_world.codeDigits[i];
  g_code[4] = (f32)g_world.codeProgress;
  g_code[5] = (f32)g_world.codeSolved;
  g_code[6] = (f32)g_world.valveProgress;
  g_code[7] = (f32)g_world.candleProgress;
}

static void syncAll() {
  syncPlayer();
  syncEnemies();
  syncRoomBlocks();
  syncPrompt();
  syncEvents();
  syncMap();
  syncCode();
}

// ------------------------------------------------------------------ exports
extern "C" void tcs_init() {
  g_world.init();
  syncAll();
}

extern "C" void tcs_new_game() {
  g_world.newGame();
  syncAll();
}

extern "C" void tcs_update(f32 dt, i32 input) {
  g_world.update(dt, input);
  g_world.eventCount = g_world.eventCount;
  syncAll();
}

extern "C" void tcs_clear_events() { g_world.eventCount = 0; syncEvents(); }

extern "C" int tcs_room_id() { return g_world.room; }
extern "C" int tcs_room_name() { return g_world.rooms[g_world.room].nameId; }
extern "C" int tcs_room_palette() { return g_world.rooms[g_world.room].palette; }
extern "C" int tcs_room_ambient() { return g_world.rooms[g_world.room].ambient; }
extern "C" int tcs_room_music() { return g_world.rooms[g_world.room].music; }
extern "C" int tcs_room_floor() { return g_world.rooms[g_world.room].floorIndex; }
extern "C" int tcs_room_w() { return ROOM_W; }
extern "C" int tcs_room_h() { return ROOM_H; }

extern "C" int tcs_tiles() { return (int)(unsigned long)&g_world.rooms[g_world.room].tiles[0]; }
extern "C" int tcs_decor() { return (int)(unsigned long)&g_world.rooms[g_world.room].decor[0]; }
extern "C" int tcs_navgrid() { return (int)(unsigned long)&g_world.rooms[g_world.room].navBlocked[0]; }
extern "C" int tcs_lights() { return (int)(unsigned long)g_lights; }
extern "C" int tcs_light_count() { return g_world.rooms[g_world.room].lightCount; }
extern "C" int tcs_props() { return (int)(unsigned long)g_props; }
extern "C" int tcs_prop_count() { return g_world.rooms[g_world.room].propCount; }
extern "C" int tcs_doors() { return (int)(unsigned long)g_doors; }
extern "C" int tcs_door_count() { return g_world.rooms[g_world.room].doorCount; }
extern "C" int tcs_items() { return (int)(unsigned long)g_items; }
extern "C" int tcs_item_count() { return g_world.rooms[g_world.room].itemCount; }
extern "C" int tcs_player() { return (int)(unsigned long)g_player; }
extern "C" int tcs_enemies() { return (int)(unsigned long)g_enemies; }
extern "C" int tcs_enemy_count() { return g_world.enemies.count; }
extern "C" int tcs_prompt_ptr() { return (int)(unsigned long)g_prompt; }
extern "C" int tcs_events() { return (int)(unsigned long)g_events; }
extern "C" int tcs_event_count() { return g_world.eventCount; }

extern "C" int tcs_state() { return g_world.state; }
extern "C" int tcs_ending() { return g_world.endingId; }
extern "C" int tcs_objective() { return g_world.computeObjective(); }
extern "C" int tcs_message() { return g_world.messageId; }
extern "C" int tcs_message_timer() { return (int)(g_world.messageTimer * 1000.f); }
extern "C" int tcs_subtitle() { return g_world.subtitleId; }
extern "C" int tcs_subtitle_timer() { return (int)(g_world.subtitleTimer * 1000.f); }
extern "C" int tcs_ui_request() { return g_world.uiRequest; }
extern "C" void tcs_ui_clear() { g_world.uiRequest = 0; }
extern "C" int tcs_danger() { return (int)(g_world.danger * 100.f); }
extern "C" int tcs_tick() { return (int)(g_world.tick * 100.f); }

extern "C" int tcs_inv_count() { return g_world.inv.count; }
extern "C" int tcs_inv_id(int i) {
  InventoryNode* n = g_world.inv.at(i);
  return n ? n->itemId : 0;
}
extern "C" int tcs_inv_qty(int i) {
  InventoryNode* n = g_world.inv.at(i);
  return n ? n->quantity : 0;
}
extern "C" int tcs_inv_cat(int i) {
  InventoryNode* n = g_world.inv.at(i);
  return n ? n->category : 0;
}
extern "C" int tcs_inv_usable(int i) {
  InventoryNode* n = g_world.inv.at(i);
  return n ? n->usable : 0;
}
extern "C" int tcs_has_item(int itemId) { return g_world.inv.find(itemId) ? 1 : 0; }
extern "C" void tcs_use_item(int itemId) {
  g_world.useItemById(itemId);
  syncAll();
}

extern "C" int tcs_code_ptr() { return (int)(unsigned long)g_code; }
extern "C" void tcs_code_digit(int d) {
  g_world.codeDigit(d);
  syncCode();
  syncRoomBlocks();
}
extern "C" void tcs_code_reset() {
  g_world.codeReset();
  syncCode();
}
extern "C" void tcs_puzzle_undo() {
  g_world.puzzleUndo();
  syncCode();
  syncRoomBlocks();
}
extern "C" int tcs_fuse_mask() { return g_world.fuseMask; }
extern "C" int tcs_candle_order(int i) {
  if (i < 0 || i > 3) return 0;
  return g_world.candleOrder[i];
}

extern "C" int tcs_clue_count() {
  i32 ids[MAX_CLUES];
  return g_world.clues.inOrder(ids, MAX_CLUES);
}
extern "C" int tcs_clue_id(int i) {
  i32 ids[MAX_CLUES];
  i32 n = g_world.clues.inOrder(ids, MAX_CLUES);
  if (i < 0 || i >= n) return 0;
  return ids[i];
}
extern "C" int tcs_has_clue(int id) { return g_world.clues.search(id) ? 1 : 0; }
extern "C" int tcs_clue_height() { return g_world.clues.height(); }

extern "C" int tcs_serialize() {
  g_world.serialize();
  return g_world.saveSize;
}
extern "C" int tcs_save_ptr() { return (int)(unsigned long)g_world.saveBuf; }
extern "C" int tcs_deserialize(int ptr, int len) {
  const char* data = (const char*)(unsigned long)ptr;
  bool ok = g_world.deserialize(data, len);
  syncAll();
  return ok ? 1 : 0;
}

extern "C" int tcs_deaths() { return g_world.deaths; }
extern "C" int tcs_checkpoints() { return g_world.checkpoints; }
extern "C" int tcs_kills() { return g_world.totalKills; }
extern "C" int tcs_secrets() { return g_world.secretCount; }
extern "C" int tcs_visited(int room) {
  if (room < 0 || room >= MAX_ROOMS) return 0;
  return g_world.visited[room] ? 1 : 0;
}

extern "C" int tcs_map_nodes() { return (int)(unsigned long)g_mapNodes; }
extern "C" int tcs_map_edges() { return (int)(unsigned long)g_mapEdges; }
extern "C" int tcs_map_node_count() { return g_world.graph.nodeCount; }
extern "C" int tcs_map_edge_count() { return (int)g_mapEdges[MAX_ROOMS * 30 - 1]; }

// --------------------------------------------------- exploration algorithms
extern "C" int tcs_bfs_reach(int from) {
  BFSResult r;
  g_world.bfs.explore(g_world.graph, g_world.doorTable, from, r);
  return r.count;
}
extern "C" int tcs_bfs_dist(int from, int to) {
  return g_world.graph.routeCost(g_world.doorTable, from, to);
}
extern "C" int tcs_bfs_reach_ptr(int from) {
  static f32 block[MAX_ROOMS * 3];
  BFSResult r;
  g_world.bfs.explore(g_world.graph, g_world.doorTable, from, r);
  for (i32 i = 0; i < MAX_ROOMS; ++i) {
    block[i * 3 + 0] = (f32)(i < r.count ? r.order[i] : -1);
    block[i * 3 + 1] = (f32)r.dist[i];
    block[i * 3 + 2] = (f32)r.parent[i];
  }
  return (int)(unsigned long)block;
}
extern "C" int tcs_dfs_regions() {
  return g_world.dfs.countRegions(g_world.graph, g_world.doorTable);
}
extern "C" int tcs_dfs_deepest(int from) {
  return g_world.dfs.deepestRoom(g_world.graph, g_world.doorTable, from);
}
extern "C" int tcs_dfs_order_ptr(int from) {
  static f32 block[MAX_ROOMS * 3];
  DFSResult r;
  g_world.dfs.explore(g_world.graph, g_world.doorTable, from, r);
  for (i32 i = 0; i < MAX_ROOMS; ++i) {
    block[i * 3 + 0] = (f32)(i < r.count ? r.order[i] : -1);
    block[i * 3 + 1] = (f32)r.depth[i];
    block[i * 3 + 2] = (f32)r.parent[i];
  }
  return (int)(unsigned long)block;
}
extern "C" int tcs_sound_count() { return g_world.sounds.size(); }
extern "C" int tcs_action_count() { return g_world.actions.size(); }
extern "C" int tcs_reach_count() { return g_world.reachCount; }
extern "C" int tcs_reach_mask(int room) {
  if (room < 0 || room >= MAX_ROOMS) return 0;
  return g_world.reachMask[room];
}
extern "C" int tcs_next_room(int from, int to) { return g_world.nextRoomToward(from, to); }
extern "C" int tcs_graph_degree(int room) { return g_world.graph.degree(room); }

// ------------------------------------------------------------ test hooks
extern "C" void tcs_debug_give(int itemId, int qty) {
  g_world.inv.insert(itemId, qty);
  if (itemId == IT_REVOLVER) {
    g_world.player.setFlag(PF_HAS_REVOLVER, true);
    g_world.player.weapon = 2;
  }
  if (itemId == IT_CROWBAR) {
    g_world.player.setFlag(PF_HAS_CROWBAR, true);
    g_world.player.weapon = 1;
  }
}
extern "C" void tcs_debug_teleport(int room, float x, float y) {
  if (room < 0 || room >= MAX_ROOMS) return;
  g_world.enemies.clear();
  g_world.loadRoom(room, x, y, false);
  syncAll();
}
extern "C" int tcs_debug_enemy_type(int i) {
  if (i < 0 || i >= g_world.enemies.count) return -1;
  return g_world.enemies.list[i].type;
}
extern "C" int tcs_debug_enemy_state(int i) {
  if (i < 0 || i >= g_world.enemies.count) return -1;
  return g_world.enemies.list[i].state;
}
extern "C" int tcs_debug_enemy_room(int i) {
  if (i < 0 || i >= g_world.enemies.count) return -1;
  return g_world.enemies.list[i].room;
}
extern "C" int tcs_debug_door_unlocked(int doorId) {
  if (doorId < 0 || doorId >= MAX_ROOMS * MAX_ROOM_DOORS) return 0;
  return g_world.doorTable.unlocked[doorId] ? 1 : 0;
}
extern "C" int tcs_debug_set_pos(float x, float y) {
  g_world.player.x = x;
  g_world.player.y = y;
  syncAll();
  return 1;
}
extern "C" int tcs_debug_player_hp() { return (int)g_world.player.stats[STAT_HP]; }
extern "C" void tcs_debug_hurt(int dmg) { g_world.hurtPlayer((f32)dmg, g_world.player.x + 40.f, g_world.player.y); }
extern "C" int tcs_debug_interact() { g_world.interact(); return 0; }
extern "C" int tcs_debug_tile(int cx, int cy) {
  return g_world.roomTileAt(g_world.room, cx, cy);
}
extern "C" int tcs_debug_room_tile(int room, int cx, int cy) {
  return g_world.roomTileAt(room, cx, cy);
}
extern "C" int tcs_debug_point_blocked(float x, float y) {
  return g_world.pointBlocked(g_world.room, x, y) ? 1 : 0;
}
extern "C" int tcs_debug_enemy_count_alive() {
  i32 n = 0;
  for (i32 i = 0; i < g_world.enemies.count; ++i) if (g_world.enemies.list[i].alive) n++;
  return n;
}
