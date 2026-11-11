// ClueBST: binary search tree indexing the journal. Node key = unique clue id,
// ordered so the journal reads chronologically by discovery id.
#pragma once
#include "core.hpp"

namespace tcs {

struct ClueNode {
  i32 id;          // unique clue id (also the string-table id)
  i32 locationId;  // room where it was found
  i32 order;       // discovery sequence
  u8  found;
  ClueNode* left;
  ClueNode* right;
};

struct ClueBST {
  ClueNode pool[MAX_CLUES + 8];
  ClueNode* root;
  ClueNode* freeList;
  i32 count;
  i32 discoverySeq;

  void reset();
  bool insert(i32 id, i32 locationId);
  ClueNode* search(i32 id);
  bool remove(i32 id);
  i32 inOrder(i32* out, i32 maxLen) const;
  i32 foundCount() const { return count; }
  i32 height() const;
  bool isBalanced() const;
};

} // namespace tcs
