#include "GameWorldManager.hpp"
#include "Messages.hpp"

namespace tcs {

void GameWorldManager::addClue(i32 clueId, i32 roomId) {
  if (clueId <= 0) return;
  if (clues.insert(clueId, roomId)) {
    pushEvent(EV_CLUE, player.x, player.y, (f32)clueId, 0.f);
    pushEvent(EV_SFX, player.x, player.y, 7.f, 0.4f);
  }
}

// ---------------------------------------------------------------- doors
static void setDoorOpen(GameWorldManager& w, i32 doorId) {
  if (doorId < 0 || doorId >= MAX_ROOMS * MAX_ROOM_DOORS) return;
  w.doorTable.unlocked[doorId] = 1;
  i32 rm = doorId / MAX_ROOM_DOORS;
  i32 idx = doorId % MAX_ROOM_DOORS;
  if (rm >= 0 && rm < MAX_ROOMS && idx < w.rooms[rm].doorCount) {
    w.rooms[rm].doors[idx].open = 1;
    w.rooms[rm].doors[idx].locked = 0;
  }
}

static void unlockRequirement(GameWorldManager& w, i32 requirement) {
  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    for (i32 d = 0; d < w.rooms[rm].doorCount; ++d) {
      RoomDoor& door = w.rooms[rm].doors[d];
      if (door.keyItem != requirement) continue;
      setDoorOpen(w, door.id);
      setDoorOpen(w, door.linkId);
    }
  }
  w.rebuildNav();
  w.refreshReachability();
}

void GameWorldManager::unlockDoor(i32 doorIdx) {
  RoomData& r = rooms[room];
  if (doorIdx < 0 || doorIdx >= r.doorCount) return;
  RoomDoor& d = r.doors[doorIdx];

  bool can = false;
  if (d.keyItem == 0) can = true;
  else if (d.keyItem > 0) can = inv.find(d.keyItem) != 0;
  else if (d.keyItem == -1) can = player.hasFlag(PF_VALVES_DONE);
  else if (d.keyItem == -2) can = player.hasFlag(PF_POWER_ON);
  else if (d.keyItem == -3) can = codeSolved != 0;

  if (!can) {
    if (d.keyItem == -9) notify(MSG_DOOR_FOREVER);
    else if (d.keyItem == -1) notify(MSG_DOOR_VALVES);
    else if (d.keyItem == -2) notify(MSG_DOOR_POWER);
    else if (d.keyItem == -3) notify(MSG_DOOR_CODE);
    else notify(MSG_DOOR_NEED_KEY);
    pushEvent(EV_SFX, d.x, d.y, 11.f, 0.6f);
    return;
  }
  setDoorOpen(*this, d.id);
  setDoorOpen(*this, d.linkId);
  rebuildNav();
  refreshReachability();
  pushEvent(EV_SFX, d.x, d.y, 5.f, 0.8f);
}

void GameWorldManager::travelThroughDoor(i32 doorIdx) {
  RoomData& r = rooms[room];
  if (doorIdx < 0 || doorIdx >= r.doorCount) return;
  RoomDoor& d = r.doors[doorIdx];
  i32 from = room;
  i32 target = d.targetRoom;
  f32 sx = d.spawnX;
  f32 sy = d.spawnY;

  // anyone actively hunting follows through the world graph
  i32 spawnTileX = (i32)(d.x / (f32)TILE);
  i32 spawnTileY = (i32)(d.y / (f32)TILE);
  (void)spawnTileX; (void)spawnTileY;
  enemies.transferRoom(from, target, sx, sy);

  loadRoom(target, sx, sy, true);
  pushEvent(EV_SFX, sx, sy, 5.f, 0.75f);
  pushEvent(EV_TRANSITION, sx, sy, (f32)from, (f32)target);
}

// ---------------------------------------------------------------- items
void GameWorldManager::takeItem(i32 roomIdx, i32 itemIdx) {
  if (roomIdx < 0 || roomIdx >= MAX_ROOMS) return;
  RoomData& r = rooms[roomIdx];
  if (itemIdx < 0 || itemIdx >= r.itemCount) return;
  RoomItem& it = r.items[itemIdx];
  if (it.taken) return;

  i32 added = inv.insert(it.itemId, it.qty);
  if (added <= 0) { notify(MSG_INV_FULL); return; }
  it.taken = 1;
  pushEvent(EV_PICKUP, it.x, it.y, (f32)it.itemId, (f32)added);
  pushEvent(EV_SFX, it.x, it.y, 7.f, 0.5f);

  if (it.itemId == IT_CROWBAR) {
    player.setFlag(PF_HAS_CROWBAR, true);
    player.weapon = 1;
  } else if (it.itemId == IT_REVOLVER) {
    player.setFlag(PF_HAS_REVOLVER, true);
    player.weapon = 2;
    if (player.stats[STAT_AMMO_IN_MAG] <= 0.f) player.stats[STAT_AMMO_IN_MAG] = 6.f;
  } else if (it.itemId == IT_BATTERY) {
    player.stats[STAT_BATTERY] = minf(player.stats[STAT_BATTERY_MAX], player.stats[STAT_BATTERY] + 55.f);
  }
}

void GameWorldManager::useItemById(i32 itemId) {
  if (itemId == IT_BANDAGE) { useHeal(); return; }
  if (itemId == IT_BATTERY) {
    if (inv.quantity(IT_BATTERY) <= 0) return;
    inv.remove(IT_BATTERY, 1);
    player.stats[STAT_BATTERY] = minf(player.stats[STAT_BATTERY_MAX], player.stats[STAT_BATTERY] + 60.f);
    pushEvent(EV_SFX, player.x, player.y, 7.f, 0.5f);
    return;
  }
  if (itemId == IT_MATCHES) {
    pushEvent(EV_SFX, player.x, player.y, 13.f, 0.6f);
    return;
  }
  if (itemId == IT_SEDATIVE) {
    notify(MSG_CRADLE_NEED);
    return;
  }
}

void GameWorldManager::useHeal() {
  if (player.stats[STAT_HP] >= player.stats[STAT_HP_MAX]) { notify(MSG_NO_HEAL); return; }
  if (inv.quantity(IT_BANDAGE) <= 0) { notify(MSG_NO_HEAL); return; }
  inv.remove(IT_BANDAGE, 1);
  player.stats[STAT_HP] = minf(player.stats[STAT_HP_MAX], player.stats[STAT_HP] + 38.f);
  notify(MSG_HEAL);
  pushEvent(EV_SFX, player.x, player.y, 8.f, 0.5f);
}

void GameWorldManager::swapWeapon() {
  if (!player.hasFlag(PF_HAS_REVOLVER)) { notify(MSG_NO_SWAP); return; }
  player.weapon = player.weapon == 2 ? 1 : 2;
  pushEvent(EV_SFX, player.x, player.y, 9.f, 0.4f);
}

void GameWorldManager::tryReload() {
  if (player.weapon != 2) return;
  if (player.stats[STAT_AMMO_IN_MAG] >= player.stats[STAT_AMMO_MAX_MAG]) return;
  if (inv.quantity(IT_AMMO) <= 0) { notify(MSG_NO_AMMO); return; }
  if (player.reloadTimer > 0.f) return;
  player.reloadTimer = 1.5f;
  notify(MSG_RELOAD);
  pushEvent(EV_SFX, player.x, player.y, 9.f, 0.6f);
}

// ---------------------------------------------------------------- combat
void GameWorldManager::doAttack() {
  PlayerState& p = player;
  p.attackTimer = 0.3f;
  if (p.weapon == 2) { fireRevolver(); p.attackCooldown = 0.55f; }
  else { meleeSwing(); p.attackCooldown = 0.46f; }
}

void GameWorldManager::fireRevolver() {
  PlayerState& p = player;
  if (!p.hasFlag(PF_HAS_REVOLVER)) { notify(MSG_NO_WEAPON); return; }
  if (p.stats[STAT_AMMO_IN_MAG] <= 0.f) {
    notify(MSG_EMPTY_GUN);
    pushEvent(EV_SFX, p.x, p.y, 10.f, 0.35f);
    pushNoise(p.x, p.y, 0.2f, 1);
    return;
  }
  p.stats[STAT_AMMO_IN_MAG] -= 1.f;
  p.attackTimer = 0.2f;
  pushEvent(EV_SFX, p.x, p.y, 3.f, 1.f);
  pushEvent(EV_FLASH, p.x, p.y, 0.35f, 2.f);
  pushEvent(EV_SHAKE, 0.f, 0.f, 6.f, 0.35f);
  pushNoise(p.x, p.y, 1.0f, 1);

  i32 best = -1;
  f32 bestD = 1e9f;
  for (i32 i = 0; i < enemies.count; ++i) {
    Enemy& e = enemies.list[i];
    if (!e.alive || e.room != room) continue;
    f32 d = distf(p.x, p.y, e.x, e.y);
    if (d > 560.f) continue;
    f32 dx = (e.x - p.x) / (d > 0.1f ? d : 1.f);
    f32 dy = (e.y - p.y) / (d > 0.1f ? d : 1.f);
    if (dx * p.dirX + dy * p.dirY < 0.80f) continue;
    if (!nav.losClear(rooms[room].navBlocked, p.x, p.y, e.x, e.y)) continue;
    if (d < bestD) { bestD = d; best = i; }
  }
  if (best >= 0) {
    Enemy& e = enemies.list[best];
    f32 dmg = e.type == E_WEAVER ? 0.f : 26.f;
    enemies.hurt(*this, e.x, e.y, 12.f, dmg, p.x, p.y);
    if (e.type == E_WEAVER) pushEvent(EV_TEXT, e.x, e.y, (f32)MSG_WEAVER_BLOCK, 0.f);
  }
  enemies.alertAll(room, p.x, p.y, 65.f);
}

void GameWorldManager::meleeSwing() {
  PlayerState& p = player;
  pushEvent(EV_SFX, p.x, p.y, 4.f, 0.7f);
  pushNoise(p.x, p.y, 0.5f, 3);
  bool hit = false;
  for (i32 i = 0; i < enemies.count; ++i) {
    Enemy& e = enemies.list[i];
    if (!e.alive || e.room != room) continue;
    f32 d = distf(p.x, p.y, e.x, e.y);
    if (d > 64.f) continue;
    f32 dx = (e.x - p.x) / (d > 0.1f ? d : 1.f);
    f32 dy = (e.y - p.y) / (d > 0.1f ? d : 1.f);
    if (dx * p.dirX + dy * p.dirY < 0.25f) continue;
    enemies.hurt(*this, e.x, e.y, 20.f, 17.f, p.x, p.y);
    hit = true;
  }
  if (hit) {
    pushEvent(EV_SHAKE, 0.f, 0.f, 5.f, 0.25f);
    pushEvent(EV_SFX, p.x, p.y, 12.f, 0.7f);
  }
  enemies.alertAll(room, p.x, p.y, 30.f);
}

void GameWorldManager::hurtPlayer(f32 dmg, f32 fromX, f32 fromY) {
  if (player.stats[STAT_INVULN] > 0.f) return;
  player.damage(dmg);
  f32 d = distf(fromX, fromY, player.x, player.y);
  if (d < 0.5f) d = 0.5f;
  f32 kx = (player.x - fromX) / d;
  f32 ky = (player.y - fromY) / d;
  moveCircle(room, player.x, player.y, kx * 26.f, ky * 26.f, 10.f);
  pushEvent(EV_HIT, player.x, player.y, dmg, 0.f);
  pushEvent(EV_SHAKE, 0.f, 0.f, 10.f, 0.5f);
  pushEvent(EV_SFX, player.x, player.y, 16.f, 1.f);
  if (player.stats[STAT_HP] <= 0.f) {
    state = GS_DEAD;
    deaths++;
    pushEvent(EV_DEATH, player.x, player.y, 0.f, 0.f);
  }
}

// ---------------------------------------------------------------- puzzles
void GameWorldManager::puzzleUndo() {
  PuzzleAction a;
  if (!actions.pop(a)) return;
  if (a.kind == ACTK_VALVE) {
    valveProgress = a.prevValue;
    player.setFlag(PF_VALVES_DONE, false);
  } else if (a.kind == ACTK_CANDLE) {
    candleProgress = a.prevValue;
    for (i32 i = 0; i < rooms[room].propCount; ++i) {
      RoomProp& p = rooms[room].props[i];
      if (p.type == PROP_RITUAL_CANDLE) p.state = 0;
    }
  } else if (a.kind == ACTK_DRAWER) {
    for (i32 i = 0; i < rooms[room].propCount; ++i) {
      RoomProp& p = rooms[room].props[i];
      if (p.type == PROP_DRAWER && p.id == a.target) p.state = a.prevValue;
    }
  } else if (a.kind == ACTK_FUSE) {
    fuseMask = a.prevValue;
  }
  pushEvent(EV_SFX, player.x, player.y, 9.f, 0.4f);
}

void GameWorldManager::codeReset() {
  codeProgress = 0;
  codeDigits[0] = codeDigits[1] = codeDigits[2] = codeDigits[3] = 0;
}

void GameWorldManager::codeDigit(i32 digit) {
  if (digit < 0) digit = 0;
  if (digit > 9) digit = 9;
  if (codeProgress < 0 || codeProgress >= 4) return;
  codeDigits[codeProgress++] = digit;
  actions.push(ACTK_CODE_DIGIT, codeProgress - 1, digit, 0, player.x, player.y, tick);
  pushEvent(EV_SFX, player.x, player.y, 10.f, 0.35f);
  if (codeProgress < 4) return;

  i32 solution[4] = { 0, 3, 1, 7 };
  bool ok = true;
  for (i32 i = 0; i < 4; ++i) if (codeDigits[i] != solution[i]) ok = false;
  if (ok) {
    codeSolved = 1;
    unlockRequirement(*this, -3);
    notify(MSG_CODE_OK);
    pushEvent(EV_SFX, player.x, player.y, 5.f, 0.9f);
    uiRequest = 0;
  } else {
    notify(MSG_CODE_WRONG);
    pushNoise(player.x, player.y, 0.8f, 3);
    pushEvent(EV_SHAKE, 0.f, 0.f, 6.f, 0.4f);
    codeReset();
  }
}

void GameWorldManager::solveCode() {
  codeSolved = 1;
  unlockRequirement(*this, -3);
}

// ---------------------------------------------------------------- interact
void GameWorldManager::interact() {
  if (state != GS_PLAYING) return;
  if (prompt.id == PROMPT_NONE) return;

  // ---- items
  if (prompt.id == PROMPT_TAKE) {
    i32 idx = -1000 - prompt.target;
    if (idx >= 0) takeItem(room, idx);
    return;
  }

  // ---- leaving a hiding place
  if (prompt.id == PROMPT_LEAVE_HIDE) {
    player.setFlag(PF_HIDING, false);
    player.hideProp = -1;
    notify(MSG_UNHIDE);
    pushEvent(EV_SFX, player.x, player.y, 6.f, 0.4f);
    return;
  }

  // ---- doors
  if (prompt.id == PROMPT_USE_DOOR || prompt.id == PROMPT_LOCKED_DOOR) {
    i32 idx = prompt.target;
    if (idx < 0 || idx >= rooms[room].doorCount) return;
    RoomDoor& d = rooms[room].doors[idx];
    if (!doorTable.unlocked[d.id]) unlockDoor(idx);
    else travelThroughDoor(idx);
    return;
  }

  // ---- props
  if (prompt.target < 0 || prompt.target >= rooms[room].propCount) return;
  RoomProp& p = rooms[room].props[prompt.target];
  f32 px = p.x, py = p.y;

  switch (p.type) {
    case PROP_NOTE: {
      if (p.clueId > 0) addClue(p.clueId, room);
      p.state = 1;
      pushEvent(EV_SFX, px, py, 8.f, 0.4f);
      break;
    }
    case PROP_CANDLE: {
      p.state = 1;
      checkpoints++;
      serialize();
      notify(MSG_SAVE);
      pushEvent(EV_SAVE, px, py, 0.f, 0.f);
      pushEvent(EV_SFX, px, py, 13.f, 0.6f);
      break;
    }
    case PROP_LOCKER:
    case PROP_BED: {
      if (p.type == PROP_LOCKER) {
        player.hideProp = prompt.target;
        player.setFlag(PF_HIDING, true);
        player.x = p.hideX;
        player.y = p.hideY;
        notify(MSG_HIDING);
        pushEvent(EV_SFX, px, py, 6.f, 0.5f);
      } else {
        notify(MSG_EXAMINE);
      }
      break;
    }
    case PROP_MIRROR: {
      if (p.state == 0) {
        p.state = 1;
        notify(MSG_MIRROR);
        pushEvent(EV_JUMPSCARE, px, py, 1.f, 0.f);
        pushEvent(EV_SHAKE, 0.f, 0.f, 12.f, 0.9f);
        pushEvent(EV_FLASH, px, py, 0.7f, 1.f);
        addClue(CLUE_MARA_NOTE, room);
      } else {
        notify(MSG_MIRROR_AGAIN);
      }
      break;
    }
    case PROP_BREAKER: {
      if (player.hasFlag(PF_POWER_ON)) { notify(MSG_POWER_ON); break; }
      i32 insertedMask = fuseMask;
      i32 need[3] = { IT_FUSE_BLUE, IT_FUSE_RED, IT_FUSE_BRASS };
      for (i32 i = 0; i < 3; ++i) {
        if (fuseMask & (1 << i)) continue;
        if (inv.quantity(need[i]) > 0) {
          inv.remove(need[i], 1);
          fuseMask |= (1 << i);
          actions.push(ACTK_FUSE, i, fuseMask, insertedMask, px, py, tick);
          pushEvent(EV_SFX, px, py, 12.f, 0.5f);
          notify(MSG_FUSE_INSERT);
          break;
        }
      }
      if (fuseMask == 7 && !player.hasFlag(PF_POWER_ON)) {
        player.setFlag(PF_POWER_ON, true);
        notify(MSG_POWER_ON);
        subtitleId = SUB_POWER;
        subtitleTimer = 4.f;
        pushEvent(EV_SFX, px, py, 17.f, 1.f);
        pushEvent(EV_SHAKE, 0.f, 0.f, 8.f, 0.8f);
        pushEvent(EV_QUEST, px, py, (f32)SUB_POWER, 0.f);
        unlockRequirement(*this, -2);
        enemies.alertAll(room, px, py, 20.f);
      } else if (fuseMask != 7) {
        notify(MSG_FUSE_NEEDED);
      }
      break;
    }
    case PROP_VALVE: {
      if (player.hasFlag(PF_VALVES_DONE)) { notify(MSG_VALVES_DONE); break; }
      if (!inv.find(IT_VALVE_HANDLE)) { notify(MSG_NEED_HANDLE); break; }
      if (p.id == valveOrder[valveProgress]) {
        i32 prev = valveProgress;
        valveProgress++;
        actions.push(ACTK_VALVE, p.id, valveProgress, prev, px, py, tick);
        p.state = 1;
        pushEvent(EV_SFX, px, py, 12.f, 0.7f);
        pushNoise(px, py, 0.35f, 2);
        if (valveProgress >= 3) {
          player.setFlag(PF_VALVES_DONE, true);
          unlockRequirement(*this, -1);
          notify(MSG_VALVES_DONE);
          subtitleId = SUB_VALVES;
          subtitleTimer = 4.f;
          pushEvent(EV_SFX, px, py, 17.f, 1.f);
          pushEvent(EV_SHAKE, 0.f, 0.f, 10.f, 1.f);
          pushEvent(EV_QUEST, px, py, (f32)SUB_VALVES, 0.f);
        } else {
          notify(MSG_VALVE_GOOD);
        }
      } else {
        valveProgress = 0;
        for (i32 i = 0; i < rooms[room].propCount; ++i)
          if (rooms[room].props[i].type == PROP_VALVE) rooms[room].props[i].state = 0;
        notify(MSG_VALVE_BAD);
        pushNoise(px, py, 1.f, 3);
        pushEvent(EV_SFX, px, py, 14.f, 1.f);
        pushEvent(EV_SHAKE, 0.f, 0.f, 10.f, 0.8f);
      }
      break;
    }
    case PROP_CODELOCK: {
      if (!player.hasFlag(PF_POWER_ON)) { notify(MSG_CODE_DEAD); break; }
      if (codeSolved) { notify(MSG_CODE_OK); break; }
      uiRequest = 1;
      codeReset();
      pushEvent(EV_SFX, px, py, 10.f, 0.5f);
      break;
    }
    case PROP_DRAWER: {
      if (p.state == 0) {
        p.state = 1;
        actions.push(ACTK_DRAWER, p.id, 1, 0, px, py, tick);
        pushEvent(EV_SFX, px, py, 8.f, 0.6f);
        if (p.clueId > 0) addClue(p.clueId, room);
        if (p.itemId > 0) {
          i32 added = inv.insert(p.itemId, 1);
          if (added > 0) {
            if (p.itemId == IT_LOCKET) {
              player.setFlag(PF_WEAVER_AWAKE, true);
              weaverPhase = 2;
              notify(MSG_TAKE_LOCKET);
              subtitleId = SUB_LOCKET;
              subtitleTimer = 5.f;
              pushEvent(EV_QUEST, px, py, (f32)SUB_LOCKET, 0.f);
              enemies.awakeWeaver(*this, room, player.x + 150.f, player.y + 40.f);
              pushEvent(EV_JUMPSCARE, px, py, 2.f, 0.f);
              pushEvent(EV_SHAKE, 0.f, 0.f, 16.f, 1.4f);
              pushEvent(EV_SFX, 0.f, 0.f, 30.f, 1.f);
            } else if (p.itemId == IT_BRASS_KEY) {
              notify(MSG_SECRET);
            }
            p.itemId = 0;
          }
        } else if (p.clueId <= 0) {
          notify(MSG_DRAWER_EMPTY);
        }
      } else {
        p.state = 0;
        PuzzleAction a;
        if (actions.pop(a) && a.kind != ACTK_DRAWER) actions.push(a.kind, a.target, a.value, a.prevValue, a.x, a.y, a.tick);
        pushEvent(EV_SFX, px, py, 8.f, 0.4f);
      }
      break;
    }
    case PROP_RITUAL_CANDLE: {
      if (p.state == 1) { notify(MSG_CANDLE_DONE); break; }
      if (p.id == candleOrder[candleProgress]) {
        i32 prev = candleProgress;
        candleProgress++;
        p.state = 1;
        actions.push(ACTK_CANDLE, p.id, candleProgress, prev, px, py, tick);
        pushEvent(EV_SFX, px, py, 13.f, 0.6f);
        if (candleProgress >= 4) {
          secretCount++;
          addClue(CLUE_CRIMSON_SLEEP, room);
          inv.insert(IT_AMMO, 6);
          inv.insert(IT_BANDAGE, 1);
          notify(MSG_CANDLE_DONE);
          pushEvent(EV_SFX, px, py, 17.f, 1.f);
          pushEvent(EV_QUEST, px, py, (f32)MSG_CANDLE_DONE, 0.f);
        } else {
          notify(MSG_CANDLE_GOOD);
        }
      } else {
        candleProgress = 0;
        for (i32 i = 0; i < rooms[room].propCount; ++i)
          if (rooms[room].props[i].type == PROP_RITUAL_CANDLE) rooms[room].props[i].state = 0;
        notify(MSG_CANDLE_BAD);
        pushNoise(px, py, 0.8f, 3);
        pushEvent(EV_SFX, px, py, 14.f, 0.9f);
      }
      break;
    }
    case PROP_CRADLE: {
      bool hasLocket = inv.find(IT_LOCKET) != 0;
      bool hasSedative = inv.find(IT_SEDATIVE) != 0;
      if (hasLocket && hasSedative) {
        state = GS_ENDING;
        endingId = 1;
        player.setFlag(PF_ENDING, true);
        pushEvent(EV_ENDING, px, py, 1.f, 0.f);
      } else if (hasLocket) {
        state = GS_ENDING;
        endingId = 2;
        player.setFlag(PF_ENDING, true);
        pushEvent(EV_ENDING, px, py, 2.f, 0.f);
      } else {
        notify(MSG_CRADLE_NOLOCKET);
      }
      break;
    }
    case PROP_BODY: {
      notify(MSG_BODY);
      pushEvent(EV_SFX, px, py, 18.f, 0.5f);
      if (rng.unit() < 0.35f) pushEvent(EV_SHAKE, 0.f, 0.f, 5.f, 0.5f);
      break;
    }
    case PROP_BOARD: {
      if (p.id == 0) {
        if (p.state == 1) { notify(MSG_EXAMINE); break; }
        if (!inv.find(IT_CROWBAR)) { notify(MSG_BOARD_NAILED); break; }
        p.state = 1;
        addClue(CLUE_HIDDEN_PANEL, room);
        i32 added = inv.insert(IT_AMMO, 6);
        if (added > 0) secretCount++;
        notify(MSG_BOARD_OPEN);
        pushEvent(EV_SFX, px, py, 12.f, 0.9f);
        pushEvent(EV_SHAKE, 0.f, 0.f, 7.f, 0.6f);
        pushNoise(px, py, 0.9f, 3);
      } else {
        notify(MSG_DOOR_FOREVER);
      }
      break;
    }
    case PROP_ORGAN: {
      addClue(CLUE_CHAPEL_HYMN, room);
      p.state = 1;
      pushEvent(EV_SFX, px, py, 19.f, 0.9f);
      pushEvent(EV_QUEST, px, py, (f32)MSG_ORGAN, 0.f);
      break;
    }
    case PROP_SIGIL: {
      notify(MSG_EXAMINE);
      if (rng.unit() < 0.25f) pushEvent(EV_JUMPSCARE, px, py, 3.f, 0.f);
      break;
    }
    case PROP_WINDOW: {
      notify(MSG_EXAMINE);
      pushEvent(EV_SFX, px, py, 21.f, 0.4f);
      break;
    }
    case PROP_LAMP: {
      notify(MSG_EXAMINE);
      break;
    }
    case PROP_GURNEY: {
      notify(MSG_EXAMINE);
      break;
    }
    default: break;
  }
}

} // namespace tcs
