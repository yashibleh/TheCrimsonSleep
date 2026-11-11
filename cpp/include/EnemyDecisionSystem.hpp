// EnemyDecisionSystem: binary max-heap priority queue that ranks the possible
// next action for an enemy every time it thinks.
#pragma once
#include "core.hpp"

namespace tcs {

enum EnemyAction : i32 {
  ACT_NONE = 0,
  ACT_PATROL,
  ACT_SEARCH,
  ACT_INVESTIGATE,
  ACT_CHASE,
  ACT_ATTACK,
  ACT_RETREAT,
  ACT_RETURN,
  ACT_IDLE,
  ACT_HIDE
};

struct EnemyDecision {
  i32 action;
  f32 score;
  f32 targetX;
  f32 targetY;
};

// Fixed-capacity binary heap ordered by score (highest first).
struct EnemyDecisionSystem {
  EnemyDecision heap[MAX_SOUNDS];
  i32 count;

  void reset() { count = 0; }
  bool empty() const { return count == 0; }
  i32 size() const { return count; }
  const EnemyDecision& top() const { return heap[0]; }
  void push(i32 action, f32 score, f32 tx, f32 ty);
  EnemyDecision pop();
  void clear() { count = 0; }
  void updateScore(i32 action, f32 score);
  f32 scoreOf(i32 action) const;
};

} // namespace tcs
