// BFSExplorer: breadth-first search over the real world graph. Drives
// reachability, escape routes, objective routing and enemy room searches.
#pragma once
#include "core.hpp"
#include "WorldGraph.hpp"

namespace tcs {

struct BFSResult {
  i32 order[MAX_ROOMS];    // visit order
  i32 dist[MAX_ROOMS];     // room distance (-1 unreachable)
  i32 parent[MAX_ROOMS];
  i32 count;
};

struct BFSExplorer {
  // Full flood from `start` through passable edges.
  void explore(const WorldGraph& g, const DoorTable& doors, i32 start, BFSResult& out) const;

  // Number of rooms reachable from start (including start).
  i32 reachableCount(const WorldGraph& g, const DoorTable& doors, i32 start) const;

  // Path start -> target, writes rooms, returns length (0 if unreachable).
  i32 path(const WorldGraph& g, const DoorTable& doors, i32 start, i32 target,
           i32* out, i32 maxLen) const;

  // Nearest room that has not been visited yet; returns -1 if all are visited.
  // This powers the "unexplored wing" objective marker and secret discovery.
  i32 nearestUnvisited(const WorldGraph& g, const DoorTable& doors, i32 start,
                       const u8* visited) const;

  // Escape route: rooms that lead away from `threat` and are reachable from
  // `start` (used by the HUD threat arrow and by enemies herding the player).
  i32 escapeRooms(const WorldGraph& g, const DoorTable& doors, i32 start, i32 threat,
                  i32* out, i32 maxLen) const;
};

} // namespace tcs
