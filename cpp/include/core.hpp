// THE CRIMSON SLEEP - core types (freestanding C++17, no STL, no libc)
#pragma once

namespace tcs {

typedef unsigned char      u8;
typedef signed char        i8;
typedef unsigned short     u16;
typedef short              i16;
typedef unsigned int       u32;
typedef int                i32;
typedef float              f32;

// ---------------------------------------------------------------- constants
static const i32 TILE        = 32;
static const i32 ROOM_W      = 40;
static const i32 ROOM_H      = 22;
static const i32 MAX_ROOMS   = 12;
static const i32 MAX_ROOM_DOORS = 12;
static const i32 MAX_ENEMIES = 16;
static const i32 MAX_ITEMS   = 80;
static const i32 MAX_PROPS   = 56;
static const i32 MAX_LIGHTS  = 24;
static const i32 MAX_CLUES   = 48;
static const i32 MAX_INV     = 24;
static const i32 MAX_EVENTS  = 96;
static const i32 MAX_SOUNDS  = 48;
static const i32 MAX_ACTIONS = 48;
static const i32 MAX_ENEMY_PATH = 48;
static const i32 MAX_TRIGGERS = 96;
static const i32 CELLS       = ROOM_W * ROOM_H;

// ---------------------------------------------------------------- math
inline f32 clampf(f32 v, f32 a, f32 b) { return v < a ? a : (v > b ? b : v); }
inline f32 minf(f32 a, f32 b) { return a < b ? a : b; }
inline f32 maxf(f32 a, f32 b) { return a > b ? a : b; }
inline f32 absf(f32 v) { return v < 0.f ? -v : v; }
inline f32 signf(f32 v) { return v < 0.f ? -1.f : 1.f; }
inline i32 imin(i32 a, i32 b) { return a < b ? a : b; }
inline i32 imax(i32 a, i32 b) { return a > b ? a : b; }
inline i32 iabs(i32 v) { return v < 0 ? -v : v; }

// Newton sqrt: keeps us free of libm while staying precise enough for gameplay.
inline f32 sqrtf_(f32 v) {
  if (v <= 0.f) return 0.f;
  f32 x = v > 1.f ? v : 1.f;
  for (i32 i = 0; i < 18; ++i) x = 0.5f * (x + v / x);
  return x;
}
inline f32 dist2(f32 ax, f32 ay, f32 bx, f32 by) {
  f32 dx = ax - bx, dy = ay - by;
  return dx * dx + dy * dy;
}
inline f32 distf(f32 ax, f32 ay, f32 bx, f32 by) { return sqrtf_(dist2(ax, ay, bx, by)); }
inline f32 lerpf(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
inline f32 approachf(f32 v, f32 target, f32 step) {
  if (v < target) { v += step; return v > target ? target : v; }
  v -= step; return v < target ? target : v;
}

struct Rng {
  u32 s;
  void seed(u32 v) { s = v ? v : 0x9E3779B9u; }
  u32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  f32 unit() { return (f32)(next() & 0xFFFFFFu) / 16777216.f; }
  f32 range(f32 a, f32 b) { return a + (b - a) * unit(); }
  i32 irange(i32 a, i32 b) { return a + (i32)(next() % (u32)(b - a + 1)); }
};

// ---------------------------------------------------------------- tiles
enum TileType : u8 {
  T_VOID = 0,   // bottomless dark / out of bounds
  T_WALL,
  T_WOOD,       // old floorboards
  T_STONE,      // basement stone
  T_TILE,       // clinical white tile
  T_CARPET,     // crimson runner
  T_WATER,      // shallow standing water (slow, noisy)
  T_BLOOD,      // pooled blood (slow, leaves prints)
  T_RUBBLE,     // collapsed debris (solid)
  T_GRASS,
  T_STAIRS,     // walkable stairwell marker
  T_METAL       // rusted metal grating
};

inline bool tileSolid(u8 t) { return t == T_VOID || t == T_WALL || t == T_RUBBLE; }
inline bool tileSlow(u8 t) { return t == T_WATER || t == T_BLOOD; }
inline bool tileSightBlocking(u8 t) { return t == T_VOID || t == T_WALL || t == T_RUBBLE; }

// Door ids are flat: room * MAX_ROOM_DOORS + index.
inline i32 doorKey(i32 room, i32 idx) { return room * MAX_ROOM_DOORS + idx; }

struct DoorTable {
  u8 open[MAX_ROOMS * MAX_ROOM_DOORS];
  u8 unlocked[MAX_ROOMS * MAX_ROOM_DOORS];
  void reset() {
    for (i32 i = 0; i < MAX_ROOMS * MAX_ROOM_DOORS; ++i) { open[i] = 0; unlocked[i] = 0; }
  }
};

// ---------------------------------------------------------------- events
// Events are produced by the C++ simulation and consumed by the renderer/audio
// layer. All fields are f32 so JavaScript can read them from one Float32Array.
enum EventType : u8 {
  EV_NONE = 0,
  EV_SFX,          // a = sfx id, b = volume
  EV_SHAKE,        // a = strength, b = duration
  EV_FLASH,        // a = alpha, b = colour id
  EV_TEXT,         // a = string id, b = duration ms
  EV_JUMPSCARE,    // a = scare id
  EV_QUEST,        // a = string id (subtitle line)
  EV_DEATH,
  EV_SAVE,
  EV_CLUE,         // a = clue id
  EV_ROOM,         // a = room id
  EV_TRANSITION,   // a = from room, b = to room
  EV_ENDING,       // a = ending id
  EV_PICKUP,       // a = item id, b = qty
  EV_HIT,          // a = damage
  EV_ENEMY_ALERT   // a = enemy type, b = alert level
};

struct GameEvent { f32 type, x, y, a, b; };

// SoundEvent: created by running, shooting, doors, breaking, dropping objects.
struct SoundEvent {
  f32 x, y;
  f32 intensity;   // 0..1
  f32 type;        // 0 step,1 shot,2 door,3 break,4 drop,5 scream,6 pickup,7 water
  f32 tick;        // simulation time in seconds
  u8  consumedBy[MAX_ENEMIES]; // per-listener bookkeeping for FIFO drains
};

} // namespace tcs
