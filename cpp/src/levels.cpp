// levels.cpp - authored world. Every room is deliberately laid out: shape,
// props, lighting and scripted furniture, not random generation.
#include "GameWorldManager.hpp"
#include "levels.hpp"

namespace tcs {
namespace {

void fillRoom(RoomData& r, u8 tile) {
  for (i32 i = 0; i < CELLS; ++i) r.tiles[i] = tile;
}

void setTile(RoomData& r, i32 x, i32 y, u8 tile) {
  if (x < 0 || y < 0 || x >= ROOM_W || y >= ROOM_H) return;
  r.tiles[y * ROOM_W + x] = tile;
}

void rect(RoomData& r, i32 x0, i32 y0, i32 x1, i32 y1, u8 tile) {
  for (i32 y = y0; y <= y1; ++y)
    for (i32 x = x0; x <= x1; ++x) setTile(r, x, y, tile);
}

void decorPass(RoomData& r, i32 salt) {
  Rng g;
  g.seed(0x51CEDu + (u32)(salt * 7919));
  for (i32 i = 0; i < CELLS; ++i) r.decor[i] = (u8)(g.next() & 0xFF);
}

void roomClear(RoomData& r) {
  fillRoom(r, T_VOID);
  r.lightCount = 0;
  r.propCount = 0;
  r.doorCount = 0;
  r.itemCount = 0;
}

void roomShell(RoomData& r, u8 wall, u8 floor) {
  rect(r, 1, 1, 38, 20, wall);
  rect(r, 2, 2, 37, 19, floor);
}

i32 addProp(RoomData& r, i32 type, f32 x, f32 y, f32 w, f32 h, i32 id,
            i32 labelId, i32 clueId, i32 itemId, bool solid, bool interactive) {
  if (r.propCount >= MAX_PROPS) return -1;
  i32 idx = r.propCount++;
  RoomProp& p = r.props[idx];
  p.type = type;
  p.x = x;
  p.y = y;
  p.w = w;
  p.h = h;
  p.id = id;
  p.state = 0;
  p.labelId = labelId;
  p.clueId = clueId;
  p.itemId = itemId;
  p.hideX = x;
  p.hideY = y;
  p.solid = solid ? 1 : 0;
  p.discovered = 0;
  p.interactive = interactive ? 1 : 0;
  return idx;
}

void addLight(RoomData& r, f32 x, f32 y, f32 radius, f32 intensity, f32 flicker,
              f32 cr, f32 cg, f32 cb) {
  if (r.lightCount >= MAX_LIGHTS) return;
  RoomLight& l = r.lights[r.lightCount++];
  l.x = x;
  l.y = y;
  l.radius = radius;
  l.intensity = intensity;
  l.flicker = flicker;
  l.phase = (f32)((x * 0.017f + y * 0.031f) - (f32)(i32)(x * 0.017f));
  l.r = cr;
  l.g = cg;
  l.b = cb;
}

void addItem(RoomData& r, i32 itemId, f32 x, f32 y, i32 qty, i32 glow) {
  if (r.itemCount >= MAX_ITEMS) return;
  RoomItem& it = r.items[r.itemCount++];
  it.itemId = itemId;
  it.x = x;
  it.y = y;
  it.qty = qty;
  it.taken = 0;
  it.glow = glow;
  it.bob = 0.f;
}

i32 addDoor(GameWorldManager& w, i32 roomIdx, i32 tx, i32 ty, i32 targetRoom,
            f32 spawnX, f32 spawnY, i32 keyItem, i32 dir) {
  RoomData& r = w.rooms[roomIdx];
  if (r.doorCount >= MAX_ROOM_DOORS) return -1;
  i32 idx = r.doorCount++;
  RoomDoor& d = r.doors[idx];
  d.id = doorKey(roomIdx, idx);
  d.linkId = -1;
  d.x = (f32)tx * TILE + TILE * 0.5f;
  d.y = (f32)ty * TILE + TILE * 0.5f;
  d.w = TILE;
  d.h = TILE;
  d.targetRoom = targetRoom;
  d.spawnX = spawnX;
  d.spawnY = spawnY;
  d.keyItem = keyItem;
  d.dir = dir;
  d.labelId = keyItem == 0 ? PROMPT_USE_DOOR : PROMPT_LOCKED_DOOR;
  d.open = keyItem == 0 ? 1 : 0;
  d.locked = keyItem == 0 ? 0 : 1;
  w.doorTable.unlocked[d.id] = keyItem == 0 ? 1 : 0;
  setTile(r, tx, ty, roomIdx == R_BOILER || roomIdx == R_FLOOD ? T_METAL : T_WOOD);
  return idx;
}

void connect(GameWorldManager& w, i32 roomA, i32 ax, i32 ay, i32 roomB, i32 bx, i32 by,
             i32 keyItem, i32 dirA, i32 dirB, f32 spawnAx, f32 spawnAy, f32 spawnBx, f32 spawnBy) {
  i32 da = addDoor(w, roomA, ax, ay, roomB, spawnBx, spawnBy, keyItem, dirA);
  i32 db = addDoor(w, roomB, bx, by, roomA, spawnAx, spawnAy, keyItem, dirB);
  if (da < 0 || db < 0) return;
  w.rooms[roomA].doors[da].linkId = w.rooms[roomB].doors[db].id;
  w.rooms[roomB].doors[db].linkId = w.rooms[roomA].doors[da].id;
  w.graph.addEdge(roomA, roomB, 1.f, w.rooms[roomA].doors[da].id);
}

// ------------------------------------------------------------------ rooms

void buildWard3(GameWorldManager& w) {
  RoomData& r = w.rooms[R_WARD3];
  r.palette = 0;
  r.nameId = 1;
  r.floorIndex = 0;
  r.ambient = 26;
  r.music = 0;
  roomShell(r, T_WALL, T_TILE);

  // divider creates two bays; a broken gap joins them
  rect(r, 26, 2, 27, 12, T_WALL);
  rect(r, 26, 9, 27, 10, T_TILE);
  // a collapsed corner and damp patch
  rect(r, 30, 16, 34, 18, T_BLOOD);
  rect(r, 3, 16, 5, 18, T_RUBBLE);
  rect(r, 14, 11, 22, 12, T_CARPET);

  addProp(r, PROP_BED, 7.f * TILE, 4.f * TILE, 92.f, 58.f, 0, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_BED, 14.f * TILE, 4.f * TILE, 92.f, 58.f, 1, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_BED, 21.f * TILE, 4.f * TILE, 92.f, 58.f, 2, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_BED, 31.f * TILE, 4.f * TILE, 92.f, 58.f, 3, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_MIRROR, 34.f * TILE, 3.f * TILE, 34.f, 54.f, 0, PROMPT_MIRROR, 0, 0, false, true);
  addProp(r, PROP_GURNEY, 17.f * TILE, 15.f * TILE, 104.f, 44.f, 0, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_CANDLE, 3.5f * TILE, 12.f * TILE, 26.f, 40.f, 0, PROMPT_SAVE, 0, 0, false, true);
  addProp(r, PROP_NOTE, 9.f * TILE, 5.5f * TILE, 26.f, 24.f, 0, PROMPT_READ_NOTE, CLUE_MARA_NOTE, 0, false, true);
  addProp(r, PROP_WINDOW, 11.f * TILE, 2.f * TILE, 96.f, 30.f, 0, PROMPT_EXAMINE, 0, 0, false, true);

  addItem(r, IT_BANDAGE, 15.f * TILE, 5.5f * TILE, 1, 1);
  addItem(r, IT_BATTERY, 22.f * TILE, 5.5f * TILE, 1, 1);
  addItem(r, IT_MATCHES, 6.f * TILE, 6.f * TILE, 4, 1);

  addLight(r, 34.f * TILE, 3.f * TILE, 190.f, 0.55f, 0.25f, 0.72f, 0.68f, 0.55f);
  addLight(r, 3.5f * TILE, 12.f * TILE, 150.f, 0.42f, 0.5f, 1.f, 0.55f, 0.30f);
  addLight(r, 18.f * TILE, 3.f * TILE, 210.f, 0.20f, 0.15f, 0.55f, 0.62f, 0.66f);

  connect(w, R_WARD3, 38, 10, R_CORRIDOR, 1, 10, 0, 1, 3, 36.5f * TILE, 10.5f * TILE, 2.8f * TILE, 10.5f * TILE);
}

void buildCorridor(GameWorldManager& w) {
  RoomData& r = w.rooms[R_CORRIDOR];
  r.palette = 1;
  r.nameId = 2;
  r.floorIndex = 0;
  r.ambient = 18;
  r.music = 1;
  roomShell(r, T_WALL, T_WOOD);
  rect(r, 3, 9, 36, 9, T_CARPET);
  rect(r, 3, 12, 36, 12, T_CARPET);
  // north alcove
  rect(r, 9, 4, 14, 7, T_WOOD);
  // south alcove
  rect(r, 24, 13, 29, 17, T_WOOD);
  rect(r, 23, 14, 23, 18, T_WALL);
  rect(r, 30, 14, 30, 18, T_WALL);
  rect(r, 24, 18, 29, 18, T_WALL);
  // collapsed section mid-corridor
  rect(r, 20, 8, 22, 8, T_RUBBLE);
  rect(r, 33, 13, 36, 13, T_BLOOD);

  addProp(r, PROP_NOTE, 5.f * TILE, 9.5f * TILE, 26.f, 24.f, 1, PROMPT_READ_NOTE, CLUE_WARD_LOG, 0, false, true);
  addProp(r, PROP_GURNEY, 17.f * TILE, 9.f * TILE, 104.f, 40.f, 1, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_LAMP, 7.f * TILE, 12.f * TILE, 30.f, 46.f, 0, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_LAMP, 33.f * TILE, 9.f * TILE, 30.f, 46.f, 1, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_BOARD, 33.f * TILE, 19.f * TILE, 64.f, 34.f, 0, PROMPT_BOARD, CLUE_HIDDEN_PANEL, IT_AMMO, false, true);
  addProp(r, PROP_CANDLE, 20.f * TILE, 12.5f * TILE, 26.f, 40.f, 1, PROMPT_SAVE, 0, 0, false, true);
  addProp(r, PROP_LOCKER, 12.f * TILE, 12.5f * TILE, 38.f, 52.f, 0, PROMPT_HIDE, 0, 0, true, true);
  addProp(r, PROP_WINDOW, 16.f * TILE, 2.f * TILE, 110.f, 30.f, 1, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_BODY, 28.f * TILE, 16.f * TILE, 60.f, 30.f, 0, PROMPT_BODY, 0, 0, false, true);

  addLight(r, 7.f * TILE, 12.f * TILE, 190.f, 0.6f, 0.55f, 1.f, 0.62f, 0.36f);
  addLight(r, 33.f * TILE, 9.f * TILE, 180.f, 0.5f, 0.75f, 0.9f, 0.66f, 0.42f);
  addLight(r, 20.f * TILE, 12.5f * TILE, 150.f, 0.35f, 0.3f, 1.f, 0.55f, 0.30f);
  addLight(r, 11.f * TILE, 6.f * TILE, 150.f, 0.30f, 0.2f, 0.5f, 0.58f, 0.6f);

  connect(w, R_CORRIDOR, 38, 10, R_BOILER, 2, 10, 0, 1, 3, 36.5f * TILE, 10.5f * TILE, 3.2f * TILE, 10.5f * TILE);
  connect(w, R_CORRIDOR, 11, 3, R_NURSE, 11, 20, 0, 0, 2, 11.5f * TILE, 5.6f * TILE, 11.5f * TILE, 18.4f * TILE);
  connect(w, R_CORRIDOR, 26, 18, R_CLOSET, 20, 18, IT_WARD_KEY, 2, 0, 26.5f * TILE, 16.6f * TILE, 20.5f * TILE, 16.6f * TILE);
}

void buildNurse(GameWorldManager& w) {
  RoomData& r = w.rooms[R_NURSE];
  r.palette = 2;
  r.nameId = 3;
  r.floorIndex = 0;
  r.ambient = 32;
  r.music = 0;
  roomShell(r, T_WALL, T_TILE);
  rect(r, 4, 12, 12, 12, T_CARPET);
  rect(r, 15, 3, 16, 7, T_WALL);
  rect(r, 15, 8, 16, 9, T_TILE);

  addProp(r, PROP_GURNEY, 10.f * TILE, 9.f * TILE, 180.f, 44.f, 2, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_DRAWER, 32.f * TILE, 4.f * TILE, 40.f, 34.f, 5, PROMPT_DRAWER, 0, IT_BANDAGE, false, true);
  addProp(r, PROP_NOTE, 20.f * TILE, 5.f * TILE, 26.f, 24.f, 2, PROMPT_READ_NOTE, CLUE_NURSE_MEMO, 0, false, true);
  addProp(r, PROP_LOCKER, 34.f * TILE, 16.f * TILE, 38.f, 52.f, 1, PROMPT_HIDE, 0, 0, true, true);
  addProp(r, PROP_LAMP, 18.f * TILE, 13.f * TILE, 30.f, 46.f, 2, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_WINDOW, 27.f * TILE, 2.f * TILE, 120.f, 30.f, 2, PROMPT_EXAMINE, 0, 0, false, true);

  addItem(r, IT_WARD_KEY, 11.f * TILE, 8.f * TILE, 1, 1);
  addItem(r, IT_SEDATIVE, 30.f * TILE, 6.f * TILE, 1, 1);
  addItem(r, IT_REVOLVER, 32.f * TILE, 8.f * TILE, 1, 1);
  addItem(r, IT_AMMO, 32.5f * TILE, 9.f * TILE, 6, 0);
  addItem(r, IT_BATTERY, 6.f * TILE, 4.f * TILE, 1, 1);

  addLight(r, 18.f * TILE, 13.f * TILE, 200.f, 0.62f, 0.4f, 0.95f, 0.82f, 0.62f);
  addLight(r, 34.f * TILE, 16.f * TILE, 160.f, 0.34f, 0.2f, 0.7f, 0.72f, 0.58f);
  addLight(r, 9.f * TILE, 6.f * TILE, 150.f, 0.3f, 0.15f, 0.5f, 0.6f, 0.62f);
}

void buildCloset(GameWorldManager& w) {
  RoomData& r = w.rooms[R_CLOSET];
  r.palette = 3;
  r.nameId = 4;
  r.floorIndex = 0;
  r.ambient = 6;
  r.music = 0;
  rect(r, 10, 3, 29, 18, T_WALL);
  rect(r, 11, 4, 28, 17, T_METAL);
  rect(r, 18, 4, 19, 17, T_VOID);

  addProp(r, PROP_GURNEY, 14.f * TILE, 7.f * TILE, 96.f, 30.f, 3, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_GURNEY, 25.f * TILE, 7.f * TILE, 96.f, 30.f, 4, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_NOTE, 16.f * TILE, 15.f * TILE, 26.f, 24.f, 3, PROMPT_READ_NOTE, CLUE_FUSE_MEMO, 0, false, true);
  addProp(r, PROP_BODY, 25.f * TILE, 14.f * TILE, 58.f, 30.f, 1, PROMPT_BODY, 0, 0, false, true);

  addItem(r, IT_CROWBAR, 13.f * TILE, 9.f * TILE, 1, 1);
  addItem(r, IT_FUSE_BLUE, 26.f * TILE, 9.f * TILE, 1, 1);
  addItem(r, IT_AMMO, 21.f * TILE, 14.f * TILE, 4, 0);

  addLight(r, 20.f * TILE, 10.f * TILE, 130.f, 0.25f, 0.9f, 0.75f, 0.45f, 0.35f);
}

void buildBoiler(GameWorldManager& w) {
  RoomData& r = w.rooms[R_BOILER];
  r.palette = 4;
  r.nameId = 5;
  r.floorIndex = -1;
  r.ambient = 8;
  r.music = 1;
  roomShell(r, T_WALL, T_STONE);
  rect(r, 10, 6, 11, 8, T_WALL);
  rect(r, 22, 6, 23, 8, T_WALL);
  rect(r, 16, 14, 17, 16, T_WALL);
  rect(r, 30, 12, 35, 12, T_METAL);
  rect(r, 3, 15, 8, 17, T_WATER);
  rect(r, 12, 10, 15, 11, T_RUBBLE);

  addProp(r, PROP_BREAKER, 5.f * TILE, 4.f * TILE, 46.f, 58.f, 0, PROMPT_BREAKER, 0, 0, false, true);
  addProp(r, PROP_VALVE, 30.f * TILE, 4.f * TILE, 30.f, 30.f, 0, PROMPT_VALVE, 0, 0, false, true);
  addProp(r, PROP_VALVE, 33.f * TILE, 9.f * TILE, 30.f, 30.f, 1, PROMPT_VALVE, 0, 0, false, true);
  addProp(r, PROP_VALVE, 28.f * TILE, 15.f * TILE, 30.f, 30.f, 2, PROMPT_VALVE, 0, 0, false, true);
  addProp(r, PROP_NOTE, 7.f * TILE, 16.f * TILE, 26.f, 24.f, 4, PROMPT_READ_NOTE, CLUE_VALVE_ORDER, 0, false, true);
  addProp(r, PROP_NOTE, 33.f * TILE, 17.f * TILE, 26.f, 24.f, 5, PROMPT_READ_NOTE, CLUE_BOILER_LOG, 0, false, true);
  addProp(r, PROP_CANDLE, 4.f * TILE, 10.f * TILE, 26.f, 40.f, 2, PROMPT_SAVE, 0, 0, false, true);
  addProp(r, PROP_BODY, 20.f * TILE, 18.f * TILE, 58.f, 28.f, 2, PROMPT_BODY, 0, 0, false, true);

  addItem(r, IT_VALVE_HANDLE, 19.f * TILE, 10.f * TILE, 1, 1);
  addItem(r, IT_AMMO, 34.f * TILE, 4.f * TILE, 4, 0);

  addLight(r, 4.f * TILE, 10.f * TILE, 160.f, 0.5f, 0.6f, 1.f, 0.5f, 0.26f);
  addLight(r, 30.f * TILE, 4.f * TILE, 150.f, 0.28f, 0.85f, 0.75f, 0.5f, 0.3f);
  addLight(r, 20.f * TILE, 10.f * TILE, 220.f, 0.14f, 0.95f, 0.6f, 0.35f, 0.3f);

  connect(w, R_BOILER, 30, 2, R_FLOOD, 2, 10, -1, 0, 3, 30.5f * TILE, 4.2f * TILE, 3.4f * TILE, 10.5f * TILE);
}

void buildFlood(GameWorldManager& w) {
  RoomData& r = w.rooms[R_FLOOD];
  r.palette = 5;
  r.nameId = 6;
  r.floorIndex = -2;
  r.ambient = 4;
  r.music = 4;
  roomShell(r, T_WALL, T_STONE);
  rect(r, 5, 3, 13, 18, T_WATER);
  rect(r, 20, 3, 25, 6, T_WATER);
  rect(r, 28, 12, 35, 18, T_WATER);
  rect(r, 15, 15, 18, 18, T_WATER);
  rect(r, 28, 7, 31, 9, T_RUBBLE);
  rect(r, 8, 9, 10, 10, T_BLOOD);

  addProp(r, PROP_BODY, 18.f * TILE, 10.f * TILE, 62.f, 30.f, 3, PROMPT_BODY, 0, 0, false, true);
  addProp(r, PROP_NOTE, 33.f * TILE, 4.f * TILE, 26.f, 24.f, 6, PROMPT_READ_NOTE, CLUE_FLOOD_SCRATCH, 0, false, true);
  addProp(r, PROP_SIGIL, 22.f * TILE, 14.f * TILE, 70.f, 70.f, 0, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 34.f * TILE, 5.f * TILE, 70.f, 70.f, 1, PROMPT_EXAMINE, 0, 0, false, true);

  addItem(r, IT_FUSE_RED, 23.f * TILE, 11.f * TILE, 1, 1);
  addItem(r, IT_AMMO, 8.f * TILE, 5.f * TILE, 6, 0);

  addLight(r, 18.f * TILE, 10.f * TILE, 180.f, 0.22f, 0.7f, 0.5f, 0.62f, 0.55f);
  addLight(r, 33.f * TILE, 4.f * TILE, 160.f, 0.18f, 0.4f, 0.55f, 0.6f, 0.65f);

  connect(w, R_FLOOD, 37, 10, R_EAST, 2, 11, 0, 1, 3, 35.4f * TILE, 10.5f * TILE, 3.2f * TILE, 11.5f * TILE);
}

void buildEast(GameWorldManager& w) {
  RoomData& r = w.rooms[R_EAST];
  r.palette = 6;
  r.nameId = 7;
  r.floorIndex = 0;
  r.ambient = 12;
  r.music = 2;
  roomShell(r, T_WALL, T_WOOD);
  rect(r, 3, 9, 36, 10, T_CARPET);
  rect(r, 12, 13, 20, 15, T_BLOOD);
  rect(r, 6, 7, 6, 7, T_RUBBLE);
  rect(r, 32, 7, 36, 8, T_RUBBLE);

  addProp(r, PROP_LOCKER, 6.f * TILE, 8.f * TILE, 38.f, 52.f, 2, PROMPT_HIDE, 0, 0, true, true);
  addProp(r, PROP_LOCKER, 29.f * TILE, 15.f * TILE, 38.f, 52.f, 3, PROMPT_HIDE, 0, 0, true, true);
  addProp(r, PROP_NOTE, 18.f * TILE, 15.f * TILE, 26.f, 24.f, 7, PROMPT_READ_NOTE, CLUE_DOCTOR_TAPE, 0, false, true);
  addProp(r, PROP_CANDLE, 24.f * TILE, 9.5f * TILE, 26.f, 40.f, 3, PROMPT_SAVE, 0, 0, false, true);
  addProp(r, PROP_BOARD, 30.f * TILE, 17.f * TILE, 64.f, 34.f, 1, PROMPT_LOCKED_DOOR, 0, 0, false, true);
  addProp(r, PROP_BODY, 34.f * TILE, 13.f * TILE, 60.f, 30.f, 4, PROMPT_BODY, 0, 0, false, true);

  addItem(r, IT_FUSE_BRASS, 12.f * TILE, 9.f * TILE, 1, 1);
  addItem(r, IT_BANDAGE, 27.f * TILE, 13.f * TILE, 1, 1);
  addItem(r, IT_AMMO, 33.f * TILE, 9.f * TILE, 6, 0);

  addLight(r, 24.f * TILE, 9.5f * TILE, 170.f, 0.45f, 0.5f, 1.f, 0.55f, 0.3f);
  addLight(r, 10.f * TILE, 8.f * TILE, 190.f, 0.2f, 0.85f, 0.65f, 0.32f, 0.28f);

  connect(w, R_EAST, 20, 6, R_CHAPEL, 20, 20, 0, 0, 2, 20.5f * TILE, 8.4f * TILE, 20.5f * TILE, 18.4f * TILE);
}

void buildChapel(GameWorldManager& w) {
  RoomData& r = w.rooms[R_CHAPEL];
  r.palette = 7;
  r.nameId = 8;
  r.floorIndex = 0;
  r.ambient = 22;
  r.music = 3;
  rect(r, 1, 1, 38, 20, T_WALL);
  rect(r, 3, 2, 36, 20, T_STONE);
  rect(r, 18, 4, 21, 18, T_CARPET);
  rect(r, 15, 19, 24, 19, T_CARPET);
  rect(r, 8, 3, 10, 5, T_RUBBLE);
  rect(r, 30, 16, 33, 18, T_BLOOD);

  // pews
  for (i32 i = 0; i < 4; ++i) {
    f32 y = (7.f + (f32)i * 3.f) * TILE;
    addProp(r, PROP_GURNEY, 13.f * TILE, y, 110.f, 34.f, 10 + i, PROMPT_EXAMINE, 0, 0, true, true);
    addProp(r, PROP_GURNEY, 27.f * TILE, y, 110.f, 34.f, 20 + i, PROMPT_EXAMINE, 0, 0, true, true);
  }
  addProp(r, PROP_ORGAN, 7.f * TILE, 4.f * TILE, 80.f, 64.f, 0, PROMPT_ORGAN, CLUE_CHAPEL_HYMN, 0, true, true);
  addProp(r, PROP_NOTE, 20.f * TILE, 6.f * TILE, 26.f, 24.f, 8, PROMPT_READ_NOTE, CLUE_CHAPEL_LECTERN, 0, false, true);
  addProp(r, PROP_NOTE, 14.f * TILE, 10.f * TILE, 26.f, 24.f, 9, PROMPT_READ_NOTE, CLUE_CODE_HALF_A, 0, false, true);
  addProp(r, PROP_NOTE, 26.f * TILE, 10.f * TILE, 26.f, 24.f, 10, PROMPT_READ_NOTE, CLUE_CODE_HALF_B, 0, false, true);
  addProp(r, PROP_CODELOCK, 18.f * TILE, 3.2f * TILE, 56.f, 46.f, 0, PROMPT_CODELOCK, 0, 0, false, true);
  addProp(r, PROP_RITUAL_CANDLE, 6.f * TILE, 17.f * TILE, 24.f, 40.f, 0, PROMPT_CANDLE, 0, 0, false, true);
  addProp(r, PROP_RITUAL_CANDLE, 33.f * TILE, 17.f * TILE, 24.f, 40.f, 1, PROMPT_CANDLE, 0, 0, false, true);
  addProp(r, PROP_RITUAL_CANDLE, 6.f * TILE, 7.f * TILE, 24.f, 40.f, 2, PROMPT_CANDLE, 0, 0, false, true);
  addProp(r, PROP_RITUAL_CANDLE, 33.f * TILE, 7.f * TILE, 24.f, 40.f, 3, PROMPT_CANDLE, 0, 0, false, true);
  addProp(r, PROP_CANDLE, 20.f * TILE, 16.f * TILE, 26.f, 40.f, 4, PROMPT_SAVE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 20.f * TILE, 2.6f * TILE, 90.f, 90.f, 2, PROMPT_EXAMINE, 0, 0, false, true);

  addItem(r, IT_AMMO, 10.f * TILE, 18.f * TILE, 4, 0);
  addItem(r, IT_BANDAGE, 25.f * TILE, 17.f * TILE, 1, 1);
  addItem(r, IT_MATCHES, 12.f * TILE, 16.f * TILE, 3, 1);

  addLight(r, 20.f * TILE, 5.f * TILE, 250.f, 0.5f, 0.35f, 1.f, 0.55f, 0.32f);
  addLight(r, 7.f * TILE, 4.f * TILE, 190.f, 0.35f, 0.25f, 0.9f, 0.75f, 0.45f);
  addLight(r, 20.f * TILE, 16.f * TILE, 170.f, 0.4f, 0.6f, 1.f, 0.52f, 0.28f);

  connect(w, R_CHAPEL, 20, 2, R_MORGUE, 20, 20, -3, 0, 2, 20.5f * TILE, 4.4f * TILE, 20.5f * TILE, 18.4f * TILE);
  connect(w, R_CHAPEL, 36, 11, R_SLEEP, 3, 11, IT_BRASS_KEY, 1, 3, 33.6f * TILE, 11.5f * TILE, 5.4f * TILE, 11.5f * TILE);
}

void buildMorgue(GameWorldManager& w) {
  RoomData& r = w.rooms[R_MORGUE];
  r.palette = 8;
  r.nameId = 9;
  r.floorIndex = -1;
  r.ambient = 9;
  r.music = 1;
  roomShell(r, T_WALL, T_TILE);
  rect(r, 4, 6, 35, 6, T_METAL);
  rect(r, 10, 14, 16, 16, T_BLOOD);
  rect(r, 24, 8, 26, 9, T_RUBBLE);

  addProp(r, PROP_DRAWER, 8.f * TILE, 4.6f * TILE, 46.f, 40.f, 0, PROMPT_DRAWER, CLUE_MORGUE_TAG, 0, false, true);
  addProp(r, PROP_DRAWER, 12.f * TILE, 4.6f * TILE, 46.f, 40.f, 1, PROMPT_DRAWER, CLUE_ILSE_LETTER, IT_BRASS_KEY, false, true);
  addProp(r, PROP_DRAWER, 16.f * TILE, 4.6f * TILE, 46.f, 40.f, 2, PROMPT_DRAWER, 0, IT_LOCKET, false, true);
  addProp(r, PROP_DRAWER, 20.f * TILE, 4.6f * TILE, 46.f, 40.f, 3, PROMPT_DRAWER, 0, IT_BANDAGE, false, true);
  addProp(r, PROP_GURNEY, 25.f * TILE, 12.f * TILE, 120.f, 50.f, 30, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_BODY, 25.f * TILE, 12.f * TILE, 70.f, 34.f, 5, PROMPT_BODY, 0, 0, false, true);
  addProp(r, PROP_LAMP, 31.f * TILE, 16.f * TILE, 30.f, 46.f, 3, PROMPT_EXAMINE, 0, 0, true, true);
  addProp(r, PROP_NOTE, 32.f * TILE, 6.f * TILE, 26.f, 24.f, 11, PROMPT_READ_NOTE, CLUE_SLEEP_CHAMBER, 0, false, true);

  addItem(r, IT_AMMO, 32.f * TILE, 14.f * TILE, 6, 0);

  addLight(r, 31.f * TILE, 16.f * TILE, 200.f, 0.42f, 0.35f, 0.85f, 0.85f, 0.72f);
  addLight(r, 8.f * TILE, 4.6f * TILE, 130.f, 0.2f, 0.2f, 0.6f, 0.7f, 0.7f);
  addLight(r, 20.f * TILE, 11.f * TILE, 160.f, 0.16f, 0.6f, 0.7f, 0.35f, 0.3f);
}

void buildSleep(GameWorldManager& w) {
  RoomData& r = w.rooms[R_SLEEP];
  r.palette = 9;
  r.nameId = 10;
  r.floorIndex = -3;
  r.ambient = 14;
  r.music = 5;
  rect(r, 1, 1, 38, 20, T_WALL);
  rect(r, 3, 2, 36, 20, T_STONE);
  rect(r, 13, 8, 26, 16, T_BLOOD);
  rect(r, 18, 4, 21, 7, T_CARPET);
  rect(r, 4, 17, 7, 19, T_RUBBLE);
  rect(r, 32, 4, 35, 6, T_RUBBLE);

  addProp(r, PROP_CRADLE, 20.f * TILE, 11.f * TILE, 96.f, 80.f, 0, PROMPT_CRADLE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 20.f * TILE, 4.f * TILE, 110.f, 110.f, 3, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 8.f * TILE, 8.f * TILE, 80.f, 80.f, 4, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 31.f * TILE, 8.f * TILE, 80.f, 80.f, 5, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 8.f * TILE, 15.f * TILE, 80.f, 80.f, 6, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_SIGIL, 31.f * TILE, 15.f * TILE, 80.f, 80.f, 7, PROMPT_EXAMINE, 0, 0, false, true);
  addProp(r, PROP_NOTE, 6.f * TILE, 4.f * TILE, 26.f, 24.f, 12, PROMPT_READ_NOTE, CLUE_SLEEP_CHAMBER, 0, false, true);
  addProp(r, PROP_CANDLE, 34.f * TILE, 18.f * TILE, 26.f, 40.f, 5, PROMPT_SAVE, 0, 0, false, true);

  addLight(r, 20.f * TILE, 11.f * TILE, 260.f, 0.5f, 0.5f, 0.9f, 0.18f, 0.22f);
  addLight(r, 8.f * TILE, 8.f * TILE, 150.f, 0.22f, 0.8f, 0.75f, 0.2f, 0.25f);
  addLight(r, 31.f * TILE, 15.f * TILE, 150.f, 0.2f, 0.9f, 0.7f, 0.18f, 0.24f);
}

// ------------------------------------------------------------------ spawns
static const EnemySpawnDef SPAWNS[] = {
  { E_CRAWLER, 28.f * TILE, 10.5f * TILE, 10.f * TILE, 10.5f * TILE, 34.f * TILE, 10.5f * TILE },
  { E_CRAWLER, 30.f * TILE, 10.5f * TILE, 10.f * TILE, 10.5f * TILE, 34.f * TILE, 10.5f * TILE },
  { E_HOUND,   25.f * TILE, 9.f * TILE,   8.f * TILE, 9.f * TILE,   34.f * TILE, 6.f * TILE },
  { E_CRAWLER, 12.f * TILE, 11.f * TILE, 6.f * TILE, 12.f * TILE,  30.f * TILE, 12.f * TILE },
  { E_HOUND,   8.f * TILE, 11.f * TILE,  5.f * TILE, 11.f * TILE,  12.f * TILE, 10.f * TILE },
  { E_CRAWLER, 24.f * TILE, 10.f * TILE, 8.f * TILE, 11.f * TILE,  24.f * TILE, 10.f * TILE },
  { E_HOUND,   26.f * TILE, 10.f * TILE, 9.f * TILE, 10.f * TILE,   26.f * TILE, 10.f * TILE },
  { E_HOUND,   20.f * TILE, 12.f * TILE, 20.f * TILE, 12.f * TILE,  26.f * TILE, 13.f * TILE }
};

struct SpawnRow { i32 room; i32 spawn; };

static const SpawnRow SPAWN_ROWS[] = {
  { R_CORRIDOR, 0 },
  { R_NURSE, 1 },
  { R_BOILER, 2 },
  { R_CHAPEL, 3 },
  { R_MORGUE, 4 },
  { R_EAST, 5 },
  { R_EAST, 6 },
  { R_EAST, 7 }
};

} // namespace

i32 enemySpawnsFor(i32 room, EnemySpawnDef* out, i32 maxCount) {
  i32 n = 0;
  const i32 rows = (i32)(sizeof(SPAWN_ROWS) / sizeof(SPAWN_ROWS[0]));
  const i32 defs = (i32)(sizeof(SPAWNS) / sizeof(SPAWNS[0]));
  for (i32 i = 0; i < rows && n < maxCount; ++i) {
    if (SPAWN_ROWS[i].room != room) continue;
    i32 s = SPAWN_ROWS[i].spawn;
    if (s < 0 || s >= defs) continue;
    out[n++] = SPAWNS[s];
  }
  return n;
}

void buildWorld(GameWorldManager& w) {
  w.graph.reset();
  w.doorTable.reset();

  for (i32 i = 0; i < MAX_ROOMS; ++i) roomClear(w.rooms[i]);

  // Room nodes: centre coordinates feed the in-game map screen.
  w.graph.addRoom(1, 120.f, 120.f, 0, 0);      // ward 3
  w.graph.addRoom(2, 420.f, 120.f, 1, 0);      // corridor
  w.graph.addRoom(3, 420.f, 20.f, 2, 0);       // nurse
  w.graph.addRoom(4, 420.f, 235.f, 3, 0);      // closet
  w.graph.addRoom(5, 640.f, 120.f, 4, -1);     // boiler
  w.graph.addRoom(6, 640.f, 20.f, 5, -2);      // flood
  w.graph.addRoom(7, 840.f, 20.f, 6, 0);       // east
  w.graph.addRoom(8, 840.f, 160.f, 7, 0);      // chapel
  w.graph.addRoom(9, 840.f, 60.f, 8, -1);      // morgue
  w.graph.addRoom(10, 1010.f, 160.f, 9, -3);   // sleep chamber

  buildWard3(w);
  buildCorridor(w);
  buildNurse(w);
  buildCloset(w);
  buildBoiler(w);
  buildFlood(w);
  buildEast(w);
  buildChapel(w);
  buildMorgue(w);
  buildSleep(w);

  for (i32 i = 0; i < ROOM_COUNT; ++i) decorPass(w.rooms[i], i + 3);
  w.graph.finalize();
}

} // namespace tcs
