#include "PuzzleActionStack.hpp"

namespace tcs {

void PuzzleActionStack::reset() {
  top = 0;
  pushes = 0;
  pops = 0;
  for (i32 i = 0; i < MAX_ACTIONS; ++i) {
    items[i].kind = ACTK_INTERACT;
    items[i].target = 0;
    items[i].value = 0;
    items[i].prevValue = 0;
    items[i].x = 0.f;
    items[i].y = 0.f;
    items[i].tick = 0.f;
  }
}

bool PuzzleActionStack::push(i32 kind, i32 target, i32 value, i32 prevValue, f32 x, f32 y, f32 tick) {
  if (top >= MAX_ACTIONS) {
    // drop the oldest entry so history keeps flowing
    for (i32 i = 1; i < MAX_ACTIONS; ++i) items[i - 1] = items[i];
    top = MAX_ACTIONS - 1;
  }
  items[top].kind = kind;
  items[top].target = target;
  items[top].value = value;
  items[top].prevValue = prevValue;
  items[top].x = x;
  items[top].y = y;
  items[top].tick = tick;
  top++;
  pushes++;
  return true;
}

bool PuzzleActionStack::pop(PuzzleAction& out) {
  if (top <= 0) return false;
  out = items[--top];
  pops++;
  return true;
}

const PuzzleAction* PuzzleActionStack::peek() const {
  if (top <= 0) return 0;
  return &items[top - 1];
}

const PuzzleAction* PuzzleActionStack::at(i32 i) const {
  if (i < 0 || i >= top) return 0;
  return &items[i];
}

} // namespace tcs
