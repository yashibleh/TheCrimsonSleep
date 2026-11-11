// content.js - all player-facing text. Ids mirror Content.hpp / Messages.hpp.

export const ROOMS = {
  1: { name: 'Ward 3', sub: 'The Waking', amb: 'Dust, iodine, and the wet smell of the garden coming through the window.' },
  2: { name: 'Corridor of Bells', sub: '', amb: 'A bell rings somewhere without a rope.' },
  3: { name: 'Nurse Station', sub: '', amb: 'Tea gone cold. A logbook open to a page someone stopped writing.' },
  4: { name: 'Supply Closet', sub: '', amb: 'Shelves of gauze, rust, and things that were never medical.' },
  5: { name: 'Boiler Room', sub: 'Basement', amb: 'The pipes are warm. Something inside them is not water.' },
  6: { name: 'The Flooded Passage', sub: 'Sub-level', amb: 'The water is warm. It breathes when you do.' },
  7: { name: 'East Corridor', sub: '', amb: 'Lockers, burnt carpet, and the sound of something dragging.' },
  8: { name: 'Chapel of the Quiet', sub: '', amb: 'Pews, candles, and a hymn nobody wrote.' },
  9: { name: 'The Cold Room', sub: 'Mortuary', amb: 'Steel drawers. One of them is not closed properly.' },
  10: { name: 'The Crimson Sleep', sub: '', amb: 'A red room with a cradle in it. You have been here before.' }
};

export const ITEMS = {
  0: { name: '—', desc: '' },
  1: { name: 'Rusty Crowbar', desc: 'Heavy enough to break a promise. Swing it with F.' },
  2: { name: 'Service Revolver', desc: 'Six chambers, no God. Fire with F, reload with R.' },
  3: { name: 'Revolver Rounds', desc: 'Scarce. Every shot is a decision the whole building hears.' },
  4: { name: 'Gauze Bandage', desc: 'Stops the bleeding. Does not stop the memory. Use with Q.' },
  5: { name: 'Fuse (Blue)', desc: 'One of three. The breaker box in the boiler room wants it.' },
  6: { name: 'Fuse (Red)', desc: 'One of three. Warm to the touch, like it was recently used.' },
  7: { name: 'Fuse (Brass)', desc: 'One of three. Someone scratched a small face into the casing.' },
  8: { name: 'Ward Key', desc: 'Opens the supply closet off the corridor.' },
  9: { name: 'Valve Handle', desc: 'Fits the three wheels in the boiler room.' },
  10: { name: 'Brass Mortuary Key', desc: 'Opens the chamber at the end of the chapel.' },
  11: { name: 'Lantern Battery', desc: 'Restores the torch. Use from the inventory.' },
  12: { name: 'Sedative Syringe', desc: 'Enough for two, she said. One dose left.' },
  13: { name: "Ilse's Locket", desc: 'A cheap brass locket with a photograph of two girls. It is warm.' },
  14: { name: 'Box of Matches', desc: 'For candles. For light. For burning the sleep out of things.' }
};

export const CLUES = {
  1: { title: 'Admission Ledger', room: 3, body:
    'ASHGROVE SANATORIUM — ADMISSION LEDGER (page torn)\n\n"Voss, Ilse. Age 19. Referred by family. Sleep disturbance, night terrors, self-injury during sleep. Admitted 3:17 AM."\n\nThe handwriting below is yours: "Patient is an ideal candidate for the Quiet Hour."\n\nYou do not remember writing it. You remember the ink.' },
  2: { title: 'Ward Log, Night 14', room: 2, body:
    'WARD LOG — NIGHT 14\n\nBed 3: patient woke screaming that the walls were breathing. Sedated.\nBed 1: patient asked why the doctor has no face.\nBed 4: empty. Again.\n\nStaff are no longer filing reports. I am no longer sure there are staff.' },
  3: { title: 'Note on the Mirror', room: 1, body:
    'Taped to the mirror with surgical tape, in your own handwriting.\n\n"M. — you asked me to write this down so you would believe it when you woke. You are not asleep. You are not awake. The corridor repeats. You have done this eleven times.\n\nDo not look at the mirror for longer than you have to.\n\n— M."\n\nYou do not remember writing it. The tape is still warm.' },
  4: { title: 'Night Shift Memo', room: 3, body:
    'MEMO — NIGHT SHIFT\n\nThe sedative cabinet is to be kept locked. Dr. Voss has been taking syringes without signing them out. When I asked her about the dose she said: "It has to be enough for two."\n\nTwo what, doctor?' },
  5: { title: 'Valve Order', room: 5, body:
    'SCRATCHED INTO THE PAINT, in a hand that pressed too hard:\n\n"South wheel first. The north wheel next. The wheel above the water last. Anything else and the pipes scream."\n\nBelow it, smaller: "I am sorry about the screaming. — E.W."' },
  6: { title: 'Boiler Log', room: 5, body:
    'BOILER LOG\n\n"Pressure holding. Coal is low.\n\nNight 9: something has been sleeping in the furnace and it is not coal. I called the patients\' names down the pipes and it answered each one in my mother\'s voice."\n\nThe rest of the log has been torn out. The stubs are still there.' },
  7: { title: 'Maintenance Tag', room: 4, body:
    'MAINTENANCE TAG\n\n"Three fuses removed while the east wing is dead. Blue, red, brass. Do not replace them. The dark is the only thing keeping it quiet."\n\nA second hand has written underneath, pressing through the card:\n\n"The dark has stopped being kind."' },
  8: { title: 'Scratched in Tile', room: 6, body:
    'SCRATCHED INTO THE TILE at knee height, as if written by someone sitting in the water:\n\n"It walks with its head turned around. It hears the water. It hears everything.\n\nIf it finds you, do not run in a straight line. It remembers straight lines."\n\nThe water is warm.' },
  9: { title: 'The Lectern', room: 8, body:
    'A hymn book lies open, but the words have been replaced.\n\n"We are the sheep who dream the shepherd. We are the sleep he cannot wake from. When the bell rings four, the sleeper becomes the dream."\n\nYou remember ringing a bell. You remember a great many hands holding yours while you did it.' },
  10: { title: 'Four Candles, Their Order', room: 8, body:
    'Written in the margin of the organ score:\n\n"North-east first: the one beside her pillow. Then south-west, beneath the broken window. Then south-east, at the pulpit\'s foot. Last, north-west, where the organ breathes.\n\nShe wakes only if the light goes round the room the way her fever did."' },
  11: { title: 'Diary Page (first numbers)', room: 8, body:
    'A diary page, water-stained, the ink running like something trying to leave.\n\n"The door to the cold room is kept with four numbers. The first two are the hour she was admitted. I have never forgotten it.\n\nI was there when they carried her in, still dreaming with her eyes open."' },
  12: { title: 'Photograph (last numbers)', room: 8, body:
    'The back of a photograph: your sister, smiling at a window you no longer recognise. Her handwriting, not yours, in pencil:\n\n"And the last two are the year we stopped counting. Seventeen.\n\nThe year the Quiet Hour began. The year I stopped being a doctor and became something that keeps a girl asleep, because waking her would mean admitting what I did."\n\nYou turn the photograph over. She is still smiling.' },
  13: { title: 'Toe Tag', room: 9, body:
    'TOE TAG\n\n"VOSS, I. — 19 — No next of kin listed."\n\nAlong the edge, in your handwriting, pressed so hard the paper tore:\n\n"She has next of kin. She has me. I am five minutes down the corridor and eleven nights too late."' },
  14: { title: 'A Letter Never Sent', room: 9, body:
    'A letter, folded into a drawer where nobody would think to look.\n\n"Mara — you keep asking what I dreamed about.\n\nI dreamed of a red room with a cradle in it, and you were standing over me with a needle, and you kept saying \'almost\'. I am not afraid of the room.\n\nI am afraid that you are still in it."\n\nIt is unsigned. It is addressed to the woman you are.' },
  15: { title: 'Case Notes, Session 14', room: 7, body:
    'A file marked SESSION 14 — SUBJECT VOSS, I.\n\nYou do not need to read it. You remember it.\n\nYour own voice, calm, dictating: "Sleep deepens with repetition. She will not wake until the protocol completes.\n\nNeither will I, if I stay to watch. So I will stay."\n\nThe file is warm. You put it down carefully.' },
  16: { title: 'The Crimson Sleep', room: 8, body:
    'WRITTEN ON THE WALL OF THE CHAPEL CELLAR in something dark and dry:\n\n"THE CRIMSON SLEEP is not a place. It is a decision, repeated every night.\n\nShe chose to hold her sister\'s hand in the dark. The dark held back.\n\nHere is the trick of it: the dreamer is the one in the bed. The one walking is the dream, trying to wake up."' },
  17: { title: 'Placard', room: 10, body:
    'SUBJECT: VOSS, I.\nPROTOCOL: QUIET HOUR.\nSTATUS: DREAMING (CONTINUOUS).\nATTENDANT: VOSS, M.\n\nDO NOT WAKE THE ATTENDANT. SHE IS THE DREAM.\n\nUnder the placard, a small brass key on a ribbon — cold, familiar, and heavier than it should be.' },
  18: { title: 'A Child\'s Drawing', room: 2, body:
    'BEHIND THE BOARDS\n\nA child\'s drawing pinned to the lath. A girl in a bed. A woman in a coat. Between them, a red crayon line that goes on and on, through the floor and out of the page.\n\nOn the back: "Mara, if you find this, you are getting better at waking.\n\nTry again. Go left this time. The cradle is not what you think."' }
};

export const PROMPTS = {
  1: 'Read', 2: 'Take', 3: 'Open', 4: 'Enter', 5: 'Locked', 6: 'Needs a key',
  7: 'Hide', 8: 'Leave', 9: 'Light candle', 10: 'Look into the mirror',
  11: 'Breaker box', 12: 'Turn valve', 13: 'Keypad', 14: 'Open drawer',
  15: 'Light candle', 16: 'The cradle', 17: 'Pry the boards', 18: 'Play the organ',
  19: 'Examine the body', 20: 'Examine', 21: 'Unpowered'
};

export const MESSAGES = {
  1: 'The Quiet Hour has not ended.',
  2: 'You have nothing to swing but your own fear.',
  3: 'The valve will not turn without a handle.',
  4: 'The wheel turns. The pipes hold their breath.',
  5: 'Wrong order. The pipes shriek.',
  6: 'The gate is open. Steam pours into the dark.',
  7: 'The fuse clicks into place.',
  8: 'The breaker wants three fuses.',
  9: 'The lights stagger back on.',
  10: 'The keypad is dead. No power.',
  11: 'The numbers are wrong. Something in the wall shifts.',
  12: 'The lock opens.',
  13: 'The door does not recognise you.',
  14: 'It is chained from the other side.',
  15: 'The door is sealed until the power returns.',
  16: 'The lock wants four numbers.',
  17: 'The door is held by pressure. The valves.',
  18: 'The candle catches.',
  19: 'The flame gutters out. Begin again.',
  20: 'Every candle burns. The wall opens — a small, terrible gift.',
  21: 'The boards give way. Something behind them was waiting.',
  22: 'Nailed shut. You would need something to lever it.',
  23: 'The glass shows a woman one second too slow.',
  24: 'Nothing. Just you. Breathing.',
  25: 'You cannot carry any more.',
  26: 'The drawer is empty.',
  27: 'You bandage the wound. It still hurts.',
  28: 'You have nothing to heal with.',
  29: 'Reloading.',
  30: 'No rounds left.',
  31: 'The revolver clicks on an empty chamber.',
  32: 'You feel watched. The candle is lit.',
  33: 'The locket is warm. Footsteps behind you.',
  34: 'The syringe is for her, not for you.',
  35: 'She has no hand to hold. Find her.',
  36: 'The organ answers in a key you did not play.',
  37: 'The body is still warm. You do not look twice.',
  38: 'You fold yourself into the dark.',
  39: 'You step back into the corridor.',
  40: 'The torch dies.',
  41: 'You switch weapons.',
  42: 'You have nothing else to hold.',
  43: 'A brass key. Cold as a small hand.',
  44: 'Empty. Just dust and a smell you know.',
  45: 'It tells you nothing you wanted to know.',
  46: 'You take it.',
  47: 'This door does not need a key. It needs a reason.',
  48: 'The sedative would let her wake. Or let you sleep.',
  49: 'The Weaver does not bleed. It only bends.',
  50: 'The breaker hums. It wants three fuses.'
};

export const SUBTITLES = {
  1: 'You are awake. Or you are still dreaming. The difference stopped mattering eleven nights ago.',
  2: 'Somewhere down the corridor, a bell rings that has no rope.',
  3: 'The water is warm. Something in it is breathing with you.',
  4: 'You are not alone in the dark. You were never alone in the dark.',
  5: 'The cold room remembers her.',
  6: 'The cradle is exactly where you left it. That is the worst part.',
  7: 'She is here. She has always been here. And now she knows you are too.',
  8: 'The east wing wakes up screaming.',
  9: 'Steam finds the seams of the world and pushes.'
};

export const OBJECTIVES = {
  1: 'Find something to defend yourself with.',
  2: 'Restore the power to the east wing.',
  3: 'Find the numbers for the mortuary lock.',
  4: 'Find Ilse.',
  5: 'Open the chamber at the end of the chapel.',
  6: 'Lie down. Or burn the sleep out of her.',
  99: 'It is over.'
};

export const ENDINGS = {
  1: {
    title: 'MORNING, IF IT COMES',
    body: [
      'You push the needle into the cradle\'s line and the red light goes out.',
      'The room folds. You are lying on a floor you remember, with your sister\'s hand in yours and your own name in your mouth. She wakes first. She looks at you the way you have been dreading: not with anger. With recognition.',
      'You walk out through the east wing together. The corridor does not repeat. The bell does not ring. The morning, if it comes, is grey and ordinary and yours.',
      'Somewhere behind you, the cradle keeps a small, patient darkness. You do not look back. That is the whole of survival: a door, and the choice not to open it again.'
    ]
  },
  2: {
    title: 'THE LONG QUIET HOUR',
    body: [
      'You lie down in the cradle because it is warm, and because you are so tired, and because there is a version of this where you are the one being cared for.',
      'The red light does not go out. It gets brighter.',
      'Outside, in a corridor that is not real, a woman in a coat keeps walking. She has been walking for eleven nights. She will keep walking.',
      'Somewhere, a girl in a bed dreams of a red room. She is not afraid of it any more. You are in it.'
    ]
  }
};

export const ENEMY_INFO = [
  { name: 'The Crawler', desc: 'Blind, patient, and led by sound. It drags itself along the ward walls and stops when you stop. It is almost sad until it is on top of you.' },
  { name: 'The Hound', desc: 'Fast, purposeful, and never far. It hunts by sight and remembers where you went. Locked doors and dark corners are the only argument it accepts.' },
  { name: 'The Weaver', desc: 'It does not bleed. It does not tire. It walks the whole building through the doors you open, and it is always already in the next room. You cannot kill it. You can only be somewhere else.' }
];

export const CREDITS = [
  'THE CRIMSON SLEEP',
  '',
  'A 2D survival horror game',
  '',
  'Design, code, art and sound — built for this project',
  'Core simulation and every data structure written in C++17,',
  'compiled to WebAssembly and driven by a thin renderer.',
  '',
  'World graph · BFS · DFS · sound-event queue · enemy decision heap',
  'inventory linked list · clue binary search tree · puzzle action stack',
  'tile / player / enemy arrays',
  '',
  'For anyone who has stayed awake beside someone they could not wake.',
  '',
  'Thank you for playing.'
];
