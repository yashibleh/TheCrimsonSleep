#include "WorldGraph.hpp"

namespace tcs {

void WorldGraph::reset() {
  nodeCount = 0;
  edgeCount = 0;
  rawCount = 0;
  for (i32 i = 0; i < MAX_ROOMS; ++i) {
    nodes[i].id = i;
    nodes[i].nameId = 0;
    nodes[i].edgeStart = 0;
    nodes[i].edgeCount = 0;
    nodes[i].cx = 0.f;
    nodes[i].cy = 0.f;
    nodes[i].palette = 0;
    nodes[i].floorIndex = 0;
  }
  for (i32 i = 0; i < MAX_ROOMS * 8; ++i) {
    raw[i].from = -1;
    raw[i].to = -1;
    raw[i].cost = 1.f;
    raw[i].doorId = -1;
    edges[i].to = -1;
    edges[i].cost = 1.f;
    edges[i].doorId = -1;
  }
}

i32 WorldGraph::addRoom(i32 nameId, f32 cx, f32 cy, i32 palette, i32 floorIndex) {
  if (nodeCount >= MAX_ROOMS) return -1;
  i32 id = nodeCount++;
  nodes[id].id = id;
  nodes[id].nameId = nameId;
  nodes[id].edgeStart = 0;
  nodes[id].edgeCount = 0;
  nodes[id].cx = cx;
  nodes[id].cy = cy;
  nodes[id].palette = palette;
  nodes[id].floorIndex = floorIndex;
  return id;
}

i32 WorldGraph::addEdge(i32 a, i32 b, f32 cost, i32 doorId) {
  if (a < 0 || b < 0 || a >= nodeCount || b >= nodeCount) return -1;
  if (a == b) return -1;
  if (rawCount + 2 > MAX_ROOMS * 8) return -1;
  raw[rawCount].from = a;
  raw[rawCount].to = b;
  raw[rawCount].cost = cost;
  raw[rawCount].doorId = doorId;
  rawCount++;
  raw[rawCount].from = b;
  raw[rawCount].to = a;
  raw[rawCount].cost = cost;
  raw[rawCount].doorId = doorId;
  rawCount++;
  return rawCount - 2;
}

void WorldGraph::finalize() {
  for (i32 r = 0; r < MAX_ROOMS; ++r) {
    nodes[r].edgeStart = 0;
    nodes[r].edgeCount = 0;
  }
  for (i32 i = 0; i < rawCount; ++i) {
    i32 f = raw[i].from;
    if (f >= 0 && f < nodeCount) nodes[f].edgeCount++;
  }
  i32 cursor = 0;
  for (i32 r = 0; r < nodeCount; ++r) {
    nodes[r].edgeStart = cursor;
    cursor += nodes[r].edgeCount;
    if (cursor > MAX_ROOMS * 8) cursor = MAX_ROOMS * 8;
  }
  edgeCount = cursor;
  for (i32 i = 0; i < MAX_ROOMS * 8; ++i) {
    edges[i].to = -1;
    edges[i].cost = 1.f;
    edges[i].doorId = -1;
  }
  i32 fill[MAX_ROOMS];
  for (i32 r = 0; r < MAX_ROOMS; ++r) fill[r] = nodes[r].edgeStart;
  for (i32 i = 0; i < rawCount; ++i) {
    i32 f = raw[i].from;
    if (f < 0 || f >= nodeCount) continue;
    i32 limit = nodes[f].edgeStart + nodes[f].edgeCount;
    if (fill[f] >= limit || fill[f] >= MAX_ROOMS * 8) continue;
    GraphEdge& e = edges[fill[f]++];
    e.to = raw[i].to;
    e.cost = raw[i].cost;
    e.doorId = raw[i].doorId;
  }
}

bool WorldGraph::edgePassable(const DoorTable& doors, const GraphEdge& e) const {
  if (e.doorId < 0) return true;
  if (e.doorId >= MAX_ROOMS * MAX_ROOM_DOORS) return true;
  return doors.unlocked[e.doorId] != 0;
}

i32 WorldGraph::degree(i32 room) const {
  if (room < 0 || room >= nodeCount) return 0;
  return nodes[room].edgeCount;
}

i32 WorldGraph::findEdge(i32 a, i32 b) const {
  if (a < 0 || a >= nodeCount) return -1;
  for (i32 i = 0; i < nodes[a].edgeCount; ++i) {
    i32 idx = nodes[a].edgeStart + i;
    if (edges[idx].to == b) return idx;
  }
  return -1;
}

i32 WorldGraph::route(const DoorTable& doors, i32 from, i32 to, i32* out, i32 maxLen) const {
  if (from < 0 || to < 0 || from >= nodeCount || to >= nodeCount) return 0;
  if (out == 0 || maxLen <= 0) return 0;
  if (from == to) { out[0] = from; return 1; }

  i32 dist[MAX_ROOMS];
  i32 parent[MAX_ROOMS];
  i32 queue[MAX_ROOMS];
  i32 head = 0, tail = 0;
  for (i32 i = 0; i < MAX_ROOMS; ++i) { dist[i] = -1; parent[i] = -1; }

  dist[from] = 0;
  queue[tail++] = from;
  while (head < tail) {
    i32 cur = queue[head++];
    if (cur == to) break;
    for (i32 i = 0; i < nodes[cur].edgeCount; ++i) {
      const GraphEdge& e = edges[nodes[cur].edgeStart + i];
      if (!edgePassable(doors, e)) continue;
      if (e.to < 0 || e.to >= nodeCount) continue;
      if (dist[e.to] != -1) continue;
      dist[e.to] = dist[cur] + 1;
      parent[e.to] = cur;
      queue[tail++] = e.to;
    }
  }
  if (dist[to] == -1) return 0;

  i32 tmp[MAX_ROOMS];
  i32 n = 0;
  i32 cur = to;
  while (cur != -1 && n < MAX_ROOMS) { tmp[n++] = cur; cur = parent[cur]; }
  i32 len = n < maxLen ? n : maxLen;
  for (i32 i = 0; i < len; ++i) out[i] = tmp[n - 1 - i];
  return len;
}

i32 WorldGraph::routeCost(const DoorTable& doors, i32 from, i32 to) const {
  if (from < 0 || to < 0 || from >= nodeCount || to >= nodeCount) return -1;
  i32 dist[MAX_ROOMS];
  i32 queue[MAX_ROOMS];
  i32 head = 0, tail = 0;
  for (i32 i = 0; i < MAX_ROOMS; ++i) dist[i] = -1;
  dist[from] = 0;
  queue[tail++] = from;
  while (head < tail) {
    i32 cur = queue[head++];
    for (i32 i = 0; i < nodes[cur].edgeCount; ++i) {
      const GraphEdge& e = edges[nodes[cur].edgeStart + i];
      if (!edgePassable(doors, e)) continue;
      if (e.to < 0 || e.to >= nodeCount || dist[e.to] != -1) continue;
      dist[e.to] = dist[cur] + 1;
      queue[tail++] = e.to;
    }
  }
  return dist[to];
}

} // namespace tcs
