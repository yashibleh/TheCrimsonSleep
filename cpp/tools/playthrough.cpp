// playthrough.cpp - a headless player that completes the actual game against the
// real C++ simulation. Used to balance enemy pressure and the survival economy:
// it walks the authored route, takes items, solves the puzzles, fights what it
// must and flees the Weaver, then reports damage, ammo, time and outcome.
//
// Build:  g++ -std=c++17 -O2 -Icpp/include cpp/src/*.cpp cpp/tools/playthrough.cpp -o build/playthrough
// Run:    ./build/playthrough [runs]
#include "GameWorldManager.hpp"
#include "Content.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace tcs;

namespace {

GameWorldManager world;
Rng botRng;

f32 simTime = 0.f;
f32 damageTaken = 0.f;
f32 lastHp = 100.f;
i32 shotsFired = 0;
i32 meleeSwings = 0;
i32 kills = 0;
i32 lastKillCount = 0;
f32 chaseTime = 0.f;
f32 dangerIntegral = 0.f;
f32 lastMag = 0.f;
i32 damageByType[ENEMY_TYPES] = { 0, 0, 0 };
i32 reachedStep = 0;
i32 deathRoom = -1;
i32 hideCount = 0;

f32 distTo(f32 x, f32 y);
Enemy* nearestThreat(f32 maxDist, bool includeWeaver = true);
bool losTo(Enemy& e);
void combatStep(Enemy& e);
void manageSurvival();
bool hideUntilSafe(f32 maxSeconds);

const f32 DT = 1.f / 60.f;

void tick(i32 input) {
  i32 magBefore = (i32)world.player.stats[STAT_AMMO_IN_MAG];
  world.update(DT, input);
  simTime += DT;
  if ((i32)world.player.stats[STAT_AMMO_IN_MAG] < magBefore) shotsFired++;
  f32 hp = world.player.stats[STAT_HP];
  if (hp < lastHp) {
    damageTaken += lastHp - hp;
    i32 best = -1;
    f32 bestD = 1e9f;
    for (i32 i = 0; i < world.enemies.count; ++i) {
      Enemy& e = world.enemies.list[i];
      if (!e.alive || e.room != world.room) continue;
      f32 d = distTo(e.x, e.y);
      if (d < bestD) { bestD = d; best = i; }
    }
    if (best >= 0) damageByType[world.enemies.list[best].type] += (i32)(lastHp - hp);
  }
  lastHp = hp;
  if (world.player.attackTimer > 0.29f && world.player.weapon == 1) meleeSwings++;
  if (world.player.kills > lastKillCount) {
    kills += world.player.kills - lastKillCount;
    lastKillCount = world.player.kills;
  }
  dangerIntegral += world.danger;
  bool chased = false;
  for (i32 i = 0; i < world.enemies.count; ++i) {
    Enemy& e = world.enemies.list[i];
    if (e.alive && e.room == world.room && (e.state == ES_CHASE || e.state == ES_ATTACK)) chased = true;
  }
  if (chased) chaseTime += DT;
}

f32 distTo(f32 x, f32 y) {
  f32 dx = world.player.x - x, dy = world.player.y - y;
  return sqrtf(dx * dx + dy * dy);
}

// walk towards a point; returns true when close enough
bool walkTo(f32 tx, f32 ty, f32 seconds) {
  f32 deadline = simTime + seconds;
  f32 lastX = world.player.x, lastY = world.player.y;
  f32 stuckTimer = 0.f, sideTimer = 0.f;
  f32 sideX = 0.f, sideY = 0.f;
  while (simTime < deadline && world.state == GS_PLAYING) {
    if (distTo(tx, ty) < 10.f) return true;
    // a real player clears what it can see on the way, and uses supplies
    Enemy* threat = nearestThreat(260.f);
    if (threat && threat->type != E_WEAVER && losTo(*threat)) {
      combatStep(*threat);
      continue;
    }
    manageSurvival();
    f32 dx = tx - world.player.x, dy = ty - world.player.y;
    f32 len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return true;
    dx /= len; dy /= len;
    i32 in = 0;
    if (dx > 0.38f) in |= IN_RIGHT;
    if (dx < -0.38f) in |= IN_LEFT;
    if (dy > 0.38f) in |= IN_DOWN;
    if (dy < -0.38f) in |= IN_UP;
    if (sideTimer > 0.f) {
      sideTimer -= DT;
      in = 0;
      if (sideX > 0.f) in |= IN_RIGHT; else in |= IN_LEFT;
      if (sideY > 0.f) in |= IN_DOWN; else in |= IN_UP;
    }
    // sprint across open ground, or whenever the Weaver is closing
    bool weaverNear = false;
    for (i32 i = 0; i < world.enemies.count; ++i) {
      Enemy& e = world.enemies.list[i];
      if (e.alive && e.type == E_WEAVER && distTo(e.x, e.y) < 340.f) weaverNear = true;
    }
    if ((len > 90.f || weaverNear) && world.player.stats[STAT_STAMINA] > 12.f) in |= IN_SPRINT;
    if (weaverNear && hideCount < 8 && !world.player.hasFlag(PF_HIDING)) {
      if (hideUntilSafe(11.f)) continue;
    }
    tick(in);
    stuckTimer += DT;
    if (stuckTimer > 0.45f) {
      f32 moved = sqrtf((world.player.x - lastX) * (world.player.x - lastX) +
                        (world.player.y - lastY) * (world.player.y - lastY));
      if (moved < 5.f) {
        f32 ang = botRng.range(0.f, 6.28318f);
        sideX = cosf(ang);
        sideY = sinf(ang);
        sideTimer = 0.35f;
      }
      stuckTimer = 0.f;
      lastX = world.player.x;
      lastY = world.player.y;
    }
  }
  return distTo(tx, ty) < 14.f;
}

Enemy* nearestThreat(f32 maxDist, bool includeWeaver) {
  Enemy* best = 0;
  f32 bestD = maxDist;
  for (i32 i = 0; i < world.enemies.count; ++i) {
    Enemy& e = world.enemies.list[i];
    if (!e.alive || e.room != world.room) continue;
    if (!includeWeaver && e.type == E_WEAVER) continue;
    f32 d = distTo(e.x, e.y);
    if (d < bestD) { bestD = d; best = &e; }
  }
  return best;
}

bool losTo(Enemy& e) {
  return world.nav.losClear(world.rooms[world.room].navBlocked, world.player.x, world.player.y, e.x, e.y);
}

// What a reasonably skilled player does when something is closing in.
void combatStep(Enemy& e) {
  f32 d = distTo(e.x, e.y);
  if (e.type == E_WEAVER) return;   // unkillable: never waste ammo on it

  if (world.player.weapon != 2 && world.inv.quantity(IT_AMMO) > 0) tick(IN_SWAP);
  if (world.player.weapon == 2) {
    if (world.player.stats[STAT_AMMO_IN_MAG] <= 0.f) {
      if (world.inv.quantity(IT_AMMO) > 0) { tick(IN_RELOAD); return; }
      tick(IN_SWAP);
      return;
    }
    // face the target (direction comes from movement), then fire
    i32 aim = 0;
    if (e.x > world.player.x + 6.f) aim |= IN_RIGHT;
    if (e.x < world.player.x - 6.f) aim |= IN_LEFT;
    if (e.y > world.player.y + 6.f) aim |= IN_DOWN;
    if (e.y < world.player.y - 6.f) aim |= IN_UP;
    tick(aim);
    tick(IN_ATTACK);
    return;
  }
  // melee
  if (d > 34.f) {
    i32 in = 0;
    if (e.x > world.player.x) in |= IN_RIGHT; else in |= IN_LEFT;
    if (e.y > world.player.y) in |= IN_DOWN; else in |= IN_UP;
    tick(in);
  } else {
    tick(IN_ATTACK);
  }
}

void manageSurvival() {
  // heal when it is safe-ish
  if (world.player.stats[STAT_HP] < 52.f && world.inv.quantity(IT_BANDAGE) > 0) {
    Enemy* near = nearestThreat(150.f);
    if (!near || near->type == E_WEAVER) {
      tick(IN_HEAL);
      return;
    }
  }
  if (world.player.weapon == 2 && world.player.stats[STAT_AMMO_IN_MAG] <= 0.f &&
      world.inv.quantity(IT_AMMO) > 0 && !nearestThreat(90.f)) {
    tick(IN_RELOAD);
  }
}

// keep fighting while something is close and visible, but do not stall the route
void handleThreats(f32 radius) {
  Enemy* e = nearestThreat(radius);
  if (!e) return;
  if (!losTo(*e) && distTo(e->x, e->y) > 120.f) return;
  combatStep(*e);
}

bool interact(f32 seconds = 6.f) {
  f32 deadline = simTime + seconds;
  while (simTime < deadline && world.state == GS_PLAYING) {
    if (world.prompt.target == -1) return false;
    tick(IN_INTERACT);
    return true;
  }
  return false;
}

i32 findItem(i32 room, i32 itemId) {
  RoomData& r = world.rooms[room];
  for (i32 i = 0; i < r.itemCount; ++i)
    if (!r.items[i].taken && r.items[i].itemId == itemId) return i;
  return -1;
}

i32 findProp(i32 room, i32 labelId, i32 propId) {
  RoomData& r = world.rooms[room];
  for (i32 i = 0; i < r.propCount; ++i) {
    if (r.props[i].labelId != labelId) continue;
    if (propId >= 0 && r.props[i].id != propId) continue;
    if (r.props[i].state != 0) continue;
    return i;
  }
  return -1;
}

// Get into a locker and wait out the Weaver. Hiding breaks its line of sight,
// which is the only thing that makes its alert decay while it is in the room.
bool hideUntilSafe(f32 maxSeconds) {
  i32 idx = findProp(world.room, PROMPT_HIDE, -1);
  if (idx < 0) return false;
  RoomProp& p = world.rooms[world.room].props[idx];
  f32 lx = p.x, ly = p.y;
  hideCount++;
  f32 deadline = simTime + 10.f;
  while (simTime < deadline && world.state == GS_PLAYING) {
    if (world.prompt.target == idx) break;
    f32 dx = lx - world.player.x, dy = ly - world.player.y;
    f32 len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) break;
    dx /= len; dy /= len;
    i32 in = 0;
    if (dx > 0.35f) in |= IN_RIGHT;
    if (dx < -0.35f) in |= IN_LEFT;
    if (dy > 0.35f) in |= IN_DOWN;
    if (dy < -0.35f) in |= IN_UP;
    tick(in);
  }
  if (world.prompt.target != idx) return false;
  tick(IN_INTERACT);
  f32 waited = 0.f;
  while (waited < maxSeconds && world.state == GS_PLAYING && world.player.hasFlag(PF_HIDING)) {
    tick(0);
    waited += DT;
    Enemy* wv = nearestThreat(500.f);
    if (!wv || wv->type != E_WEAVER || wv->alert < 20.f) break;
  }
  tick(IN_INTERACT);
  return true;
}

bool takeItem(i32 itemId) {
  for (i32 attempt = 0; attempt < 4; ++attempt) {
    i32 idx = findItem(world.room, itemId);
    if (idx < 0) return false;
    RoomItem& it = world.rooms[world.room].items[idx];
    walkTo(it.x, it.y, 22.f);
    if (world.state != GS_PLAYING) return false;
    handleThreats(120.f);
    if (world.prompt.target == -1000 - idx) {
      tick(IN_INTERACT);
      if (world.rooms[world.room].items[idx].taken) return true;
    } else {
      tick(0);
    }
  }
  return false;
}

bool useProp(i32 labelId, i32 propId = -1, i32 seconds = 20.f) {
  i32 idx = findProp(world.room, labelId, propId);
  if (idx < 0) return false;
  RoomProp& p = world.rooms[world.room].props[idx];
  f32 px = p.x, py = p.y;
  walkTo(px, py + 26.f, seconds);
  if (world.state != GS_PLAYING) return false;
  handleThreats(110.f);
  if (world.prompt.target == idx) {
    tick(IN_INTERACT);
    return true;
  }
  return false;
}

bool goToRoom(i32 target) {
  for (i32 hop = 0; hop < 12; ++hop) {
    if (world.room == target) return true;
    if (world.state != GS_PLAYING) return false;
    i32 next = world.nextRoomToward(world.room, target);
    if (next < 0) return false;
    RoomData& r = world.rooms[world.room];
    i32 doorIdx = -1;
    for (i32 d = 0; d < r.doorCount; ++d)
      if (r.doors[d].targetRoom == next) { doorIdx = d; break; }
    if (doorIdx < 0) return false;
    RoomDoor door = r.doors[doorIdx];
    i32 before = world.room;
    for (i32 attempt = 0; attempt < 5 && world.room == before; ++attempt) {
      walkTo(door.x, door.y, 26.f);
      if (world.state != GS_PLAYING) return false;
      handleThreats(130.f);
      if (world.room != before) break;
      if (world.prompt.target == doorIdx) {
        tick(IN_INTERACT);
        if (!world.doorTable.unlocked[door.id]) {
          // locked: try the key, or the puzzle that opens it
          world.unlockDoor(doorIdx);
          tick(0);
        }
      } else {
        tick(0);
      }
    }
    if (world.room == before) return false;
  }
  return world.room == target;
}

struct RunResult {
  bool finished = false;
  f32 time = 0.f;
  f32 damage = 0.f;
  f32 hpLeft = 0.f;
  i32 deaths = 0;
  i32 ammoLeft = 0;
  i32 shots = 0;
  i32 kills = 0;
  f32 chase = 0.f;
  i32 ending = 0;
  i32 step = 0;
  i32 room = -1;
};

RunResult playOnce(u32 seed) {
  botRng.seed(seed);
  world.init();
  world.newGame();
  simTime = 0.f;
  damageTaken = 0.f;
  lastHp = world.player.stats[STAT_HP];
  shotsFired = 0;
  meleeSwings = 0;
  kills = 0;
  lastKillCount = 0;
  chaseTime = 0.f;
  dangerIntegral = 0.f;
  reachedStep = 0;
  for (i32 i = 0; i < ENEMY_TYPES; ++i) damageByType[i] = 0;

  struct Step { i32 kind; i32 arg; i32 room; };
  enum { S_GO, S_TAKE, S_PROP, S_VALVES, S_BREAKER, S_CODE, S_CRADLE };
  static const Step ROUTE[] = {
    { S_TAKE, IT_BANDAGE, R_WARD3 },
    { S_GO, 0, R_NURSE },
    { S_TAKE, IT_WARD_KEY, R_NURSE },
    { S_TAKE, IT_REVOLVER, R_NURSE },
    { S_TAKE, IT_AMMO, R_NURSE },
    { S_TAKE, IT_SEDATIVE, R_NURSE },
    { S_PROP, PROMPT_DRAWER, R_NURSE },      // bandage in a drawer
    { S_GO, 0, R_CLOSET },
    { S_TAKE, IT_CROWBAR, R_CLOSET },
    { S_TAKE, IT_FUSE_BLUE, R_CLOSET },
    { S_TAKE, IT_AMMO, R_CLOSET },
    { S_GO, 0, R_BOILER },
    { S_TAKE, IT_VALVE_HANDLE, R_BOILER },
    { S_TAKE, IT_AMMO, R_BOILER },
    { S_VALVES, 0, R_BOILER },
    { S_GO, 0, R_FLOOD },
    { S_TAKE, IT_FUSE_RED, R_FLOOD },
    { S_TAKE, IT_AMMO, R_FLOOD },
    { S_GO, 0, R_EAST },
    { S_TAKE, IT_FUSE_BRASS, R_EAST },
    { S_TAKE, IT_BANDAGE, R_EAST },
    { S_TAKE, IT_AMMO, R_EAST },
    { S_GO, 0, R_CHAPEL },
    { S_TAKE, IT_AMMO, R_CHAPEL },
    { S_TAKE, IT_BANDAGE, R_CHAPEL },
    { S_GO, 0, R_BOILER },                   // back for the breaker
    { S_BREAKER, 0, R_BOILER },
    { S_GO, 0, R_CHAPEL },
    { S_CODE, 0, R_CHAPEL },
    { S_GO, 0, R_MORGUE },
    { S_PROP, PROMPT_DRAWER, R_MORGUE },
    { S_PROP, PROMPT_DRAWER, R_MORGUE },
    { S_PROP, PROMPT_DRAWER, R_MORGUE },
    { S_PROP, PROMPT_DRAWER, R_MORGUE },
    { S_TAKE, IT_AMMO, R_MORGUE },
    { S_GO, 0, R_CHAPEL },
    { S_GO, 0, R_SLEEP },
    { S_CRADLE, 0, R_SLEEP },
  };
  const i32 stepCount = (i32)(sizeof(ROUTE) / sizeof(ROUTE[0]));

  for (i32 s = 0; s < stepCount; ++s) {
    if (world.state != GS_PLAYING) break;
    reachedStep = s;
    const Step& st = ROUTE[s];
    if (st.kind == S_GO) {
      goToRoom(st.room);
      continue;
    }
    if (st.kind == S_TAKE) { takeItem(st.arg); continue; }
    if (st.kind == S_PROP) { useProp(st.arg); continue; }
    if (st.kind == S_VALVES) {
      static const i32 order[3] = { 2, 0, 1 };
      for (i32 v = 0; v < 3; ++v) {
        for (i32 tries = 0; tries < 3 && world.valveProgress <= v; ++tries) useProp(PROMPT_VALVE, order[v]);
      }
      continue;
    }
    if (st.kind == S_BREAKER) {
      for (i32 tries = 0; tries < 8 && world.fuseMask != 7; ++tries) useProp(PROMPT_BREAKER);
      continue;
    }
    if (st.kind == S_CODE) {
      useProp(PROMPT_CODELOCK);
      if (world.uiRequest) {
        world.codeDigit(0); tick(0);
        world.codeDigit(3); tick(0);
        world.codeDigit(1); tick(0);
        world.codeDigit(7); tick(0);
        world.uiRequest = 0;
      }
      continue;
    }
    if (st.kind == S_CRADLE) {
      i32 idx = findProp(world.room, PROMPT_CRADLE, -1);
      if (idx >= 0) {
        RoomProp& p = world.rooms[world.room].props[idx];
        walkTo(p.x, p.y + 52.f, 26.f);
        if (world.prompt.target == idx) tick(IN_INTERACT);
        for (i32 i = 0; i < 120 && world.state == GS_PLAYING; ++i) tick(0);
      }
    }
  }

  RunResult r;
  r.finished = world.state == GS_ENDING;
  r.time = simTime;
  r.damage = damageTaken;
  r.hpLeft = world.player.stats[STAT_HP];
  r.deaths = world.state == GS_DEAD ? 1 : 0;
  r.ammoLeft = world.inv.quantity(IT_AMMO) + (i32)world.player.stats[STAT_AMMO_IN_MAG];
  r.shots = shotsFired;
  r.kills = kills;
  r.chase = chaseTime;
  r.ending = world.endingId;
  r.step = reachedStep;
  r.room = world.room;
  return r;
}

} // namespace

int main(int argc, char** argv) {
  i32 runs = argc > 1 ? atoi(argv[1]) : 5;
  if (runs < 1) runs = 1;
  printf("THE CRIMSON SLEEP - automated playthroughs (real C++ core)\n");
  printf("run  result   time   damage  hp   ammo  shots kills  chased  ending\n");
  i32 wins = 0, deaths = 0;
  f32 totalTime = 0.f, totalDamage = 0.f, totalHp = 0.f, totalChase = 0.f;
  i32 totalShots = 0, totalKills = 0, totalAmmo = 0;
  for (i32 i = 0; i < runs; ++i) {
    RunResult r = playOnce(9001u + (u32)i * 733u);
    if (r.finished) wins++;
    if (r.deaths) deaths++;
    totalTime += r.time;
    totalDamage += r.damage;
    totalHp += r.hpLeft;
    totalChase += r.chase;
    totalShots += r.shots;
    totalKills += r.kills;
    totalAmmo += r.ammoLeft;
    printf("%3d  %-8s %5.1fs %6.1f  %5.1f %4d  %4d  %4d  %5.1fs  %d  step %d room %d\n",
           i + 1, r.finished ? "ESCAPED" : (r.deaths ? "DIED" : "STUCK"),
           r.time, r.damage, r.hpLeft, r.ammoLeft, r.shots, r.kills, r.chase, r.ending,
           r.step, r.room);
  }
  printf("\n----------------------------------------\n");
  printf("completed:      %d/%d\n", wins, runs);
  printf("deaths:         %d/%d\n", deaths, runs);
  printf("avg time:       %.1fs\n", totalTime / runs);
  printf("avg damage:     %.1f\n", totalDamage / runs);
  printf("avg hp left:    %.1f\n", totalHp / runs);
  printf("avg ammo left:  %.1f\n", (f32)totalAmmo / runs);
  printf("avg shots:      %.1f\n", (f32)totalShots / runs);
  printf("avg kills:      %.1f\n", (f32)totalKills / runs);
  printf("avg chased:     %.1fs\n", totalChase / runs);
  printf("damage by type: crawler %d, hound %d, weaver %d\n",
         damageByType[E_CRAWLER], damageByType[E_HOUND], damageByType[E_WEAVER]);
  return wins == runs ? 0 : 1;
}
