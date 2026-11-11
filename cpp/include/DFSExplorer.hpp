// DFSExplorer: depth-first search over the world graph. Used for region
// discovery, hidden-branch detection and validating that newly opened areas
// are genuinely connected (secret passages, collapse shortcuts).
#pragma once
#include "core.hpp"
#include "WorldGraph.hpp"

namespace tcs {

struct DFSResult {
  i32 order[MAX_ROOMS];
  i32 depth[MAX_ROOMS];
  i32 parent[MAX_ROOMS];
  i32 count;
  i32 regions;       // connected components discoverable from all rooms
  i32 branchRooms;   // rooms with >1 forward connection (junction density)
};

struct DFSExplorer {
  void explore(const WorldGraph& g, const DoorTable& doors, i32 start, DFSResult& out) const;

  // Counts connected regions across the whole graph (uses every room as root).
  i32 countRegions(const WorldGraph& g, const DoorTable& doors) const;

  // Rooms discoverable only after `newRoom` is unlocked - the "hidden wing".
  i32 hiddenBranches(const WorldGraph& g, const DoorTable& doors, i32 newRoom,
                     i32* out, i32 maxLen) const;

  // Deepest reachable room from start (used by the ambient exploration hint).
  i32 deepestRoom(const WorldGraph& g, const DoorTable& doors, i32 start) const;

  // Visiting order of a region, filtered to rooms the player has discovered.
  i32 discoveredOrder(const WorldGraph& g, const DoorTable& doors, i32 start,
                      const u8* visited, i32* out, i32 maxLen) const;
};

} // namespace tcs
