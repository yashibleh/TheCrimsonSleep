#include "DFSExplorer.hpp"

namespace tcs {

void DFSExplorer::explore(const WorldGraph& g, const DoorTable& doors, i32 start, DFSResult& out) const {
  for (i32 i = 0; i < MAX_ROOMS; ++i) {
    out.order[i] = -1;
    out.depth[i] = -1;
    out.parent[i] = -1;
  }
  out.count = 0;
  out.regions = 0;
  out.branchRooms = 0;
  if (start < 0 || start >= g.nodeCount) return;

  // Explicit stack (iterative DFS) with per-room neighbour cursor.
  i32 stack[MAX_ROOMS];
  i32 cursor[MAX_ROOMS];
  u8 seen[MAX_ROOMS];
  for (i32 i = 0; i < MAX_ROOMS; ++i) { cursor[i] = 0; seen[i] = 0; }

  i32 top = 0;
  i32 cur = start;
  out.depth[start] = 0;
  seen[start] = 1;
  out.order[out.count++] = start;
  stack[top++] = start;

  i32 forwardCount = 0;
  while (top > 0) {
    cur = stack[top - 1];
    if (cursor[cur] < g.nodes[cur].edgeCount) {
      const GraphEdge& e = g.edges[g.nodes[cur].edgeStart + cursor[cur]];
      cursor[cur]++;
      if (!g.edgePassable(doors, e)) continue;
      if (e.to < 0 || e.to >= g.nodeCount || seen[e.to]) continue;
      seen[e.to] = 1;
      out.parent[e.to] = cur;
      out.depth[e.to] = out.depth[cur] + 1;
      out.order[out.count++] = e.to;
      forwardCount++;
      stack[top++] = e.to;
    } else {
      top--;
    }
  }

  // A junction is a room whose forward DFS connections exceed one.
  i32 childCount[MAX_ROOMS];
  for (i32 i = 0; i < MAX_ROOMS; ++i) childCount[i] = 0;
  for (i32 i = 0; i < g.nodeCount; ++i) {
    if (out.parent[i] >= 0 && out.parent[i] < MAX_ROOMS) childCount[out.parent[i]]++;
  }
  for (i32 i = 0; i < g.nodeCount; ++i) if (childCount[i] > 1) out.branchRooms++;
  if (forwardCount == 0) out.branchRooms = 0;
}

i32 DFSExplorer::countRegions(const WorldGraph& g, const DoorTable& doors) const {
  u8 seen[MAX_ROOMS];
  for (i32 i = 0; i < MAX_ROOMS; ++i) seen[i] = 0;
  i32 regions = 0;
  i32 stack[MAX_ROOMS];
  for (i32 root = 0; root < g.nodeCount; ++root) {
    if (seen[root]) continue;
    regions++;
    i32 top = 0;
    stack[top++] = root;
    seen[root] = 1;
    while (top > 0) {
      i32 cur = stack[--top];
      for (i32 i = 0; i < g.nodes[cur].edgeCount; ++i) {
        const GraphEdge& e = g.edges[g.nodes[cur].edgeStart + i];
        if (!g.edgePassable(doors, e)) continue;
        if (e.to < 0 || e.to >= g.nodeCount || seen[e.to]) continue;
        seen[e.to] = 1;
        stack[top++] = e.to;
      }
    }
  }
  return regions;
}

i32 DFSExplorer::hiddenBranches(const WorldGraph& g, const DoorTable& doors, i32 newRoom,
                                i32* out, i32 maxLen) const {
  if (newRoom < 0 || newRoom >= g.nodeCount) return 0;
  DoorTable without = doors;
  for (i32 i = 0; i < MAX_ROOMS * MAX_ROOM_DOORS; ++i) {
    without.unlocked[i] = 1;
  }
  DFSResult before;
  explore(g, without, newRoom, before);
  i32 n = 0;
  // Report rooms whose only route runs through the freshly opened one.
  for (i32 i = 0; i < before.count && n < maxLen; ++i) {
    i32 room = before.order[i];
    i32 ds = g.routeCost(doors, newRoom, room);
    i32 da = g.routeCost(without, newRoom, room);
    if (ds != -1 && (da == -1 || ds > da)) out[n++] = room;
  }
  return n;
}

i32 DFSExplorer::deepestRoom(const WorldGraph& g, const DoorTable& doors, i32 start) const {
  DFSResult r;
  explore(g, doors, start, r);
  i32 best = start;
  i32 bestDepth = -1;
  for (i32 i = 0; i < r.count; ++i) {
    i32 room = r.order[i];
    if (r.depth[room] > bestDepth) { bestDepth = r.depth[room]; best = room; }
  }
  return best;
}

i32 DFSExplorer::discoveredOrder(const WorldGraph& g, const DoorTable& doors, i32 start,
                                 const u8* visited, i32* out, i32 maxLen) const {
  DFSResult r;
  explore(g, doors, start, r);
  i32 n = 0;
  for (i32 i = 0; i < r.count && n < maxLen; ++i) {
    if (visited && !visited[r.order[i]]) continue;
    out[n++] = r.order[i];
  }
  return n;
}

} // namespace tcs
