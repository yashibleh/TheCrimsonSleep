// PlayerInventory: singly linked list of inventory items. The in-game
// inventory panel is a direct view of this list.
#pragma once
#include "core.hpp"

namespace tcs {

enum ItemCategory : i32 {
  CAT_NONE = 0,
  CAT_WEAPON,
  CAT_AMMO,
  CAT_HEAL,
  CAT_KEY,
  CAT_QUEST,
  CAT_BATTERY
};

struct InventoryNode {
  i32 itemId;
  i32 quantity;
  i32 category;
  i32 usable;
  u8  used;
  InventoryNode* next;
};

struct ItemDef {
  i32 id;
  const char* name;
  i32 category;
  i32 maxStack;
  i32 usable;
  f32 weight;
};

// Static item table (id -> definition). Ids are stable and used by the JS
// text table, so changing an id means changing the string table too.
static const i32 MAX_ITEM_DEFS = 40;
struct ItemTable {
  ItemDef defs[MAX_ITEM_DEFS];
  i32 count;
  void build();
  const ItemDef* find(i32 id) const;
};

struct PlayerInventory {
  InventoryNode pool[MAX_INV + 8];
  InventoryNode* head;
  InventoryNode* freeList;
  i32 count;

  ItemTable table;

  void reset();
  i32 insert(i32 itemId, i32 qty);
  bool remove(i32 itemId, i32 qty = 1);
  InventoryNode* find(i32 itemId);
  const InventoryNode* find(i32 itemId) const;
  i32 quantity(i32 itemId) const;
  i32 updateQuantity(i32 itemId, i32 delta);
  i32 countItems() const { return count; }
  InventoryNode* at(i32 index) const;
  i32 totalOfCategory(i32 category) const;
  bool hasCategory(i32 category) const;
};

} // namespace tcs
