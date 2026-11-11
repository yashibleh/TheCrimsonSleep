#include "EnemyController.hpp"
#include "GameWorldManager.hpp"
#include "Messages.hpp"

namespace tcs {

// ------------------------------------------------------------------ helpers

static void clearEnemy(Enemy& e) {
  e.alive = 0;
  e.type = 0;
  e.room = 0;
  e.x = 0.f; e.y = 0.f;
  e.vx = 0.f; e.vy = 0.f;
  e.homeX = 0.f; e.homeY = 0.f;
  for (i32 i = 0; i < 6; ++i) e.patrolAnchors[i] = 0.f;
  e.anchorCount = 0;
  e.anchorIndex = 0;
  e.hp = 0.f; e.maxHp = 0.f;
  e.state = ES_IDLE;
  e.stateTimer = 0.f;
  e.alert = 0.f;
  e.dirX = 0.f; e.dirY = 1.f;
  e.anim = 0.f; e.animPhase = 0.f;
  e.attackTimer = 0.f;
  e.attackCooldown = 0.f;
  e.thinkTimer = 0.f;
  e.repathTimer = 0.f;
  e.hitFlash = 0.f;
  e.staggerTimer = 0.f;
  e.speakTimer = 0.f;
  e.searchTimer = 0.f;
  e.searchSteps = 0;
  e.lastKnownX = 0.f; e.lastKnownY = 0.f;
  e.targetX = 0.f; e.targetY = 0.f;
  e.wanderX = 0.f; e.wanderY = 0.f;
  for (i32 i = 0; i < MAX_ENEMY_PATH; ++i) e.path[i] = 0;
  e.pathLen = 0;
  e.pathIndex = 0;
  e.seesPlayer = 0;
  e.heardNoise = 0;
  e.didHit = 0;
  e.noiseX = 0.f; e.noiseY = 0.f; e.noisePower = 0.f;
  e.pendingRoom = -1;
  e.transferTimer = 0.f;
  e.pendingX = 0.f; e.pendingY = 0.f;
  e.killsPlayerOnTouch = 0;
}

static i32 stateForAction(i32 action) {
  switch (action) {
    case ACT_ATTACK: return ES_ATTACK;
    case ACT_CHASE: return ES_CHASE;
    case ACT_INVESTIGATE: return ES_INVESTIGATE;
    case ACT_SEARCH: return ES_SEARCH;
    case ACT_RETREAT: return ES_SUSPICIOUS;
    case ACT_RETURN: return ES_RETURN;
    case ACT_PATROL: return ES_PATROL;
    case ACT_IDLE: return ES_IDLE;
    default: return ES_IDLE;
  }
}

static void stepToward(GameWorldManager& w, EnemyController& c, Enemy& e,
                       f32 tx, f32 ty, f32 speed, f32 dt) {
  const u8* grid = w.rooms[e.room].navBlocked;
  f32 d = distf(e.x, e.y, tx, ty);
  if (d < 3.f) return;

  if (c.nav.losClear(grid, e.x, e.y, tx, ty)) {
    f32 nx = e.x + (tx - e.x) / d * speed * dt;
    f32 ny = e.y + (ty - e.y) / d * speed * dt;
    if (c.nav.walkableAt(grid, nx, ny)) {
      e.dirX = (tx - e.x) / d;
      e.dirY = (ty - e.y) / d;
      e.x = nx;
      e.y = ny;
    }
    return;
  }

  e.repathTimer -= dt;
  if (e.pathLen == 0 || e.repathTimer <= 0.f) {
    e.pathLen = c.nav.findCellPath(grid, e.x, e.y, tx, ty, e.path, MAX_ENEMY_PATH);
    e.pathIndex = 0;
    e.repathTimer = 0.45f;
  }
  i32 guard = 0;
  while (e.pathIndex < e.pathLen && guard++ < MAX_ENEMY_PATH) {
    i32 cell = e.path[e.pathIndex];
    f32 wx = (f32)(cell % ROOM_W) * (f32)TILE + (f32)TILE * 0.5f;
    f32 wy = (f32)(cell / ROOM_W) * (f32)TILE + (f32)TILE * 0.5f;
    f32 dd = distf(e.x, e.y, wx, wy);
    if (dd < 13.f) { e.pathIndex++; continue; }
    e.dirX = (wx - e.x) / dd;
    e.dirY = (wy - e.y) / dd;
    e.x += e.dirX * speed * dt;
    e.y += e.dirY * speed * dt;
    e.animPhase += speed * dt * 0.08f;
    break;
  }
}

// ------------------------------------------------------------------ lifecycle

void EnemyController::reset() {
  count = 0;
  decisions.reset();
  nav.reset();
  rng.seed(0xBADF00Du);
  for (i32 i = 0; i < MAX_ENEMIES; ++i) clearEnemy(list[i]);
}

void EnemyController::clear() {
  for (i32 i = 0; i < MAX_ENEMIES; ++i) clearEnemy(list[i]);
  count = 0;
}

i32 EnemyController::spawn(i32 type, i32 room, f32 x, f32 y) {
  if (type < 0 || type >= ENEMY_TYPES) return -1;
  if (count >= MAX_ENEMIES) return -1;
  i32 idx = -1;
  for (i32 i = 0; i < MAX_ENEMIES; ++i) {
    if (!list[i].alive) { idx = i; break; }
  }
  if (idx < 0) return -1;
  if (idx >= count) count = idx + 1;
  Enemy& e = list[idx];
  clearEnemy(e);
  const EnemyDef& def = ENEMY_DEFS[type];
  e.alive = 1;
  e.type = type;
  e.room = room;
  e.x = x;
  e.y = y;
  e.homeX = x;
  e.homeY = y;
  e.hp = def.hp;
  e.maxHp = def.hp;
  e.state = ES_PATROL;
  e.alert = 0.f;
  e.dirX = 0.f;
  e.dirY = 1.f;
  e.thinkTimer = rng.range(0.1f, 0.4f);
  e.repathTimer = 0.f;
  e.attackCooldown = rng.range(0.f, 0.4f);
  e.speakTimer = rng.range(4.f, 12.f);
  e.killsPlayerOnTouch = type == E_WEAVER ? 1 : 0;
  return idx;
}

void EnemyController::addAnchor(Enemy& e, f32 x, f32 y) {
  if (e.anchorCount >= 3) return;
  e.patrolAnchors[e.anchorCount * 2] = x;
  e.patrolAnchors[e.anchorCount * 2 + 1] = y;
  e.anchorCount++;
}

void EnemyController::awakeWeaver(GameWorldManager& w, i32 room, f32 x, f32 y) {
  for (i32 i = 0; i < count; ++i) {
    Enemy& e = list[i];
    if (!e.alive || e.type != E_WEAVER) continue;
    e.room = room;
    e.x = x;
    e.y = y;
    e.homeX = x;
    e.homeY = y;
    e.state = ES_CHASE;
    e.alert = 100.f;
    e.lastKnownX = w.player.x;
    e.lastKnownY = w.player.y;
    e.pathLen = 0;
    e.thinkTimer = 0.1f;
    return;
  }
  i32 idx = spawn(E_WEAVER, room, x, y);
  if (idx < 0) return;
  Enemy& e = list[idx];
  e.state = ES_CHASE;
  e.alert = 100.f;
  e.lastKnownX = w.player.x;
  e.lastKnownY = w.player.y;
  addAnchor(e, x, y);
  addAnchor(e, w.player.x, w.player.y);
}

i32 EnemyController::aliveInRoom(i32 room) const {
  i32 n = 0;
  for (i32 i = 0; i < count; ++i)
    if (list[i].alive && list[i].room == room) n++;
  return n;
}

i32 EnemyController::nearestIndex(i32 room, f32 x, f32 y, f32 maxDist) const {
  i32 best = -1;
  f32 bestD = maxDist * maxDist;
  for (i32 i = 0; i < count; ++i) {
    const Enemy& e = list[i];
    if (!e.alive) continue;
    if (room >= 0 && e.room != room) continue;
    f32 d = dist2(x, y, e.x, e.y);
    if (d < bestD) { bestD = d; best = i; }
  }
  return best;
}

void EnemyController::alertAll(i32 room, f32 x, f32 y, f32 amount) {
  for (i32 i = 0; i < count; ++i) {
    Enemy& e = list[i];
    if (!e.alive || e.room != room) continue;
    e.alert += amount;
    if (e.alert > 100.f) e.alert = 100.f;
    e.lastKnownX = x;
    e.lastKnownY = y;
    if (e.alert > 45.f && e.state != ES_CHASE && e.state != ES_ATTACK) {
      e.state = ES_CHASE;
      e.stateTimer = 0.f;
      e.pathLen = 0;
    }
  }
}

void EnemyController::transferRoom(i32 from, i32 to, f32 spawnX, f32 spawnY) {
  for (i32 i = 0; i < count; ++i) {
    Enemy& e = list[i];
    if (!e.alive || e.room != from) continue;
    bool hunting = e.state == ES_CHASE || e.state == ES_ATTACK || e.alert > 55.f;
    if (!hunting) continue;
    e.pendingRoom = to;
    e.pendingX = spawnX + rng.range(-30.f, 30.f);
    e.pendingY = spawnY + rng.range(-30.f, 30.f);
    e.transferTimer = e.type == E_WEAVER ? 0.9f : 1.6f;
  }
}

// ------------------------------------------------------------------ damage

bool EnemyController::hurt(GameWorldManager& w, f32 x, f32 y, f32 radius, f32 damage,
                           f32 fromX, f32 fromY) {
  i32 idx = -1;
  f32 bestD = radius * radius;
  for (i32 i = 0; i < count; ++i) {
    Enemy& e = list[i];
    if (!e.alive || e.room != w.room) continue;
    f32 d = dist2(x, y, e.x, e.y);
    if (d < bestD) { bestD = d; idx = i; }
  }
  if (idx < 0) return false;
  Enemy& e = list[idx];

  e.alert = 100.f;
  e.lastKnownX = w.player.x;
  e.lastKnownY = w.player.y;
  if (e.state != ES_ATTACK) {
    e.state = ES_CHASE;
    e.stateTimer = 0.f;
    e.pathLen = 0;
  }
  w.pushEvent(EV_SFX, e.x, e.y, 18.f, 0.9f);
  w.pushEvent(EV_SHAKE, 0.f, 0.f, 4.f, 0.25f);

  f32 d = distf(fromX, fromY, e.x, e.y);
  if (d < 1.f) d = 1.f;
  f32 kx = (e.x - fromX) / d;
  f32 ky = (e.y - fromY) / d;
  f32 push = e.type == E_WEAVER ? 6.f : 16.f;
  const u8* grid = w.rooms[e.room].navBlocked;
  if (nav.walkableAt(grid, e.x + kx * push, e.y + ky * push)) {
    e.x += kx * push;
    e.y += ky * push;
  }

  if (e.type == E_WEAVER) {
    e.staggerTimer = 0.5f;
    w.pushEvent(EV_TEXT, e.x, e.y, (f32)MSG_WEAVER_BLOCK, 0.f);
    return true;
  }

  e.hp -= damage;
  e.hitFlash = 1.f;
  e.staggerTimer = 0.22f;
  w.player.damageDealt += (i32)damage;

  if (e.hp <= 0.f) {
    e.hp = 0.f;
    e.alive = 0;
    w.player.kills++;
    w.totalKills++;
    w.pushEvent(EV_SFX, e.x, e.y, 22.f, 1.f);
    w.pushEvent(EV_SHAKE, 0.f, 0.f, 8.f, 0.5f);
  }
  return true;
}

// ------------------------------------------------------------------ think

void EnemyController::think(GameWorldManager& w, Enemy& e, i32 index) {
  (void)index;
  const EnemyDef& def = ENEMY_DEFS[e.type];
  decisions.reset();
  f32 d = distf(e.x, e.y, w.player.x, w.player.y);
  bool canSee = e.seesPlayer && !w.player.hasFlag(PF_HIDING);

  if (canSee && d <= def.attackRange + 10.f && e.attackCooldown <= 0.f) {
    decisions.push(ACT_ATTACK, 100.f + (def.attackRange - d), w.player.x, w.player.y);
  }
  if (canSee) {
    f32 proximity = 1.f - clampf(d / def.sightRange, 0.f, 1.f);
    decisions.push(ACT_CHASE, 72.f + proximity * 24.f + e.alert * 0.12f, w.player.x, w.player.y);
  }
  if (e.heardNoise) {
    decisions.push(ACT_INVESTIGATE, 40.f + e.noisePower * 46.f, e.noiseX, e.noiseY);
  }
  if (e.alert > 15.f && !canSee) {
    decisions.push(ACT_SEARCH, 18.f + e.alert * 0.38f, e.lastKnownX, e.lastKnownY);
  }
  if (e.type != E_WEAVER && e.hp < e.maxHp * 0.35f) {
    f32 awayX = e.x + (e.x - w.player.x);
    f32 awayY = e.y + (e.y - w.player.y);
    decisions.push(ACT_RETREAT, 48.f, awayX, awayY);
  }
  decisions.push(ACT_RETURN, 9.f, e.homeX, e.homeY);
  decisions.push(ACT_PATROL, 7.f, e.anchorCount > 0 ? e.patrolAnchors[0] : e.homeX,
                 e.anchorCount > 0 ? e.patrolAnchors[1] : e.homeY);
  decisions.push(ACT_IDLE, 3.f, e.x, e.y);

  EnemyDecision best = decisions.pop();
  i32 mapped = stateForAction(best.action);
  if (mapped != e.state) {
    applyState(w, e, mapped);
    if (mapped == ES_INVESTIGATE) { e.targetX = best.targetX; e.targetY = best.targetY; }
    if (mapped == ES_SUSPICIOUS) { e.targetX = best.targetX; e.targetY = best.targetY; }
  }
}

void EnemyController::applyState(GameWorldManager& w, Enemy& e, i32 state) {
  e.state = state;
  e.stateTimer = 0.f;
  e.pathLen = 0;
  e.pathIndex = 0;
  e.repathTimer = 0.f;
  e.didHit = 0;

  switch (state) {
    case ES_INVESTIGATE:
      e.targetX = e.noiseX;
      e.targetY = e.noiseY;
      e.heardNoise = 0;
      break;
    case ES_SEARCH: {
      e.searchSteps = 0;
      f32 ox = rng.range(-90.f, 90.f);
      f32 oy = rng.range(-70.f, 70.f);
      e.wanderX = e.lastKnownX + ox;
      e.wanderY = e.lastKnownY + oy;
      const u8* grid = w.rooms[e.room].navBlocked;
      if (!nav.walkableAt(grid, e.wanderX, e.wanderY)) {
        e.wanderX = e.lastKnownX;
        e.wanderY = e.lastKnownY;
      }
      break;
    }
    case ES_CHASE:
      e.targetX = w.player.x;
      e.targetY = w.player.y;
      break;
    case ES_ATTACK:
      e.attackTimer = 0.42f;
      w.pushEvent(EV_SFX, e.x, e.y, e.type == E_WEAVER ? 31.f : 24.f, 0.9f);
      break;
    case ES_RETURN:
      e.targetX = e.homeX;
      e.targetY = e.homeY;
      break;
    case ES_SUSPICIOUS:
      e.targetX = e.lastKnownX;
      e.targetY = e.lastKnownY;
      break;
    default:
      break;
  }
  if (state == ES_CHASE) {
    w.pushEvent(EV_ENEMY_ALERT, e.x, e.y, (f32)e.type, e.alert);
  }
}

// ------------------------------------------------------------------ update

void EnemyController::updateAll(GameWorldManager& w, f32 dt) {
  SoundEventQueue& q = w.sounds;

  // listeners that cannot hear this frame still mark events so the FIFO drains
  for (i32 i = 0; i < MAX_ENEMIES; ++i) {
    if (i >= count || !list[i].alive || list[i].room != w.room) q.markRead(i);
  }

  for (i32 i = 0; i < count; ++i) {
    Enemy& e = list[i];
    if (!e.alive) { q.markRead(i); continue; }

    if (e.transferTimer > 0.f) {
      e.transferTimer -= dt;
      if (e.transferTimer <= 0.f) {
        e.transferTimer = 0.f;
        e.room = e.pendingRoom >= 0 ? e.pendingRoom : w.room;
        e.x = e.pendingX;
        e.y = e.pendingY;
        e.state = ES_CHASE;
        e.stateTimer = 0.f;
        e.alert = 100.f;
        e.lastKnownX = w.player.x;
        e.lastKnownY = w.player.y;
        e.pathLen = 0;
        w.pushEvent(EV_SFX, e.x, e.y, 30.f, 1.f);
        w.pushEvent(EV_SHAKE, 0.f, 0.f, 9.f, 0.8f);
      }
      continue;
    }

    if (e.room != w.room) {
      // The Weaver hunts along the world graph: BFS over rooms joined by doors that
      // are actually open. Lock a door behind you and its route disappears.
      // Its pursuit is driven by alert, not a permanent switch: break contact for
      // long enough and the trail goes cold (this is what makes hiding work).
      if (e.type == E_WEAVER) {
        e.alert -= 4.5f * dt;
        if (e.alert < 0.f) e.alert = 0.f;
      }
      if (e.type == E_WEAVER && w.reachMask[e.room] && e.alert > 55.f) {
        e.graphTimer -= dt;
        if (e.graphTimer <= 0.f) {
          i32 next = w.nextRoomToward(e.room, w.room);
          if (next >= 0 && next != e.room) {
            RoomData& nr = w.rooms[next];
            f32 sx = 160.f, sy = 160.f;
            for (i32 d = 0; d < nr.doorCount; ++d) {
              if (nr.doors[d].targetRoom == e.room) {
                sx = nr.doors[d].spawnX;
                sy = nr.doors[d].spawnY;
                break;
              }
            }
            e.room = next;
            e.x = sx;
            e.y = sy;
            e.state = ES_CHASE;
            e.stateTimer = 0.f;
            e.alert = 100.f;
            e.lastKnownX = w.player.x;
            e.lastKnownY = w.player.y;
            e.pathLen = 0;
            e.graphTimer = 2.4f;
            w.pushEvent(EV_SFX, e.x, e.y, 30.f, 0.9f);
            w.pushEvent(EV_SHAKE, 0.f, 0.f, 5.f, 0.5f);
          } else {
            e.graphTimer = 1.2f;
          }
        }
      }
      q.markRead(i);
      continue;
    }
    updateEnemy(w, e, i, dt);
  }

  i32 guard = 0;
  while (!q.empty() && q.allRead() && guard++ < MAX_SOUNDS) q.pop();
}

void EnemyController::updateEnemy(GameWorldManager& w, Enemy& e, i32 index, f32 dt) {
  const EnemyDef& def = ENEMY_DEFS[e.type];
  e.anim += dt;
  e.animPhase += dt * (e.state == ES_CHASE ? 6.f : 2.2f);
  if (e.hitFlash > 0.f) e.hitFlash -= dt * 2.5f;
  if (e.attackCooldown > 0.f) e.attackCooldown -= dt;
  if (e.staggerTimer > 0.f) { e.staggerTimer -= dt; return; }
  e.stateTimer += dt;

  PlayerState& pl = w.player;
  f32 d = distf(e.x, e.y, pl.x, pl.y);
  f32 sight = def.sightRange;
  if (pl.hasFlag(PF_FLASHLIGHT_ON)) sight *= 1.28f;
  if (pl.hasFlag(PF_SPRINTING)) sight *= 1.06f;
  bool los = nav.losClear(w.rooms[e.room].navBlocked, e.x, e.y, pl.x, pl.y);
  bool inFov = true;
  if (def.fovCos >= 0.f && d > 2.f) {
    f32 dx = (pl.x - e.x) / d;
    f32 dy = (pl.y - e.y) / d;
    inFov = (dx * e.dirX + dy * e.dirY) >= def.fovCos;
  }
  bool hidden = pl.hasFlag(PF_HIDING);
  e.seesPlayer = (!hidden && d < sight && los && inFov) ? 1 : 0;

  if (e.seesPlayer) {
    e.alert += def.alertGain * dt * (1.f + (1.f - clampf(d / sight, 0.f, 1.f)) * 0.9f);
    e.lastKnownX = pl.x;
    e.lastKnownY = pl.y;
  } else {
    e.alert -= def.alertDecay * dt;
    if (e.alert < 0.f) e.alert = 0.f;
  }
  if (e.alert > 100.f) e.alert = 100.f;

  // ---- FIFO sound investigation
  if (!w.sounds.empty() && w.sounds.unread(index)) {
    SoundEvent& s = w.sounds.front();
    f32 sd = distf(e.x, e.y, s.x, s.y);
    f32 hearRadius = def.hearing * (0.28f + s.intensity * 0.72f);
    if (sd < hearRadius) {
      e.noiseX = s.x;
      e.noiseY = s.y;
      e.noisePower = s.intensity;
      e.heardNoise = 1;
      e.lastKnownX = s.x;
      e.lastKnownY = s.y;
      if (e.alert < 70.f) e.alert = minf(100.f, e.alert + 18.f + s.intensity * 42.f);
    }
    w.sounds.markRead(index);
  }

  // ---- decision tick
  e.thinkTimer -= dt;
  if (e.thinkTimer <= 0.f) {
    think(w, e, index);
    e.thinkTimer = 0.22f + rng.unit() * 0.2f;
  }

  // ---- state machine
  switch (e.state) {
    case ES_IDLE: {
      if (e.stateTimer > 2.4f) applyState(w, e, ES_PATROL);
      break;
    }
    case ES_PATROL: {
      if (e.seesPlayer) { applyState(w, e, ES_CHASE); break; }
      if (e.heardNoise) { applyState(w, e, ES_INVESTIGATE); break; }
      f32 tx = e.homeX, ty = e.homeY;
      if (e.anchorCount > 0) {
        tx = e.patrolAnchors[e.anchorIndex * 2];
        ty = e.patrolAnchors[e.anchorIndex * 2 + 1];
        if (distf(e.x, e.y, tx, ty) < 26.f) {
          e.anchorIndex = (e.anchorIndex + 1) % e.anchorCount;
          tx = e.patrolAnchors[e.anchorIndex * 2];
          ty = e.patrolAnchors[e.anchorIndex * 2 + 1];
          e.pathLen = 0;
        }
      }
      stepToward(w, *this, e, tx, ty, def.speed, dt);
      break;
    }
    case ES_SUSPICIOUS: {
      f32 dt2 = distf(e.x, e.y, e.targetX, e.targetY);
      if (dt2 > 12.f && e.hp > e.maxHp * 0.35f) stepToward(w, *this, e, e.targetX, e.targetY, def.speed, dt);
      if (e.hp <= e.maxHp * 0.35f) {
        // retreat away from the player, then resume hunting
        f32 ax = e.x + (e.x - pl.x), ay = e.y + (e.y - pl.y);
        stepToward(w, *this, e, ax, ay, def.chaseSpeed * 0.8f, dt);
        if (e.stateTimer > 1.2f) applyState(w, e, ES_SEARCH);
      } else if (e.stateTimer > 1.4f) {
        applyState(w, e, ES_SEARCH);
      }
      if (e.seesPlayer) applyState(w, e, ES_CHASE);
      break;
    }
    case ES_INVESTIGATE: {
      if (e.seesPlayer) { applyState(w, e, ES_CHASE); break; }
      f32 dd = distf(e.x, e.y, e.targetX, e.targetY);
      if (dd < 26.f) { applyState(w, e, ES_SEARCH); break; }
      stepToward(w, *this, e, e.targetX, e.targetY, def.speed * 1.2f, dt);
      if (e.stateTimer > 12.f) applyState(w, e, ES_RETURN);
      break;
    }
    case ES_CHASE: {
      if (e.seesPlayer) {
        e.stateTimer = 0.f;
        e.targetX = pl.x;
        e.targetY = pl.y;
      } else if (e.stateTimer > 3.4f) {
        applyState(w, e, ES_SEARCH);
        break;
      }
      if (d < def.attackRange && e.attackCooldown <= 0.f && !hidden) {
        applyState(w, e, ES_ATTACK);
        break;
      }
      stepToward(w, *this, e, e.targetX, e.targetY, def.chaseSpeed, dt);
      break;
    }
    case ES_ATTACK: {
      e.attackTimer -= dt;
      if (!e.didHit && e.attackTimer <= 0.22f) {
        e.didHit = 1;
        f32 reach = def.attackRange + 12.f;
        if (distf(e.x, e.y, pl.x, pl.y) < reach && !hidden) {
          w.hurtPlayer(def.damage, e.x, e.y);
        } else {
          w.pushEvent(EV_SFX, e.x, e.y, 23.f, 0.6f);
        }
      }
      if (e.attackTimer <= 0.f) {
        e.attackCooldown = def.attackCooldown;
        applyState(w, e, ES_CHASE);
      }
      break;
    }
    case ES_SEARCH: {
      if (e.seesPlayer) { applyState(w, e, ES_CHASE); break; }
      f32 dd = distf(e.x, e.y, e.wanderX, e.wanderY);
      if (dd < 24.f) {
        e.searchSteps++;
        if (e.searchSteps >= 3) { applyState(w, e, ES_RETURN); break; }
        f32 ox = rng.range(-110.f, 110.f);
        f32 oy = rng.range(-90.f, 90.f);
        const u8* grid = w.rooms[e.room].navBlocked;
        f32 nx = e.lastKnownX + ox, ny = e.lastKnownY + oy;
        if (nav.walkableAt(grid, nx, ny)) { e.wanderX = nx; e.wanderY = ny; }
        e.pathLen = 0;
      }
      stepToward(w, *this, e, e.wanderX, e.wanderY, def.speed * 1.05f, dt);
      if (e.stateTimer > 14.f) applyState(w, e, ES_RETURN);
      break;
    }
    case ES_RETURN: {
      if (e.seesPlayer) { applyState(w, e, ES_CHASE); break; }
      if (e.heardNoise) { applyState(w, e, ES_INVESTIGATE); break; }
      f32 dd = distf(e.x, e.y, e.homeX, e.homeY);
      if (dd < 30.f) {
        e.alert = 0.f;
        applyState(w, e, ES_PATROL);
        break;
      }
      stepToward(w, *this, e, e.homeX, e.homeY, def.speed, dt);
      break;
    }
    default:
      break;
  }

  // ambient vocalisations keep the room feeling alive
  e.speakTimer -= dt;
  if (e.speakTimer <= 0.f) {
    f32 vol = 0.25f + e.alert / 220.f;
    w.pushEvent(EV_SFX, e.x, e.y, e.type == E_WEAVER ? 31.f : (e.type == E_HOUND ? 26.f : 25.f), vol);
    e.speakTimer = rng.range(5.f, 13.f);
  }
}

} // namespace tcs
