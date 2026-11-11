#include "SoundEventQueue.hpp"

namespace tcs {

void SoundEventQueue::reset() {
  head = 0;
  tail = 0;
  count = 0;
  clock = 0.f;
}

SoundEvent& SoundEventQueue::front() {
  return items[head];
}

SoundEvent& SoundEventQueue::at(i32 logicalIndex) {
  return items[(head + logicalIndex) % MAX_SOUNDS];
}

bool SoundEventQueue::push(f32 x, f32 y, f32 intensity, f32 type) {
  if (count >= MAX_SOUNDS) pop(); // oldest event is discarded, never the newest
  SoundEvent& e = items[tail];
  e.x = x;
  e.y = y;
  e.intensity = clampf(intensity, 0.f, 1.f);
  e.type = type;
  e.tick = clock;
  for (i32 i = 0; i < MAX_ENEMIES; ++i) e.consumedBy[i] = 0;
  tail = (tail + 1) % MAX_SOUNDS;
  count++;
  return true;
}

void SoundEventQueue::pop() {
  if (count == 0) return;
  head = (head + 1) % MAX_SOUNDS;
  count--;
}

void SoundEventQueue::expire(f32 maxAge) {
  while (count > 0 && (clock - items[head].tick) > maxAge) pop();
}

void SoundEventQueue::clear() {
  head = tail = count = 0;
}

bool SoundEventQueue::unread(i32 listener) const {
  if (listener < 0 || listener >= MAX_ENEMIES) return false;
  return items[head].consumedBy[listener] == 0;
}

void SoundEventQueue::markRead(i32 listener) {
  if (listener < 0 || listener >= MAX_ENEMIES) return;
  items[head].consumedBy[listener] = 1;
}

bool SoundEventQueue::allRead() const {
  if (count == 0) return true;
  for (i32 i = 0; i < MAX_ENEMIES; ++i) {
    if (!items[head].consumedBy[i]) return false;
  }
  return true;
}

void SoundEventQueue::markAllLoud(f32 radius) {
  (void)radius;
  for (i32 i = 0; i < count; ++i) {
    SoundEvent& e = at(i);
    for (i32 k = 0; k < MAX_ENEMIES; ++k) e.consumedBy[k] = 1;
  }
}

} // namespace tcs
