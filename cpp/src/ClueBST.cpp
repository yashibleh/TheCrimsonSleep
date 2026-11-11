#include "ClueBST.hpp"

namespace tcs {

void ClueBST::reset() {
  root = 0;
  count = 0;
  discoverySeq = 0;
  for (i32 i = 0; i < MAX_CLUES + 8; ++i) {
    pool[i].id = 0;
    pool[i].locationId = 0;
    pool[i].order = 0;
    pool[i].found = 0;
    pool[i].left = 0;
    pool[i].right = 0;
  }
  // free list threading
  for (i32 i = 0; i < MAX_CLUES + 7; ++i) pool[i].left = &pool[i + 1];
  pool[MAX_CLUES + 7].left = 0;
  freeList = &pool[0];
}

ClueNode* ClueBST::search(i32 id) {
  ClueNode* cur = root;
  while (cur) {
    if (id == cur->id) return cur;
    cur = id < cur->id ? cur->left : cur->right;
  }
  return 0;
}

bool ClueBST::insert(i32 id, i32 locationId) {
  if (id <= 0) return false;
  if (search(id)) return false;
  if (!freeList) return false;
  ClueNode* node = freeList;
  freeList = node->left;      // free list uses `left` as the chain pointer
  node->id = id;
  node->locationId = locationId;
  node->order = ++discoverySeq;
  node->found = 1;
  node->left = 0;
  node->right = 0;

  if (!root) { root = node; count++; return true; }
  ClueNode* cur = root;
  for (;;) {
    if (id < cur->id) {
      if (!cur->left) { cur->left = node; break; }
      cur = cur->left;
    } else {
      if (!cur->right) { cur->right = node; break; }
      cur = cur->right;
    }
  }
  count++;
  return true;
}

static ClueNode* bstRemoveNode(ClueNode* node, i32 id, bool& removed) {
  if (!node) return 0;
  if (id < node->id) {
    node->left = bstRemoveNode(node->left, id, removed);
    return node;
  }
  if (id > node->id) {
    node->right = bstRemoveNode(node->right, id, removed);
    return node;
  }
  removed = true;
  if (!node->left) return node->right;
  if (!node->right) return node->left;
  // successor = leftmost node of right subtree
  ClueNode* succ = node->right;
  while (succ->left) succ = succ->left;
  node->id = succ->id;
  node->locationId = succ->locationId;
  node->order = succ->order;
  bool dummy = false;
  node->right = bstRemoveNode(node->right, succ->id, dummy);
  return node;
}

bool ClueBST::remove(i32 id) {
  bool removed = false;
  root = bstRemoveNode(root, id, removed);
  if (removed && count > 0) count--;
  return removed;
}

static void inorderWalk(const ClueNode* node, i32* out, i32 maxLen, i32& n) {
  if (!node || n >= maxLen) return;
  inorderWalk(node->left, out, maxLen, n);
  if (n < maxLen && node->found) out[n++] = node->id;
  inorderWalk(node->right, out, maxLen, n);
}

i32 ClueBST::inOrder(i32* out, i32 maxLen) const {
  i32 n = 0;
  inorderWalk(root, out, maxLen, n);
  return n;
}

static i32 depthOf(const ClueNode* node) {
  if (!node) return 0;
  i32 l = depthOf(node->left);
  i32 r = depthOf(node->right);
  return 1 + (l > r ? l : r);
}

i32 ClueBST::height() const { return depthOf(root); }

static bool balancedRec(const ClueNode* node) {
  if (!node) return true;
  i32 l = depthOf(node->left);
  i32 r = depthOf(node->right);
  if (iabs(l - r) > 1) return false;
  return balancedRec(node->left) && balancedRec(node->right);
}

bool ClueBST::isBalanced() const { return balancedRec(root); }

} // namespace tcs
