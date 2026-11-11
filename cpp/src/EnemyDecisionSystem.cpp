#include "EnemyDecisionSystem.hpp"

namespace tcs {

static void heapSwap(EnemyDecision& a, EnemyDecision& b) {
  EnemyDecision t = a;
  a = b;
  b = t;
}

void EnemyDecisionSystem::push(i32 action, f32 score, f32 tx, f32 ty) {
  if (count >= MAX_SOUNDS) {
    // keep the heap bounded: discard the weakest tail entry
    if (count > 0 && heap[count - 1].score >= score) return;
    count--;
  }
  i32 i = count++;
  heap[i].action = action;
  heap[i].score = score;
  heap[i].targetX = tx;
  heap[i].targetY = ty;
  while (i > 0) {
    i32 parent = (i - 1) / 2;
    if (heap[parent].score >= heap[i].score) break;
    heapSwap(heap[parent], heap[i]);
    i = parent;
  }
}

EnemyDecision EnemyDecisionSystem::pop() {
  EnemyDecision out;
  out.action = ACT_NONE;
  out.score = -1.f;
  out.targetX = 0.f;
  out.targetY = 0.f;
  if (count <= 0) return out;
  out = heap[0];
  count--;
  if (count > 0) {
    heap[0] = heap[count];
    i32 i = 0;
    for (;;) {
      i32 l = i * 2 + 1;
      i32 r = l + 1;
      i32 best = i;
      if (l < count && heap[l].score > heap[best].score) best = l;
      if (r < count && heap[r].score > heap[best].score) best = r;
      if (best == i) break;
      heapSwap(heap[i], heap[best]);
      i = best;
    }
  }
  return out;
}

void EnemyDecisionSystem::updateScore(i32 action, f32 score) {
  for (i32 i = 0; i < count; ++i) {
    if (heap[i].action != action) continue;
    f32 old = heap[i].score;
    heap[i].score = score;
    if (score > old) {
      i32 k = i;
      while (k > 0) {
        i32 parent = (k - 1) / 2;
        if (heap[parent].score >= heap[k].score) break;
        heapSwap(heap[parent], heap[k]);
        k = parent;
      }
    } else if (score < old) {
      i32 k = i;
      for (;;) {
        i32 l = k * 2 + 1, r = l + 1, best = k;
        if (l < count && heap[l].score > heap[best].score) best = l;
        if (r < count && heap[r].score > heap[best].score) best = r;
        if (best == k) break;
        heapSwap(heap[k], heap[best]);
        k = best;
      }
    }
    return;
  }
}

f32 EnemyDecisionSystem::scoreOf(i32 action) const {
  for (i32 i = 0; i < count; ++i) if (heap[i].action == action) return heap[i].score;
  return 0.f;
}

} // namespace tcs
