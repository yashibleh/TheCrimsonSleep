#include "PlayerInventory.hpp"

namespace tcs {

void ItemTable::build() {
  count = 0;
  struct Row { i32 id; const char* name; i32 cat; i32 stack; i32 usable; f32 w; };
  static const Row rows[] = {
    { 1,  "Rusty Crowbar",        CAT_WEAPON, 1,  1, 1.4f },
    { 2,  "Service Revolver",     CAT_WEAPON, 1,  1, 0.9f },
    { 3,  "Revolver Rounds",      CAT_AMMO,   36, 0, 0.1f },
    { 4,  "Gauze Bandage",        CAT_HEAL,   5,  1, 0.2f },
    { 5,  "Fuse (Blue)",          CAT_KEY,    1,  0, 0.1f },
    { 6,  "Fuse (Red)",           CAT_KEY,    1,  0, 0.1f },
    { 7,  "Fuse (Brass)",         CAT_KEY,    1,  0, 0.1f },
    { 8,  "Ward Key",             CAT_KEY,    1,  0, 0.1f },
    { 9,  "Valve Handle",         CAT_KEY,    1,  0, 0.6f },
    { 10, "Brass Mortuary Key",   CAT_KEY,    1,  0, 0.1f },
    { 11, "Lantern Battery",      CAT_BATTERY,4, 0, 0.2f },
    { 12, "Sedative Syringe",     CAT_QUEST,  1,  1, 0.2f },
    { 13, "Ilse's Locket",        CAT_QUEST,  1,  0, 0.1f },
    { 14, "Box of Matches",       CAT_HEAL,   9,  1, 0.1f }
  };
  const i32 n = (i32)(sizeof(rows) / sizeof(rows[0]));
  for (i32 i = 0; i < n && i < MAX_ITEM_DEFS; ++i) {
    defs[i].id = rows[i].id;
    defs[i].name = rows[i].name;
    defs[i].category = rows[i].cat;
    defs[i].maxStack = rows[i].stack;
    defs[i].usable = rows[i].usable;
    defs[i].weight = rows[i].w;
    count++;
  }
}

const ItemDef* ItemTable::find(i32 id) const {
  for (i32 i = 0; i < count; ++i) if (defs[i].id == id) return &defs[i];
  return 0;
}

void PlayerInventory::reset() {
  head = 0;
  count = 0;
  for (i32 i = 0; i < MAX_INV + 8; ++i) {
    pool[i].itemId = 0;
    pool[i].quantity = 0;
    pool[i].category = CAT_NONE;
    pool[i].usable = 0;
    pool[i].used = 0;
    pool[i].next = (i + 1 < MAX_INV + 8) ? &pool[i + 1] : 0;
  }
  freeList = &pool[0];
  table.build();
}

i32 PlayerInventory::insert(i32 itemId, i32 qty) {
  if (qty <= 0) return 0;
  const ItemDef* def = table.find(itemId);
  if (!def) return 0;

  // Stack into an existing node when the definition allows it.
  InventoryNode* existing = find(itemId);
  if (existing && def->maxStack > 1) {
    i32 room = def->maxStack - existing->quantity;
    if (room > 0) {
      i32 moved = qty < room ? qty : room;
      existing->quantity += moved;
      return moved;
    }
  }
  if (!freeList) return 0; // inventory full
  InventoryNode* node = freeList;
  freeList = node->next;
  node->itemId = itemId;
  node->quantity = qty > def->maxStack ? def->maxStack : qty;
  node->category = def->category;
  node->usable = def->usable;
  node->used = 0;
  node->next = head;
  head = node;
  count++;
  return node->quantity;
}

bool PlayerInventory::remove(i32 itemId, i32 qty) {
  if (qty <= 0) return false;
  InventoryNode* prev = 0;
  InventoryNode* cur = head;
  while (cur) {
    if (cur->itemId == itemId) {
      if (cur->quantity > qty) { cur->quantity -= qty; return true; }
      if (prev) prev->next = cur->next;
      else head = cur->next;
      qty -= cur->quantity;
      cur->itemId = 0;
      cur->quantity = 0;
      cur->category = CAT_NONE;
      cur->usable = 0;
      cur->next = freeList;
      freeList = cur;
      count--;
      if (qty <= 0) return true;
      // continue searching for further stacks of the same item
      cur = prev ? prev->next : head;
      continue;
    }
    prev = cur;
    cur = cur->next;
  }
  return false;
}

InventoryNode* PlayerInventory::find(i32 itemId) {
  InventoryNode* cur = head;
  while (cur) {
    if (cur->itemId == itemId) return cur;
    cur = cur->next;
  }
  return 0;
}

const InventoryNode* PlayerInventory::find(i32 itemId) const {
  const InventoryNode* cur = head;
  while (cur) {
    if (cur->itemId == itemId) return cur;
    cur = cur->next;
  }
  return 0;
}

i32 PlayerInventory::quantity(i32 itemId) const {
  const InventoryNode* cur = head;
  i32 sum = 0;
  while (cur) {
    if (cur->itemId == itemId) sum += cur->quantity;
    cur = cur->next;
  }
  return sum;
}

i32 PlayerInventory::updateQuantity(i32 itemId, i32 delta) {
  InventoryNode* node = find(itemId);
  if (!node) return 0;
  node->quantity += delta;
  const ItemDef* def = table.find(itemId);
  i32 maxQ = def ? def->maxStack : 1;
  if (node->quantity > maxQ) node->quantity = maxQ;
  if (node->quantity <= 0) {
    remove(itemId, 1);
    return 0;
  }
  return node->quantity;
}

InventoryNode* PlayerInventory::at(i32 index) const {
  const InventoryNode* cur = head;
  i32 i = 0;
  while (cur) {
    if (i == index) return (InventoryNode*)cur;
    cur = cur->next;
    i++;
  }
  return 0;
}

i32 PlayerInventory::totalOfCategory(i32 category) const {
  const InventoryNode* cur = head;
  i32 sum = 0;
  while (cur) {
    if (cur->category == category) sum += cur->quantity;
    cur = cur->next;
  }
  return sum;
}

bool PlayerInventory::hasCategory(i32 category) const {
  const InventoryNode* cur = head;
  while (cur) {
    if (cur->category == category) return true;
    cur = cur->next;
  }
  return false;
}

} // namespace tcs
