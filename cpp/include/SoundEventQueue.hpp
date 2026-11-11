// SoundEventQueue: FIFO queue of world sound events. Enemies drain it in
// order and investigate according to what they can hear.
#pragma once
#include "core.hpp"

namespace tcs {

struct SoundEventQueue {
  SoundEvent items[MAX_SOUNDS];
  i32 head;
  i32 tail;
  i32 count;
  f32 clock;

  void reset();
  bool empty() const { return count == 0; }
  i32 size() const { return count; }
  SoundEvent& front();
  SoundEvent& at(i32 logicalIndex);
  bool push(f32 x, f32 y, f32 intensity, f32 type);
  void pop();
  void expire(f32 maxAge);
  void clear();
  // True when this listener has not yet processed the front event.
  bool unread(i32 listener) const;
  void markRead(i32 listener);
  bool allRead() const;
  void markAllLoud(f32 radius);
};

} // namespace tcs
