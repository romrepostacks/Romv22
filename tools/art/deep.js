// Party Royale: underwater tiles (Phase 4, DIVE). Original pixel art, 16x16 per map tile, shown at 2x.
// The rock and floor shapes reuse the cave tiles' rows in a seabed palette; deep_weed is kelp on the seabed.
const cave = require('./cave.js');
const ROCK = {K:'#10232c', D:'#1d3c4a', A:'#2b5566', B:'#3d7082', C:'#5a90a0'};
const SAND = {F:'#3a7a8c', f:'#2f6878', g:'#5aa0b0', k:'#255563'};
module.exports = {
  deep_rock: {...cave.cave_wall, pal:ROCK},
  deep_face: {...cave.cave_face, pal:ROCK},
  deep_floor_v0: {...cave.cave_floor_v0, pal:SAND},
  deep_floor_v1: {...cave.cave_floor_v1, pal:SAND},
  deep_weed: {w:16, h:16, pal:{...SAND, G:'#2e8a4a', L:'#56b060', d:'#1e5e34'}, rows:[
    'FFGFFFFFFLFFFFFF',
    'FGLFFFFFFLGFFFGF',
    'FGLFFFLFFGLFFGLF',
    'FdGFFGLFFGLFFGLF',
    'FFGLFGLFFdGFFGdF',
    'FFGLFdGFFFGLFGFF',
    'FFdGFFGLFFGLFGLF',
    'FFFGFFGLFFdGFdGF',
    'FFFGLFdGFFFGFFGF',
    'FFFdGFFGLFFGLFGF',
    'FFFFGFFdGFFdGFdF',
    'FFFFGLFFGFFFGFFF',
    'FfFFdGFFGLFFGLFF',
    'FFFFFGFFdGFFdGFF',
    'FFgFFdFFFGFFFGFF',
    'FFFFFFFFFdFFFdFF',
  ]},
};
