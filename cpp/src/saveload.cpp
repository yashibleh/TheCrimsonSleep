#include "GameWorldManager.hpp"

namespace tcs {

union FloatBits { f32 f; i32 i; };

void GameWorldManager::writeInt(i32& c, i32 v) {
  if (c + 4 > (i32)sizeof(saveBuf)) return;
  saveBuf[c++] = (char)(v & 0xFF);
  saveBuf[c++] = (char)((v >> 8) & 0xFF);
  saveBuf[c++] = (char)((v >> 16) & 0xFF);
  saveBuf[c++] = (char)((v >> 24) & 0xFF);
}

i32 GameWorldManager::readInt(i32& c) {
  if (c + 4 > saveSize) return 0;
  i32 v = (i32)(u8)saveBuf[c] | ((i32)(u8)saveBuf[c + 1] << 8) |
          ((i32)(u8)saveBuf[c + 2] << 16) | ((i32)(u8)saveBuf[c + 3] << 24);
  c += 4;
  return v;
}

void GameWorldManager::writeFloat(i32& c, f32 value) {
  FloatBits b;
  b.f = value;
  writeInt(c, b.i);
}

f32 GameWorldManager::readFloat(i32& c) {
  FloatBits b;
  b.i = readInt(c);
  return b.f;
}

void GameWorldManager::serialize() {
  i32 c = 0;
  if (c + 5 > (i32)sizeof(saveBuf)) return;
  saveBuf[c++] = 'T';
  saveBuf[c++] = 'C';
  saveBuf[c++] = 'S';
  saveBuf[c++] = '1';
  writeInt(c, 2); // version

  writeInt(c, room);
  writeInt(c, state);
  writeInt(c, endingId);
  writeInt(c, objective);
  writeInt(c, codeSolved);
  writeInt(c, codeProgress);
  for (i32 i = 0; i < 4; ++i) writeInt(c, codeDigits[i]);
  writeInt(c, fuseMask);
  writeInt(c, valveProgress);
  writeInt(c, candleProgress);
  writeInt(c, weaverPhase);
  writeInt(c, deaths);
  writeInt(c, checkpoints);
  writeInt(c, totalKills);
  writeInt(c, secretCount);
  writeInt(c, player.room);
  writeInt(c, player.weapon);
  writeInt(c, player.hideProp);

  writeFloat(c, player.x);
  writeFloat(c, player.y);
  writeFloat(c, player.dirX);
  writeFloat(c, player.dirY);
  for (i32 i = 0; i < STAT_STAT_COUNT; ++i) writeFloat(c, player.stats[i]);
  for (i32 i = 0; i < PF_FLAG_COUNT; ++i) writeInt(c, player.flags[i]);

  writeInt(c, inv.count);
  for (i32 i = 0; i < inv.count; ++i) {
    InventoryNode* n = inv.at(i);
    if (!n) { writeInt(c, 0); writeInt(c, 0); continue; }
    writeInt(c, n->itemId);
    writeInt(c, n->quantity);
  }

  i32 ids[MAX_CLUES];
  i32 n = clues.inOrder(ids, MAX_CLUES);
  writeInt(c, n);
  for (i32 i = 0; i < n; ++i) writeInt(c, ids[i]);

  for (i32 i = 0; i < MAX_ROOMS * MAX_ROOM_DOORS; ++i) writeInt(c, doorTable.unlocked[i] ? 1 : 0);
  for (i32 i = 0; i < MAX_ROOMS; ++i) writeInt(c, visited[i] ? 1 : 0);
  for (i32 i = 0; i < 16; ++i) writeInt(c, storyFlags[i] ? 1 : 0);

  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    writeInt(c, rooms[rm].itemCount);
    for (i32 i = 0; i < rooms[rm].itemCount; ++i) writeInt(c, rooms[rm].items[i].taken ? 1 : 0);
  }
  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    writeInt(c, rooms[rm].propCount);
    for (i32 i = 0; i < rooms[rm].propCount; ++i) {
      writeInt(c, rooms[rm].props[i].state);
      writeInt(c, rooms[rm].props[i].itemId);
    }
  }

  writeInt(c, 0x54435345); // end marker
  saveSize = c;
}

bool GameWorldManager::deserialize(const char* data, i32 len) {
  if (!data || len < 16 || len > (i32)sizeof(saveBuf)) return false;
  for (i32 i = 0; i < len; ++i) saveBuf[i] = data[i];
  saveSize = len;

  i32 c = 0;
  if (saveBuf[c++] != 'T' || saveBuf[c++] != 'C' || saveBuf[c++] != 'S' || saveBuf[c++] != '1') return false;
  i32 version = readInt(c);
  if (version != 2) return false;

  i32 newRoom = readInt(c);
  state = readInt(c);
  endingId = readInt(c);
  objective = readInt(c);
  codeSolved = readInt(c);
  codeProgress = readInt(c);
  for (i32 i = 0; i < 4; ++i) codeDigits[i] = readInt(c);
  fuseMask = readInt(c);
  valveProgress = readInt(c);
  candleProgress = readInt(c);
  weaverPhase = readInt(c);
  deaths = readInt(c);
  checkpoints = readInt(c);
  totalKills = readInt(c);
  secretCount = readInt(c);
  i32 savedPlayerRoom = readInt(c);
  player.weapon = readInt(c);
  player.hideProp = readInt(c);

  player.x = readFloat(c);
  player.y = readFloat(c);
  player.dirX = readFloat(c);
  player.dirY = readFloat(c);
  for (i32 i = 0; i < STAT_STAT_COUNT; ++i) player.stats[i] = readFloat(c);
  for (i32 i = 0; i < PF_FLAG_COUNT; ++i) player.flags[i] = (u8)readInt(c);

  i32 invCount = readInt(c);
  inv.reset();
  for (i32 i = 0; i < invCount && i < MAX_INV; ++i) {
    i32 id = readInt(c);
    i32 qty = readInt(c);
    if (id > 0 && qty > 0) inv.insert(id, qty);
  }

  clues.reset();
  i32 clueCount = readInt(c);
  for (i32 i = 0; i < clueCount && i < MAX_CLUES; ++i) {
    i32 id = readInt(c);
    if (id > 0) clues.insert(id, savedPlayerRoom);
  }

  for (i32 i = 0; i < MAX_ROOMS * MAX_ROOM_DOORS; ++i) doorTable.unlocked[i] = readInt(c) ? 1 : 0;
  for (i32 i = 0; i < MAX_ROOMS; ++i) visited[i] = (u8)readInt(c);
  for (i32 i = 0; i < 16; ++i) storyFlags[i] = (u8)readInt(c);

  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    i32 itemCount = readInt(c);
    for (i32 i = 0; i < itemCount && i < MAX_ITEMS; ++i) {
      rooms[rm].items[i].taken = (u8)readInt(c);
    }
  }
  for (i32 rm = 0; rm < MAX_ROOMS; ++rm) {
    i32 propCount = readInt(c);
    for (i32 i = 0; i < propCount && i < MAX_PROPS; ++i) {
      rooms[rm].props[i].state = readInt(c);
      rooms[rm].props[i].itemId = readInt(c);
    }
  }
  i32 marker = readInt(c);
  if (marker != 0x54435345) return false;

  i32 rr = savedPlayerRoom;
  if (rr < 0 || rr >= MAX_ROOMS) rr = newRoom;
  f32 px = player.x, py = player.y;
  bool wasHiding = player.hasFlag(PF_HIDING);
  i32 hideSpot = player.hideProp;

  room = rr;
  enemies.clear();
  loadRoom(rr, px, py, false);
  player.x = px;
  player.y = py;
  if (wasHiding) {
    player.setFlag(PF_HIDING, true);
    player.hideProp = hideSpot;
  }
  rebuildNav();
  sounds.clear();
  tick += 0.1f;
  return true;
}

} // namespace tcs
