// PlayerState: fixed-size arrays for player statistics, timers and animation
// frame data. No vectors - arrays are the game data.
#pragma once
#include "core.hpp"

namespace tcs {

enum PlayerStatIndex : i32 {
  STAT_HP = 0,
  STAT_HP_MAX,
  STAT_STAMINA,
  STAT_STAMINA_MAX,
  STAT_BATTERY,
  STAT_BATTERY_MAX,
  STAT_MOVE_SPEED,
  STAT_RUN_SPEED,
  STAT_NOISE,
  STAT_DAMAGE_FLASH,
  STAT_INVULN,
  STAT_AMMO_IN_MAG,
  STAT_AMMO_MAX_MAG,
  STAT_STAT_COUNT
};

enum PlayerFlagIndex : i32 {
  PF_HAS_CROWBAR = 0,
  PF_HAS_REVOLVER,
  PF_FLASHLIGHT_ON,
  PF_HIDING,
  PF_SPRINTING,
  PF_DEAD,
  PF_POWER_ON,
  PF_VALVES_DONE,
  PF_CODE_DONE,
  PF_MORGUE_OPEN,
  PF_WEAVER_AWAKE,
  PF_ENDING,
  PF_FLAG_COUNT
};

struct PlayerState {
  f32 stats[STAT_STAT_COUNT];
  u8  flags[PF_FLAG_COUNT];
  f32 x, y;
  f32 vx, vy;
  f32 dirX, dirY;         // facing unit vector
  f32 animTime;
  f32 walkPhase;
  f32 attackTimer;        // >0 while attack animation plays
  f32 attackCooldown;
  f32 reloadTimer;
  f32 footstepTimer;
  f32 heartbeat;
  i32 weapon;              // 1 crowbar, 2 revolver
  i32 room;
  i32 hideProp;            // index of hiding prop, -1 none
  i32 damageDealt;
  i32 kills;

  void reset();
  bool hasFlag(i32 f) const { return flags[f] != 0; }
  void setFlag(i32 f, bool v) { flags[f] = v ? 1 : 0; }
  f32 stat(i32 i) const { return stats[i]; }
  void setStat(i32 i, f32 v) { stats[i] = v; }
  void damage(f32 amount);
  bool alive() const { return stats[STAT_HP] > 0.f && !flags[PF_DEAD]; }
};

} // namespace tcs
