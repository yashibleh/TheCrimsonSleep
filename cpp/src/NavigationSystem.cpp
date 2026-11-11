#include "NavigationSystem.hpp"

namespace tcs {

void NavSystem::reset() {
  stamp = 0;
  for (i32 i = 0; i < CELLS; ++i) {
    parent[i] = 0;
    queue[i] = 0;
    visited[i] = 0;
  }
}

// Grids passed here are *blocked masks*: 0 = open, non-zero = impassable.
bool NavSystem::walkableCell(const u8* grid, i32 cx, i32 cy) const {
  if (cx < 0 || cy < 0 || cx >= ROOM_W || cy >= ROOM_H) return false;
  return grid[cy * ROOM_W + cx] == 0;
}

bool NavSystem::walkableAt(const u8* grid, f32 x, f32 y) const {
  return walkableCell(grid, (i32)(x / (f32)TILE), (i32)(y / (f32)TILE));
}

i32 NavSystem::findCellPath(const u8* grid, f32 sx, f32 sy, f32 tx, f32 ty,
                            i32* out, i32 maxCells) {
  if (!out || maxCells <= 0) return 0;
  i32 scx = imax(0, imin(ROOM_W - 1, (i32)(sx / (f32)TILE)));
  i32 scy = imax(0, imin(ROOM_H - 1, (i32)(sy / (f32)TILE)));
  i32 tcx = imax(0, imin(ROOM_W - 1, (i32)(tx / (f32)TILE)));
  i32 tcy = imax(0, imin(ROOM_H - 1, (i32)(ty / (f32)TILE)));

  if (!walkableCell(grid, tcx, tcy)) {
    i32 best = -1;
    f32 bestD = 1e9f;
    for (i32 dy = -1; dy <= 1; ++dy) {
      for (i32 dx = -1; dx <= 1; ++dx) {
        i32 nx = tcx + dx, ny = tcy + dy;
        if (!walkableCell(grid, nx, ny)) continue;
        f32 d = (f32)(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = ny * ROOM_W + nx; }
      }
    }
    if (best < 0) return 0;
    tcx = best % ROOM_W;
    tcy = best / ROOM_W;
  }
  if (!walkableCell(grid, scx, scy)) {
    i32 best = -1;
    f32 bestD = 1e9f;
    for (i32 dy = -1; dy <= 1; ++dy) {
      for (i32 dx = -1; dx <= 1; ++dx) {
        i32 nx = scx + dx, ny = scy + dy;
        if (!walkableCell(grid, nx, ny)) continue;
        f32 d = (f32)(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = ny * ROOM_W + nx; }
      }
    }
    if (best < 0) return 0;
    scx = best % ROOM_W;
    scy = best / ROOM_W;
  }

  i32 start = scy * ROOM_W + scx;
  i32 goal = tcy * ROOM_W + tcx;
  if (start == goal) { out[0] = goal; return 1; }

  stamp++;
  i32 head = 0, tail = 0;
  queue[tail++] = start;
  visited[start] = stamp;
  parent[start] = (u16)start;

  static const i32 NDX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
  static const i32 NDY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

  bool found = false;
  while (head < tail) {
    i32 cur = queue[head++];
    if (cur == goal) { found = true; break; }
    i32 cx = cur % ROOM_W, cy = cur / ROOM_W;
    for (i32 n = 0; n < 8; ++n) {
      i32 nx = cx + NDX[n], ny = cy + NDY[n];
      if (!walkableCell(grid, nx, ny)) continue;
      if (NDX[n] != 0 && NDY[n] != 0) {
        // no corner cutting
        if (!walkableCell(grid, cx + NDX[n], cy)) continue;
        if (!walkableCell(grid, cx, cy + NDY[n])) continue;
      }
      i32 ni = ny * ROOM_W + nx;
      if (visited[ni] == stamp) continue;
      visited[ni] = stamp;
      parent[ni] = (u16)cur;
      queue[tail++] = ni;
      if (tail >= CELLS) break;
    }
  }
  if (!found) return 0;

  i32 reversed[CELLS];
  i32 n = 0;
  i32 cur = goal;
  while (n < CELLS) {
    reversed[n++] = cur;
    if (cur == start) break;
    i32 p = parent[cur];
    if (p == cur) break;
    cur = p;
  }
  i32 len = n < maxCells ? n : maxCells;
  for (i32 i = 0; i < len; ++i) out[i] = reversed[n - 1 - i];
  return len;
}

bool NavSystem::losClear(const u8* grid, f32 ax, f32 ay, f32 bx, f32 by) const {
  f32 dx = bx - ax, dy = by - ay;
  f32 d = sqrtf_(dx * dx + dy * dy);
  if (d < 1.f) return true;
  i32 steps = (i32)(d / 5.f);
  if (steps < 1) steps = 1;
  f32 stepX = dx / (f32)steps;
  f32 stepY = dy / (f32)steps;
  f32 x = ax, y = ay;
  for (i32 i = 0; i < steps; ++i) {
    x += stepX;
    y += stepY;
    i32 cx = (i32)(x / (f32)TILE), cy = (i32)(y / (f32)TILE);
    if (cx < 0 || cy < 0 || cx >= ROOM_W || cy >= ROOM_H) return false;
    if (grid[cy * ROOM_W + cx] != 0) return false;
  }
  return true;
}

f32 NavSystem::noiseDistance(f32 ax, f32 ay, f32 bx, f32 by) const {
  return distf(ax, ay, bx, by);
}

} // namespace tcs
