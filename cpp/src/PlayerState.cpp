#include "PlayerState.hpp"

namespace tcs {

void PlayerState::reset() {
  for (i32 i = 0; i < STAT_STAT_COUNT; ++i) stats[i] = 0.f;
  for (i32 i = 0; i < PF_FLAG_COUNT; ++i) flags[i] = 0;
  stats[STAT_HP] = 100.f;
  stats[STAT_HP_MAX] = 100.f;
  stats[STAT_STAMINA] = 100.f;
  stats[STAT_STAMINA_MAX] = 100.f;
  stats[STAT_BATTERY] = 100.f;
  stats[STAT_BATTERY_MAX] = 100.f;
  stats[STAT_MOVE_SPEED] = 124.f;
  stats[STAT_RUN_SPEED] = 214.f;
  stats[STAT_NOISE] = 0.f;
  stats[STAT_DAMAGE_FLASH] = 0.f;
  stats[STAT_INVULN] = 0.f;
  stats[STAT_AMMO_IN_MAG] = 0.f;
  stats[STAT_AMMO_MAX_MAG] = 6.f;
  x = 0.f; y = 0.f;
  vx = 0.f; vy = 0.f;
  dirX = 0.f; dirY = 1.f;
  animTime = 0.f;
  walkPhase = 0.f;
  attackTimer = 0.f;
  attackCooldown = 0.f;
  reloadTimer = 0.f;
  footstepTimer = 0.f;
  heartbeat = 0.f;
  weapon = 1;
  room = 0;
  hideProp = -1;
  damageDealt = 0;
  kills = 0;
}

void PlayerState::damage(f32 amount) {
  if (flags[PF_DEAD]) return;
  if (stats[STAT_INVULN] > 0.f) return;
  stats[STAT_HP] -= amount;
  stats[STAT_DAMAGE_FLASH] = 1.f;
  stats[STAT_INVULN] = 0.55f;
  if (stats[STAT_HP] <= 0.f) {
    stats[STAT_HP] = 0.f;
    flags[PF_DEAD] = 1;
  }
}

} // namespace tcs
