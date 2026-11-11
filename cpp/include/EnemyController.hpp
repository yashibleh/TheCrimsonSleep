// EnemyController: state machine + priority-queue decisions + BFS navigation
// for every enemy. Three types share the machinery but behave differently.
#pragma once
#include "core.hpp"
#include "Content.hpp"
#include "EnemyDecisionSystem.hpp"
#include "NavigationSystem.hpp"
#include "SoundEventQueue.hpp"

namespace tcs {

class GameWorldManager;

struct Enemy {
  i32 alive;
  i32 type;
  i32 room;
  f32 x, y;
  f32 vx, vy;
  f32 homeX, homeY;
  f32 patrolAnchors[6];
  i32 anchorCount;
  i32 anchorIndex;
  f32 hp, maxHp;
  i32 state;
  f32 stateTimer;
  f32 alert;
  f32 dirX, dirY;
  f32 anim;
  f32 animPhase;
  f32 attackTimer;
  f32 attackCooldown;
  f32 thinkTimer;
  f32 repathTimer;
  f32 hitFlash;
  f32 staggerTimer;
  f32 speakTimer;
  f32 searchTimer;
  i32 searchSteps;
  f32 lastKnownX, lastKnownY;
  f32 targetX, targetY;
  f32 wanderX, wanderY;
  i32 path[MAX_ENEMY_PATH];
  i32 pathLen;
  i32 pathIndex;
  u8 seesPlayer;
  u8 heardNoise;
  u8 didHit;
  f32 noiseX, noiseY, noisePower;
  i32 pendingRoom;
  f32 transferTimer;
  f32 graphTimer;   // pacing between rooms on the Weaver's world-graph pursuit
  f32 pendingX, pendingY;
  i32 killsPlayerOnTouch;
};

struct EnemyController {
  Enemy list[MAX_ENEMIES];
  i32 count;
  EnemyDecisionSystem decisions;
  NavSystem nav;
  Rng rng;

  void reset();
  i32 spawn(i32 type, i32 room, f32 x, f32 y);
  void clear();
  void addAnchor(Enemy& e, f32 x, f32 y);
  void awakeWeaver(GameWorldManager& w, i32 room, f32 x, f32 y);
  void updateAll(GameWorldManager& w, f32 dt);
  void updateEnemy(GameWorldManager& w, Enemy& e, i32 index, f32 dt);
  void think(GameWorldManager& w, Enemy& e, i32 index);
  void applyState(GameWorldManager& w, Enemy& e, i32 state);
  bool hurt(GameWorldManager& w, f32 x, f32 y, f32 radius, f32 damage, f32 fromX, f32 fromY);
  i32 aliveInRoom(i32 room) const;
  i32 nearestIndex(i32 room, f32 x, f32 y, f32 maxDist) const;
  void alertAll(i32 room, f32 x, f32 y, f32 amount);
  void transferRoom(i32 from, i32 to, f32 spawnX, f32 spawnY);
};

} // namespace tcs
