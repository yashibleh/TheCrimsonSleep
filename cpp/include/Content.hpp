// Content.hpp: shared, stable gameplay ids (also mirrored by the JS text
// tables), plus the array-driven enemy configuration.
#pragma once
#include "core.hpp"

namespace tcs {

// ------------------------------------------------------------------ rooms
enum RoomId {
  R_WARD3 = 0,
  R_CORRIDOR,
  R_NURSE,
  R_CLOSET,
  R_BOILER,
  R_FLOOD,
  R_EAST,
  R_CHAPEL,
  R_MORGUE,
  R_SLEEP,
  ROOM_COUNT
};

// ------------------------------------------------------------------ items
enum ItemId {
  IT_NONE = 0,
  IT_CROWBAR = 1,
  IT_REVOLVER = 2,
  IT_AMMO = 3,
  IT_BANDAGE = 4,
  IT_FUSE_BLUE = 5,
  IT_FUSE_RED = 6,
  IT_FUSE_BRASS = 7,
  IT_WARD_KEY = 8,
  IT_VALVE_HANDLE = 9,
  IT_BRASS_KEY = 10,
  IT_BATTERY = 11,
  IT_SEDATIVE = 12,
  IT_LOCKET = 13,
  IT_MATCHES = 14
};

// ------------------------------------------------------------------ props
enum PropType {
  PROP_NONE = 0,
  PROP_NOTE,
  PROP_CANDLE,
  PROP_LOCKER,
  PROP_BED,
  PROP_MIRROR,
  PROP_BREAKER,
  PROP_VALVE,
  PROP_CODELOCK,
  PROP_DRAWER,
  PROP_RITUAL_CANDLE,
  PROP_CRADLE,
  PROP_BODY,
  PROP_LAMP,
  PROP_SIGIL,
  PROP_BOARD,
  PROP_ORGAN,
  PROP_GURNEY,
  PROP_WINDOW,
  PROP_DOORWAY
};

// --------------------------------------------------------------- enemies
enum EnemyType {
  E_CRAWLER = 0,
  E_HOUND = 1,
  E_WEAVER = 2,
  ENEMY_TYPES
};

enum EnemyState {
  ES_IDLE = 0,
  ES_PATROL,
  ES_SUSPICIOUS,
  ES_INVESTIGATE,
  ES_CHASE,
  ES_ATTACK,
  ES_SEARCH,
  ES_RETURN,
  ENEMY_STATES
};

struct EnemyDef {
  i32 type;
  f32 speed;
  f32 chaseSpeed;
  f32 sightRange;
  f32 fovCos;        // cos of half vision angle, <0 = omnidirectional
  f32 hearing;       // radius multiplier
  f32 hp;
  f32 damage;
  f32 attackRange;
  f32 attackCooldown;
  f32 radius;
  f32 alertGain;
  f32 alertDecay;
};

static const EnemyDef ENEMY_DEFS[ENEMY_TYPES] = {
  // Crawler: nearly blind, hunts by sound, slow and fragile.
  { E_CRAWLER, 38.f, 96.f, 190.f, 0.72f, 300.f, 42.f, 12.f, 30.f, 1.55f, 13.f, 34.f, 12.f },
  // Hound: fast, long sight, bites and circles.
  { E_HOUND,   62.f, 168.f, 320.f, 0.30f, 340.f, 64.f, 16.f, 32.f, 1.15f, 14.f, 46.f, 16.f },
  // Weaver: unkillable pursuer. Sees everything, hears everything.
  { E_WEAVER,  70.f, 188.f, 380.f, -1.f,  620.f, 9999.f, 32.f, 34.f, 2.60f, 17.f, 90.f, 6.f }
};

// --------------------------------------------------------------- prompts
enum PromptId {
  PROMPT_NONE = 0,
  PROMPT_READ_NOTE,
  PROMPT_TAKE,
  PROMPT_OPEN_DOOR,
  PROMPT_USE_DOOR,
  PROMPT_LOCKED_DOOR,
  PROMPT_NEEDS_KEY,
  PROMPT_HIDE,
  PROMPT_LEAVE_HIDE,
  PROMPT_SAVE,
  PROMPT_MIRROR,
  PROMPT_BREAKER,
  PROMPT_VALVE,
  PROMPT_CODELOCK,
  PROMPT_DRAWER,
  PROMPT_CANDLE,
  PROMPT_CRADLE,
  PROMPT_BOARD,
  PROMPT_ORGAN,
  PROMPT_BODY,
  PROMPT_EXAMINE,
  PROMPT_POWER_OFF
};

// ---------------------------------------------------------------- clues
enum ClueId {
  CLUE_ADMISSION = 1,
  CLUE_WARD_LOG = 2,
  CLUE_MARA_NOTE = 3,
  CLUE_NURSE_MEMO = 4,
  CLUE_VALVE_ORDER = 5,
  CLUE_BOILER_LOG = 6,
  CLUE_FUSE_MEMO = 7,
  CLUE_FLOOD_SCRATCH = 8,
  CLUE_CHAPEL_LECTERN = 9,
  CLUE_CHAPEL_HYMN = 10,
  CLUE_CODE_HALF_A = 11,
  CLUE_CODE_HALF_B = 12,
  CLUE_MORGUE_TAG = 13,
  CLUE_ILSE_LETTER = 14,
  CLUE_DOCTOR_TAPE = 15,
  CLUE_CRIMSON_SLEEP = 16,
  CLUE_SLEEP_CHAMBER = 17,
  CLUE_HIDDEN_PANEL = 18
};

} // namespace tcs
