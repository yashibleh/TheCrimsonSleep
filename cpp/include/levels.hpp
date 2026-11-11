// levels.hpp: authored level construction entry points.
#pragma once
#include "core.hpp"

namespace tcs {

class GameWorldManager;

struct EnemySpawnDef {
  i32 type;
  f32 x, y;         // spawn point (pixels)
  f32 ax, ay;       // patrol anchor A (pixels)
  f32 bx, by;       // patrol anchor B (pixels)
};

void buildWorld(GameWorldManager& w);
i32 enemySpawnsFor(i32 room, EnemySpawnDef* out, i32 maxCount);

} // namespace tcs
