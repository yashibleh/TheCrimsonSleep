// WorldGraph: adjacency-list graph of rooms used by enemy pursuit,
// objective routing and world exploration.
#pragma once
#include "core.hpp"

namespace tcs {

struct GraphEdge {
  i32 to;      // destination room
  f32 cost;    // traversal cost (seconds-ish)
  i32 doorId;  // flat door id that gates this edge (-1 = always open)
};

struct RoomNode {
  i32 id;
  i32 nameId;
  i32 edgeStart;
  i32 edgeCount;
  f32 cx, cy;   // room centre in world pixels (for map screen)
  i32 palette;
  i32 floorIndex;
};

struct RawEdge {
  i32 from;
  i32 to;
  f32 cost;
  i32 doorId;
};

struct WorldGraph {
  RoomNode nodes[MAX_ROOMS];
  GraphEdge edges[MAX_ROOMS * 8];
  RawEdge raw[MAX_ROOMS * 8];
  i32 nodeCount;
  i32 edgeCount;   // entries in the contiguous adjacency list
  i32 rawCount;

  void reset();
  i32 addRoom(i32 nameId, f32 cx, f32 cy, i32 palette, i32 floorIndex);
  i32 addEdge(i32 a, i32 b, f32 cost, i32 doorId);  // records both directions
  void finalize();                                  // builds adjacency blocks

  bool edgePassable(const DoorTable& doors, const GraphEdge& e) const;
  i32 degree(i32 room) const;

  // Shortest room route (BFS over the adjacency list, honouring locked doors).
  // Writes room ids into out[], returns length or 0 when unreachable.
  i32 route(const DoorTable& doors, i32 from, i32 to, i32* out, i32 maxLen) const;
  i32 routeCost(const DoorTable& doors, i32 from, i32 to) const;

  i32 findEdge(i32 a, i32 b) const;
};

} // namespace tcs
