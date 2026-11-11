#include "BFSExplorer.hpp"

namespace tcs {

void BFSExplorer::explore(const WorldGraph& g, const DoorTable& doors, i32 start, BFSResult& out) const {
  for (i32 i = 0; i < MAX_ROOMS; ++i) {
    out.order[i] = -1;
    out.dist[i] = -1;
    out.parent[i] = -1;
  }
  out.count = 0;
  if (start < 0 || start >= g.nodeCount) return;

  i32 queue[MAX_ROOMS];
  i32 head = 0, tail = 0;
  out.dist[start] = 0;
  queue[tail++] = start;

  while (head < tail) {
    i32 cur = queue[head++];
    out.order[out.count++] = cur;
    for (i32 i = 0; i < g.nodes[cur].edgeCount; ++i) {
      const GraphEdge& e = g.edges[g.nodes[cur].edgeStart + i];
      if (!g.edgePassable(doors, e)) continue;
      if (e.to < 0 || e.to >= g.nodeCount || out.dist[e.to] != -1) continue;
      out.dist[e.to] = out.dist[cur] + 1;
      out.parent[e.to] = cur;
      queue[tail++] = e.to;
    }
  }
}

i32 BFSExplorer::reachableCount(const WorldGraph& g, const DoorTable& doors, i32 start) const {
  BFSResult r;
  explore(g, doors, start, r);
  return r.count;
}

i32 BFSExplorer::path(const WorldGraph& g, const DoorTable& doors, i32 start, i32 target,
                      i32* out, i32 maxLen) const {
  BFSResult r;
  explore(g, doors, start, r);
  if (target < 0 || target >= MAX_ROOMS || r.dist[target] == -1) return 0;
  i32 tmp[MAX_ROOMS];
  i32 n = 0;
  i32 cur = target;
  while (cur != -1 && n < MAX_ROOMS) { tmp[n++] = cur; cur = r.parent[cur]; }
  i32 len = n < maxLen ? n : maxLen;
  for (i32 i = 0; i < len; ++i) out[i] = tmp[n - 1 - i];
  return len;
}

i32 BFSExplorer::nearestUnvisited(const WorldGraph& g, const DoorTable& doors, i32 start,
                                  const u8* visited) const {
  BFSResult r;
  explore(g, doors, start, r);
  for (i32 i = 0; i < r.count; ++i) {
    i32 room = r.order[i];
    if (room < 0 || room >= MAX_ROOMS) continue;
    if (visited == 0 || !visited[room]) return room;
  }
  return -1;
}

i32 BFSExplorer::escapeRooms(const WorldGraph& g, const DoorTable& doors, i32 start, i32 threat,
                             i32* out, i32 maxLen) const {
  BFSResult fromStart;
  BFSResult fromThreat;
  explore(g, doors, start, fromStart);
  explore(g, doors, threat, fromThreat);
  i32 n = 0;
  for (i32 i = 0; i < fromStart.count && n < maxLen; ++i) {
    i32 room = fromStart.order[i];
    if (room == start) continue;
    i32 ds = fromStart.dist[room];
    i32 dt = fromThreat.dist[room];
    if (dt == -1 || dt > ds) out[n++] = room;
  }
  return n;
}

} // namespace tcs
