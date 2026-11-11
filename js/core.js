// core.js - WebAssembly bridge. Reads only the flat blocks the C++ core
// publishes; it never makes gameplay decisions of its own.
export const CELLS = 880;
export const INPUT = {
  UP: 1 << 0, DOWN: 1 << 1, LEFT: 1 << 2, RIGHT: 1 << 3,
  ATTACK: 1 << 4, INTERACT: 1 << 5, LIGHT: 1 << 6, SPRINT: 1 << 7,
  RELOAD: 1 << 8, HEAL: 1 << 9, SWAP: 1 << 10, USE: 1 << 11, HIDE: 1 << 12
};

export const ENEMY_STATES = ['Idle', 'Patrolling', 'Suspicious', 'Investigating', 'Chasing', 'Attacking', 'Searching', 'Returning'];
export const ENEMY_TYPES = ['Crawler', 'Hound', 'Weaver'];

export class CrimsonCore {
  constructor(instance) {
    this.x = instance.exports;
    this.mem = this.x.memory;
  }

  static async load(url = 'assets/game.wasm') {
    let instance;
    if (WebAssembly.instantiateStreaming) {
      try {
        const res = await fetch(url);
        const out = await WebAssembly.instantiateStreaming(res, {});
        instance = out.instance;
      } catch (err) {
        const buf = await (await fetch(url)).arrayBuffer();
        const out = await WebAssembly.instantiate(buf, {});
        instance = out.instance;
      }
    } else {
      const buf = await (await fetch(url)).arrayBuffer();
      const out = await WebAssembly.instantiate(buf, {});
      instance = out.instance;
    }
    return new CrimsonCore(instance);
  }

  f32(ptr, len) { return new Float32Array(this.mem.buffer, ptr, len); }
  u8(ptr, len) { return new Uint8Array(this.mem.buffer, ptr, len); }

  init() { this.x.tcs_init(); }
  newGame() { this.x.tcs_new_game(); }
  update(dt, input) { this.x.tcs_update(dt, input); }

  get roomId() { return this.x.tcs_room_id(); }
  get roomNameId() { return this.x.tcs_room_name(); }
  get palette() { return this.x.tcs_room_palette(); }
  get ambient() { return this.x.tcs_room_ambient(); }
  get musicMood() { return this.x.tcs_room_music(); }
  get floorIndex() { return this.x.tcs_room_floor(); }
  get state() { return this.x.tcs_state(); }
  get ending() { return this.x.tcs_ending(); }
  get objective() { return this.x.tcs_objective(); }
  get danger() { return this.x.tcs_danger() / 100; }
  get message() { return this.x.tcs_message(); }
  get messageTimer() { return this.x.tcs_message_timer(); }
  get subtitle() { return this.x.tcs_subtitle(); }
  get subtitleTimer() { return this.x.tcs_subtitle_timer(); }
  get uiRequest() { return this.x.tcs_ui_request(); }
  clearUi() { this.x.tcs_ui_clear(); }
  get deaths() { return this.x.tcs_deaths(); }
  get checkpoints() { return this.x.tcs_checkpoints(); }
  get kills() { return this.x.tcs_kills(); }
  get secrets() { return this.x.tcs_secrets(); }
  get tick() { return this.x.tcs_tick() / 100; }
  get fuseMask() { return this.x.tcs_fuse_mask(); }
  get soundCount() { return this.x.tcs_sound_count(); }
  get actionCount() { return this.x.tcs_action_count(); }
  get bfsReachFromStart() { return this.x.tcs_bfs_reach(0); }

  player() {
    const b = this.f32(this.x.tcs_player(), 32);
    return {
      x: b[0], y: b[1], vx: b[2], vy: b[3], dirX: b[4], dirY: b[5],
      hp: b[6], hpMax: b[7], stamina: b[8], staminaMax: b[9],
      battery: b[10], batteryMax: b[11], ammo: b[12], mag: b[13],
      weapon: b[14], light: b[15] === 1, hiding: b[16] === 1,
      invuln: b[17], attackTimer: b[18], attackCooldown: b[19],
      walkPhase: b[20], noise: b[21], damageFlash: b[22], sprinting: b[23] === 1,
      dead: b[24] === 1, room: b[25], hideProp: b[26], kills: b[27],
      animTime: b[28], reloadTimer: b[29], ammoReserve: b[30], heartbeat: b[31]
    };
  }

  enemies() {
    const n = this.x.tcs_enemy_count();
    const b = this.f32(this.x.tcs_enemies(), n * 20);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 20;
      if (b[o + 16] !== 1) continue;
      out.push({
        index: i, x: b[o], y: b[o + 1], type: b[o + 2], state: b[o + 3],
        hp: b[o + 4], maxHp: b[o + 5], dirX: b[o + 6], dirY: b[o + 7],
        alert: b[o + 8], attackTimer: b[o + 9], animPhase: b[o + 10],
        sees: b[o + 11] === 1, stateTimer: b[o + 12], room: b[o + 13],
        hitFlash: b[o + 14], anim: b[o + 15], lastKnownX: b[o + 17], lastKnownY: b[o + 18]
      });
    }
    return out;
  }

  tiles() { return this.u8(this.x.tcs_tiles(), CELLS); }
  decor() { return this.u8(this.x.tcs_decor(), CELLS); }
  navGrid() { return this.u8(this.x.tcs_navgrid(), CELLS); }

  lights() {
    const n = this.x.tcs_light_count();
    const b = this.f32(this.x.tcs_lights(), n * 9);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 9;
      out.push({ x: b[o], y: b[o + 1], radius: b[o + 2], intensity: b[o + 3],
        flicker: b[o + 4], phase: b[o + 5], r: b[o + 6], g: b[o + 7], b: b[o + 8] });
    }
    return out;
  }

  props() {
    const n = this.x.tcs_prop_count();
    const b = this.f32(this.x.tcs_props(), n * 9);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 9;
      out.push({ index: i, x: b[o], y: b[o + 1], w: b[o + 2], h: b[o + 3],
        type: b[o + 4], id: b[o + 5], state: b[o + 6], label: b[o + 7], interactive: b[o + 8] === 1 });
    }
    return out;
  }

  doors() {
    const n = this.x.tcs_door_count();
    const b = this.f32(this.x.tcs_doors(), n * 8);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 8;
      out.push({ index: i, x: b[o], y: b[o + 1], open: b[o + 2] === 1, locked: b[o + 3] === 1,
        dir: b[o + 4], targetRoom: b[o + 5], keyItem: b[o + 6], label: b[o + 7] });
    }
    return out;
  }

  items() {
    const n = this.x.tcs_item_count();
    const b = this.f32(this.x.tcs_items(), n * 6);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 6;
      if (b[o + 5] === 1) continue;
      out.push({ index: i, x: b[o], y: b[o + 1], itemId: b[o + 2], qty: b[o + 3], glow: b[o + 4] });
    }
    return out;
  }

  prompt() {
    const b = this.f32(this.x.tcs_prompt_ptr(), 6);
    return { id: b[0], label: b[1], target: b[2], x: b[3], y: b[4] };
  }

  events() {
    const n = this.x.tcs_event_count();
    const b = this.f32(this.x.tcs_events(), n * 5);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 5;
      out.push({ type: b[o], x: b[o + 1], y: b[o + 2], a: b[o + 3], b: b[o + 4] });
    }
    return out;
  }
  clearEvents() { this.x.tcs_clear_events(); }

  inventory() {
    const n = this.x.tcs_inv_count();
    const out = [];
    for (let i = 0; i < n; i++) {
      out.push({ id: this.x.tcs_inv_id(i), qty: this.x.tcs_inv_qty(i),
        cat: this.x.tcs_inv_cat(i), usable: this.x.tcs_inv_usable(i) === 1 });
    }
    return out;
  }
  useItem(id) { this.x.tcs_use_item(id); }
  hasItem(id) { return this.x.tcs_has_item(id) === 1; }

  clues() {
    const n = this.x.tcs_clue_count();
    const out = [];
    for (let i = 0; i < n; i++) out.push(this.x.tcs_clue_id(i));
    return out;
  }
  hasClue(id) { return this.x.tcs_has_clue(id) === 1; }
  clueHeight() { return this.x.tcs_clue_height(); }

  codeState() {
    const b = this.f32(this.x.tcs_code_ptr(), 8);
    return { digits: [b[0], b[1], b[2], b[3]], progress: b[4], solved: b[5] === 1,
      valves: b[6], candles: b[7] };
  }
  codeDigit(d) { this.x.tcs_code_digit(d); }
  codeReset() { this.x.tcs_code_reset(); }
  puzzleUndo() { this.x.tcs_puzzle_undo(); }

  mapNodes() {
    const n = this.x.tcs_map_node_count();
    const b = this.f32(this.x.tcs_map_nodes(), n * 6);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 6;
      out.push({ id: b[o], nameId: b[o + 1], cx: b[o + 2], cy: b[o + 3], palette: b[o + 4], floor: b[o + 5] });
    }
    return out;
  }
  mapEdges() {
    const n = this.x.tcs_map_edge_count();
    const b = this.f32(this.x.tcs_map_edges(), n * 3);
    const out = [];
    for (let i = 0; i < n; i++) {
      const o = i * 3;
      out.push({ a: b[o], b: b[o + 1], open: b[o + 2] === 1 });
    }
    return out;
  }
  visited(room) { return this.x.tcs_visited(room) === 1; }

  serialize() {
    const size = this.x.tcs_serialize();
    return new Uint8Array(this.mem.buffer, this.x.tcs_save_ptr(), size).slice();
  }
  deserialize(bytes) {
    const ptr = this.x.tcs_save_ptr();
    new Uint8Array(this.mem.buffer, ptr, bytes.length).set(bytes);
    return this.x.tcs_deserialize(ptr, bytes.length) === 1;
  }

  // ---- diagnostic hooks (used by the automated tests)
  bfsReach(from) { return this.x.tcs_bfs_reach(from); }
  bfsDistance(a, b) { return this.x.tcs_bfs_dist(a, b); }
  dfsRegions() { return this.x.tcs_dfs_regions(); }
  enemyState(i) { return this.x.tcs_debug_enemy_state(i); }
  giveItem(id, qty = 1) { this.x.tcs_debug_give(id, qty); }
  teleport(room, x, y) { this.x.tcs_debug_teleport(room, x, y); }
  setPos(x, y) { this.x.tcs_debug_set_pos(x, y); }
  tileAt(cx, cy) { return this.x.tcs_debug_tile(cx, cy); }
  pointBlocked(x, y) { return this.x.tcs_debug_point_blocked(x, y) === 1; }
  hurt(dmg) { this.x.tcs_debug_hurt(dmg); }
  doorUnlocked(id) { return this.x.tcs_debug_door_unlocked(id) === 1; }
  debugInteract() { this.x.tcs_debug_interact(); }
}
