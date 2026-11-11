// NavigationSystem: BFS over the room tile grid plus line-of-sight raycasts.
// Every enemy walks a real path produced here.
#pragma once
#include "core.hpp"

namespace tcs {

struct NavSystem {
  u16 parent[CELLS];
  i32 queue[CELLS];
  u32 visited[CELLS];
  u32 stamp;

  void reset();
  bool walkableAt(const u8* tiles, f32 x, f32 y) const;
  bool walkableCell(const u8* tiles, i32 cx, i32 cy) const;
  i32 findCellPath(const u8* tiles, f32 sx, f32 sy, f32 tx, f32 ty, i32* out, i32 maxCells);
  bool losClear(const u8* tiles, f32 ax, f32 ay, f32 bx, f32 by) const;
  f32 noiseDistance(f32 ax, f32 ay, f32 bx, f32 by) const;
};

} // namespace tcs
