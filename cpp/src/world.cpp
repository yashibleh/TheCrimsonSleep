#include "GameWorldManager.hpp"
#include "levels.hpp"
#include "Messages.hpp"

namespace tcs {

GameWorldManager g_world;

// ------------------------------------------------------------------ helpers

void GameWorldManager::pushEvent(i32 type, f32 x, f32 y, f32 a, f32 b) {
  if (eventCount >= MAX_EVENTS) {
    for (i32 i = 1; i < MAX_EVENTS; ++i) events[i - 1] = events[i];
    eventCount = MAX_EVENTS - 1;
  }
  GameEvent& e = events[eventCount++];
  e.type = (f32)type;
  e.x = x;
  e.y = y;
  e.a = a;
  e.b = b;
}

void GameWorldManager::notify(i32 message) {
  messageId = message;
  messageTimer = 3.4f;
}

void GameWorldManager::pushNoise(f32 x, f32 y, f32 intensity, i32 kind) {
  sounds.push(x, y, intensity, (f32)kind);
  pushEvent(EV_SFX, x, y, 20.f + (f32)kind, intensity);
}

i32 GameWorldManager::roomTileAt(i32 roomIdx, i32 cx, i32 cy) const {
  if (roomIdx < 0 || roomIdx >= MAX_ROOMS) return T_VOID;
  if (cx < 0 || cy < 0 || cx >= ROOM_W || cy >= ROOM_H) return T_VOID;
  return rooms[roomIdx].tiles[cy * ROOM_W + cx];
}

bool GameWorldManager::tileBlocked(i32 roomIdx, i32 cx, i32 cy) const {
  if (roomIdx < 0 || roomIdx >= MAX_ROOMS) return true;
  if (cx < 0 || cy < 0 || cx >= ROOM_W || cy >= ROOM_H) return true;
  return rooms[roomIdx].navBlocked[cy * ROOM_W + cx] != 0;
}

bool GameWorldManager::pointBlocked(i32 roomIdx, f32 x, f32 y) const {
  i32 cx = (i32)(x / (f32)TILE);
  i32 cy = (i32)(y / (f32)TILE);
  return tileBlocked(roomIdx, cx, cy);
}

void GameWorldManager::moveCircle(i32 roomIdx, f32& x, f32& y, f32 dx, f32 dy, f32 radius) {
  f32 nx = x + dx;
  if (!pointBlocked(roomIdx, nx - radius, y - radius) &&
      !pointBlocked(roomIdx, nx + radius, y - radius) &&
      !pointBlocked(roomIdx, nx - radius, y + radius) &&
      !pointBlocked(roomIdx, nx + radius, y + radius) &&
      !pointBlocked(roomIdx, nx, y)) {
    x = nx;
  }
  f32 ny = y + dy;
  if (!pointBlocked(roomIdx, x - radius, ny - radius) &&
      !pointBlocked(roomIdx, x + radius, ny - radius) &&
      !pointBlocked(roomIdx, x - radius, ny + radius) &&
      !pointBlocked(roomIdx, x + radius, ny + radius) &&
      !pointBlocked(roomIdx, x, ny)) {
    y = ny;
  }
}

void GameWorldManager::rebuildNav() {
  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    RoomData& r = rooms[rm];
    for (i32 i = 0; i < CELLS; ++i) r.navBlocked[i] = tileSolid(r.tiles[i]) ? 1 : 0;

    for (i32 p = 0; p < r.propCount; ++p) {
      RoomProp& prop = r.props[p];
      if (!prop.solid) continue;
      f32 hw = prop.w * 0.5f, hh = prop.h * 0.5f;
      i32 x0 = (i32)((prop.x - hw) / (f32)TILE);
      i32 x1 = (i32)((prop.x + hw) / (f32)TILE);
      i32 y0 = (i32)((prop.y - hh) / (f32)TILE);
      i32 y1 = (i32)((prop.y + hh) / (f32)TILE);
      for (i32 cy = y0; cy <= y1; ++cy) {
        for (i32 cx = x0; cx <= x1; ++cx) {
          if (cx < 0 || cy < 0 || cx >= ROOM_W || cy >= ROOM_H) continue;
          r.navBlocked[cy * ROOM_W + cx] = 1;
        }
      }
    }

    for (i32 d = 0; d < r.doorCount; ++d) {
      RoomDoor& door = r.doors[d];
      if (doorTable.unlocked[door.id]) continue;
      i32 cx = (i32)(door.x / (f32)TILE);
      i32 cy = (i32)(door.y / (f32)TILE);
      if (cx >= 0 && cy >= 0 && cx < ROOM_W && cy < ROOM_H) {
        r.navBlocked[cy * ROOM_W + cx] = 1;
      }
    }
  }
}

// ------------------------------------------------------------------ lifecycle

void GameWorldManager::init() {
  rng.seed(0xC0FFEEu);
  player.reset();
  inv.reset();
  clues.reset();
  actions.reset();
  sounds.reset();
  sounds.clock = 0.f;
  enemies.reset();
  nav.reset();
  doorTable.reset();

  for (i32 i = 0; i < MAX_ROOMS; ++i) visited[i] = 0;
  for (i32 i = 0; i < 16; ++i) storyFlags[i] = 0;
  for (i32 i = 0; i < MAX_EVENTS; ++i) events[i].type = EV_NONE;

  room = R_WARD3;
  tick = 0.f;
  roomTime = 0.f;
  state = GS_PLAYING;
  endingId = 0;
  objective = 1;
  objectiveProgress = 0;
  danger = 0.f;
  chaseHeat = 0.f;
  valveProgress = 0;
  valveOrder[0] = 2; valveOrder[1] = 0; valveOrder[2] = 1;
  fuseMask = 0;
  codeDigits[0] = 0; codeDigits[1] = 0; codeDigits[2] = 0; codeDigits[3] = 0;
  codeProgress = 0;
  codeSolved = 0;
  candleProgress = 0;
  candleOrder[0] = 3; candleOrder[1] = 0; candleOrder[2] = 1; candleOrder[3] = 2;
  weaverPhase = 0;
  prompt.id = PROMPT_NONE;
  prompt.target = -1;
  prompt.labelId = 0;
  prompt.x = 0.f; prompt.y = 0.f;
  messageId = 0;
  messageTimer = 0.f;
  subtitleId = 0;
  subtitleTimer = 0.f;
  eventCount = 0;
  flashTimer = 0.f;
  shakeTimer = 0.f;
  shakeStrength = 0.f;
  deaths = 0;
  checkpoints = 0;
  totalKills = 0;
  secretCount = 0;
  saveSize = 0;

  buildWorld(*this);
  rebuildNav();

  player.x = 10.5f * TILE;
  player.y = 12.5f * TILE;
  player.room = R_WARD3;
  player.dirX = 1.f;
  player.dirY = 0.f;
  player.setFlag(PF_FLASHLIGHT_ON, true);
  player.stats[STAT_AMMO_IN_MAG] = 0.f;

  loadRoom(R_WARD3, player.x, player.y, false);
  notify(MSG_START);
  pushEvent(EV_TEXT, 0.f, 0.f, (f32)MSG_START, 0.f);
}

void GameWorldManager::newGame() { init(); }

void GameWorldManager::loadRoom(i32 id, f32 spawnX, f32 spawnY, bool preserveEnemies) {
  if (id < 0 || id >= ROOM_COUNT) return;
  i32 previous = room;
  room = id;
  roomTime = 0.f;
  visited[id] = 1;
  player.x = spawnX;
  player.y = spawnY;
  player.room = id;
  player.vx = 0.f;
  player.vy = 0.f;
  player.hideProp = -1;
  player.setFlag(PF_HIDING, false);

  if (!preserveEnemies) {
    for (i32 i = 0; i < enemies.count; ++i) {
      if (enemies.list[i].alive && enemies.list[i].room != id) enemies.list[i].alive = 0;
    }
  }

  // spawn this room's authored enemies when none are left alive here
  if (enemies.aliveInRoom(id) == 0) {
    EnemySpawnDef defs[8];
    i32 n = enemySpawnsFor(id, defs, 8);
    for (i32 i = 0; i < n; ++i) {
      i32 idx = enemies.spawn(defs[i].type, id, defs[i].x, defs[i].y);
      if (idx >= 0) {
        enemies.addAnchor(enemies.list[idx], defs[i].ax, defs[i].ay);
        enemies.addAnchor(enemies.list[idx], defs[i].bx, defs[i].by);
      }
    }
  }

  rebuildNav();
  refreshReachability();
  sounds.clear();
  pushEvent(EV_ROOM, 0.f, 0.f, (f32)id, (f32)previous);
}

// ------------------------------------------------------------------ update

void GameWorldManager::update(f32 dt, i32 input) {
  if (dt > 0.05f) dt = 0.05f;
  if (dt <= 0.f) return;
  tick += dt;
  roomTime += dt;
  sounds.clock = tick;

  if (state == GS_PLAYING) {
    updatePlayer(dt, input);
    enemies.updateAll(*this, dt);
    updatePrompt();
    if (input & IN_INTERACT) interact();
    updateTriggers();
    updateScripted(dt);
    updateAssetBobbing(dt);
    danger = proximityThreat();
    if (player.flags[PF_DEAD]) {
      state = GS_DEAD;
      deaths++;
      pushEvent(EV_DEATH, player.x, player.y, 0.f, 0.f);
    }
  }

  if (messageTimer > 0.f) messageTimer -= dt;
  if (subtitleTimer > 0.f) subtitleTimer -= dt;
  if (shakeTimer > 0.f) shakeTimer -= dt;
  if (flashTimer > 0.f) flashTimer -= dt;
  sounds.expire(5.f);
  objective = computeObjective();
}

void GameWorldManager::updateAssetBobbing(f32 dt) {
  RoomData& r = rooms[room];
  for (i32 i = 0; i < r.itemCount; ++i) {
    r.items[i].bob += dt * 2.1f;
    if (r.items[i].bob > 6.28318f) r.items[i].bob -= 6.28318f;
  }
}

void GameWorldManager::updatePlayer(f32 dt, i32 input) {
  PlayerState& p = player;
  p.animTime += dt;
  if (p.attackCooldown > 0.f) p.attackCooldown -= dt;
  if (p.attackTimer > 0.f) p.attackTimer -= dt;
  if (p.stats[STAT_INVULN] > 0.f) p.stats[STAT_INVULN] -= dt;
  if (p.stats[STAT_DAMAGE_FLASH] > 0.f) p.stats[STAT_DAMAGE_FLASH] -= dt * 2.2f;
  if (p.stats[STAT_DAMAGE_FLASH] < 0.f) p.stats[STAT_DAMAGE_FLASH] = 0.f;

  if (input & IN_LIGHT) {
    bool on = p.hasFlag(PF_FLASHLIGHT_ON);
    if (!on && p.stats[STAT_BATTERY] <= 0.5f) {
      notify(MSG_BATTERY_DEAD);
    } else {
      p.setFlag(PF_FLASHLIGHT_ON, !on);
      pushNoise(p.x, p.y, 0.10f, 2);
      pushEvent(EV_SFX, p.x, p.y, 6.f, 0.5f);
    }
  }
  if (input & IN_SWAP) swapWeapon();
  if (input & IN_RELOAD) tryReload();
  if (input & IN_HEAL) useHeal();
  if ((input & IN_ATTACK) && p.attackCooldown <= 0.f && !p.hasFlag(PF_HIDING)) doAttack();

  if (p.reloadTimer > 0.f) {
    p.reloadTimer -= dt;
    if (p.reloadTimer <= 0.f) {
      i32 need = (i32)p.stats[STAT_AMMO_MAX_MAG] - (i32)p.stats[STAT_AMMO_IN_MAG];
      i32 have = inv.quantity(IT_AMMO);
      i32 take = need < have ? need : have;
      if (take > 0) {
        inv.remove(IT_AMMO, take);
        p.stats[STAT_AMMO_IN_MAG] += (f32)take;
        pushEvent(EV_SFX, p.x, p.y, 9.f, 0.6f);
      }
    }
    return;
  }

  if (p.hasFlag(PF_HIDING)) {
    p.vx = 0.f;
    p.vy = 0.f;
    p.stats[STAT_NOISE] = 0.f;
    return;
  }

  f32 ix = ((input & IN_RIGHT) ? 1.f : 0.f) - ((input & IN_LEFT) ? 1.f : 0.f);
  f32 iy = ((input & IN_DOWN) ? 1.f : 0.f) - ((input & IN_UP) ? 1.f : 0.f);
  f32 len = sqrtf_(ix * ix + iy * iy);
  bool moving = len > 0.01f;
  if (moving) { ix /= len; iy /= len; }

  bool wantSprint = (input & IN_SPRINT) && moving && p.stats[STAT_STAMINA] > 2.f;
  p.setFlag(PF_SPRINTING, wantSprint);
  f32 speed = wantSprint ? p.stats[STAT_RUN_SPEED] : p.stats[STAT_MOVE_SPEED];

  i32 tileUnder = roomTileAt(room, (i32)(p.x / TILE), (i32)(p.y / TILE));
  if (tileSlow((u8)tileUnder)) speed *= 0.66f;

  f32 wantX = ix * speed;
  f32 wantY = iy * speed;
  f32 accel = moving ? 1500.f : 1900.f;
  p.vx = approachf(p.vx, wantX, accel * dt);
  p.vy = approachf(p.vy, wantY, accel * dt);

  f32 oldX = p.x, oldY = p.y;
  moveCircle(room, p.x, p.y, p.vx * dt, p.vy * dt, 10.f);
  f32 movedDist = distf(oldX, oldY, p.x, p.y);
  if (movedDist < 0.01f) { p.vx *= 0.4f; p.vy *= 0.4f; }

  if (moving) {
    p.dirX = lerpf(p.dirX, ix, minf(1.f, dt * 16.f));
    p.dirY = lerpf(p.dirY, iy, minf(1.f, dt * 16.f));
    f32 dl = sqrtf_(p.dirX * p.dirX + p.dirY * p.dirY);
    if (dl > 0.001f) { p.dirX /= dl; p.dirY /= dl; }
    p.walkPhase += movedDist * 0.055f;
  }

  // stamina
  if (wantSprint) {
    p.stats[STAT_STAMINA] -= 24.f * dt;
    if (p.stats[STAT_STAMINA] < 0.f) p.stats[STAT_STAMINA] = 0.f;
  } else {
    p.stats[STAT_STAMINA] += 16.f * dt;
    if (p.stats[STAT_STAMINA] > p.stats[STAT_STAMINA_MAX]) p.stats[STAT_STAMINA] = p.stats[STAT_STAMINA_MAX];
  }

  // flashlight battery
  if (p.hasFlag(PF_FLASHLIGHT_ON)) {
    p.stats[STAT_BATTERY] -= 0.42f * dt;
    if (p.stats[STAT_BATTERY] <= 0.f) {
      p.stats[STAT_BATTERY] = 0.f;
      p.setFlag(PF_FLASHLIGHT_ON, false);
      notify(MSG_BATTERY_DEAD);
    }
  }

  // footsteps & the noise the player makes
  f32 noise = moving ? (wantSprint ? 0.62f : 0.24f) : 0.f;
  if (tileSlow((u8)tileUnder) && moving) noise += 0.2f;
  p.stats[STAT_NOISE] = noise;
  if (moving && !wantSprint) {
    p.footstepTimer -= dt;
    if (p.footstepTimer <= 0.f) {
      p.footstepTimer = 0.46f;
      pushNoise(p.x, p.y, noise * 0.55f, 0);
      pushEvent(EV_SFX, p.x, p.y, 1.f, 0.35f);
    }
  }
  if (wantSprint) {
    p.footstepTimer -= dt;
    if (p.footstepTimer <= 0.f) {
      p.footstepTimer = 0.28f;
      pushNoise(p.x, p.y, noise, 0);
      pushEvent(EV_SFX, p.x, p.y, 2.f, 0.55f);
    }
  }
  if (p.stats[STAT_HP] < 40.f) {
    p.heartbeat += dt * (1.2f + (40.f - p.stats[STAT_HP]) * 0.03f);
  }
  chaseHeat = maxf(0.f, chaseHeat - dt * 0.35f);
}

void GameWorldManager::updatePrompt() {
  prompt.id = PROMPT_NONE;
  prompt.target = -1;
  prompt.labelId = 0;
  prompt.x = player.x;
  prompt.y = player.y;

  if (player.hasFlag(PF_HIDING)) {
    prompt.id = PROMPT_LEAVE_HIDE;
    prompt.labelId = PROMPT_LEAVE_HIDE;
    prompt.target = player.hideProp;
    return;
  }

  // items need a close reach; large props (cradle, breaker, organ) publish their own
  const f32 kItemReach = 44.f;
  f32 best = 96.f * 96.f;
  RoomData& r = rooms[room];

  for (i32 i = 0; i < r.itemCount; ++i) {
    if (r.items[i].taken) continue;
    f32 d = dist2(player.x, player.y, r.items[i].x, r.items[i].y);
    if (d < kItemReach * kItemReach && d < best) {
      best = d;
      prompt.id = PROMPT_TAKE;
      prompt.labelId = PROMPT_TAKE;
      prompt.target = -1000 - i;
      prompt.x = r.items[i].x;
      prompt.y = r.items[i].y;
    }
  }

  for (i32 i = 0; i < r.propCount; ++i) {
    RoomProp& p = r.props[i];
    if (!p.interactive) continue;
    f32 reach = maxf(p.w, p.h) * 0.5f + 26.f;
    f32 d = dist2(player.x, player.y, p.x, p.y);
    if (d < reach * reach && d < best) {
      best = d;
      prompt.id = p.labelId;
      prompt.labelId = p.labelId;
      prompt.target = i;
      prompt.x = p.x;
      prompt.y = p.y;
    }
  }

  for (i32 i = 0; i < r.doorCount; ++i) {
    RoomDoor& d0 = r.doors[i];
    f32 d = dist2(player.x, player.y, d0.x, d0.y);
    if (d < best && d < 48.f * 48.f) {
      best = d;
      prompt.id = doorTable.unlocked[d0.id] ? PROMPT_USE_DOOR : PROMPT_LOCKED_DOOR;
      prompt.labelId = prompt.id;
      prompt.target = i;
      prompt.x = d0.x;
      prompt.y = d0.y;
    }
  }
}

// DFS over the room graph: the rooms the player can actually reach right now.
// Locked doors remove edges, so this is the reachability the Weaver pursues by.
void GameWorldManager::refreshReachability() {
  for (i32 i = 0; i < MAX_ROOMS; ++i) reachMask[i] = 0;
  reachCount = 0;
  if (room < 0 || room >= graph.nodeCount) return;
  DFSResult res;
  dfs.explore(graph, doorTable, room, res);
  if (res.count > MAX_ROOMS) res.count = MAX_ROOMS;
  for (i32 i = 0; i < res.count; ++i) {
    i32 rm = res.order[i];
    if (rm < 0 || rm >= MAX_ROOMS) continue;
    if (!reachMask[rm]) { reachMask[rm] = 1; reachCount++; }
  }
  if (!reachMask[room]) { reachMask[room] = 1; reachCount++; }
}

// BFS next hop along the shortest open-door route from `from` towards `to`.
i32 GameWorldManager::nextRoomToward(i32 from, i32 to) {
  if (from == to) return to;
  if (from < 0 || to < 0 || from >= MAX_ROOMS || to >= MAX_ROOMS) return -1;
  BFSResult res;
  bfs.explore(graph, doorTable, from, res);
  if (res.dist[to] < 0) return -1;
  i32 cur = to;
  i32 guard = 0;
  while (res.parent[cur] >= 0 && res.parent[cur] != from && guard++ < MAX_ROOMS) cur = res.parent[cur];
  return cur;
}

void GameWorldManager::updateTriggers() {
  if (room == R_WARD3 && !storyFlags[0]) {
    storyFlags[0] = 1;
    subtitleId = SUB_WAKE;
    subtitleTimer = 6.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_WAKE, 0.f);
  } else if (room == R_CORRIDOR && !storyFlags[1]) {
    storyFlags[1] = 1;
    subtitleId = SUB_CORRIDOR;
    subtitleTimer = 6.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_CORRIDOR, 0.f);
    pushEvent(EV_SFX, 0.f, 0.f, 14.f, 0.5f);  // distant bell
  } else if (room == R_FLOOD && !storyFlags[2]) {
    storyFlags[2] = 1;
    subtitleId = SUB_FLOOD;
    subtitleTimer = 6.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_FLOOD, 0.f);
    enemies.awakeWeaver(*this, room, 34.f * TILE, 4.f * TILE);
    weaverPhase = 1;
    pushEvent(EV_SHAKE, 0.f, 0.f, 12.f, 1.1f);
    pushEvent(EV_SFX, 0.f, 0.f, 30.f, 1.f);
  } else if (room == R_EAST && !storyFlags[3]) {
    storyFlags[3] = 1;
    subtitleId = SUB_EAST;
    subtitleTimer = 5.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_EAST, 0.f);
    pushEvent(EV_SHAKE, 0.f, 0.f, 8.f, 0.8f);
    pushEvent(EV_SFX, 0.f, 0.f, 15.f, 0.8f);
  } else if (room == R_MORGUE && !storyFlags[4]) {
    storyFlags[4] = 1;
    subtitleId = SUB_MORGUE;
    subtitleTimer = 5.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_MORGUE, 0.f);
  } else if (room == R_SLEEP && !storyFlags[5]) {
    storyFlags[5] = 1;
    subtitleId = SUB_SLEEP;
    subtitleTimer = 6.f;
    pushEvent(EV_QUEST, 0.f, 0.f, (f32)SUB_SLEEP, 0.f);
    pushEvent(EV_SFX, 0.f, 0.f, 31.f, 0.9f);
  }
}

void GameWorldManager::updateScripted(f32 dt) {
  (void)dt;
  // The Weaver is not teleported: EnemyController walks it room to room along the
  // BFS route through doors that are actually open (see updateAll).
}

f32 GameWorldManager::proximityThreat() const {
  f32 best = 1e9f;
  for (i32 i = 0; i < enemies.count; ++i) {
    const Enemy& e = enemies.list[i];
    if (!e.alive || e.room != room) continue;
    f32 d = distf(player.x, player.y, e.x, e.y);
    f32 local = d;
    if (e.state == ES_CHASE) local -= 140.f;
    if (e.state == ES_ATTACK) local -= 200.f;
    if (e.type == E_WEAVER) local -= 60.f;
    if (local < best) best = local;
  }
  if (best > 900.f) return 0.f;
  f32 t = 1.f - clampf(best / 640.f, 0.f, 1.f);
  return clampf(t, 0.f, 1.f);
}

i32 GameWorldManager::computeObjective() const {
  if (state == GS_ENDING) return 99;
  if (!inv.find(IT_CROWBAR)) return 1;
  if (!player.hasFlag(PF_POWER_ON)) return 2;
  if (!codeSolved) return 3;
  if (!inv.find(IT_LOCKET)) return 4;
  if (!inv.find(IT_BRASS_KEY)) return 5;
  return 6;
}

i32 GameWorldManager::promptLabelForProp(const RoomProp& p) const {
  return p.labelId;
}

} // namespace tcs
