// PuzzleActionStack: LIFO history of player interactions and puzzle actions.
// Supports rollback of reversible puzzle steps (valve sequence, candle ritual)
// and keeps the last N interactions for the "recent actions" system.
#pragma once
#include "core.hpp"

namespace tcs {

enum ActionKind : i32 {
  ACTK_INTERACT = 0,
  ACTK_VALVE,
  ACTK_CANDLE,
  ACTK_FUSE,
  ACTK_CODE_DIGIT,
  ACTK_DRAWER,
  ACTK_BREAK
};

struct PuzzleAction {
  i32 kind;
  i32 target;
  i32 value;
  i32 prevValue;
  f32 x, y;
  f32 tick;
};

struct PuzzleActionStack {
  PuzzleAction items[MAX_ACTIONS];
  i32 top;   // number of items (top == next free slot)
  i32 pushes;
  i32 pops;

  void reset();
  bool push(i32 kind, i32 target, i32 value, i32 prevValue, f32 x, f32 y, f32 tick);
  bool pop(PuzzleAction& out);
  const PuzzleAction* peek() const;
  bool empty() const { return top == 0; }
  i32 size() const { return top; }
  const PuzzleAction* at(i32 i) const;
  void clear() { top = 0; }
};

} // namespace tcs
